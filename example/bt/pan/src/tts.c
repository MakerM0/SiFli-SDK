/**
  ******************************************************************************
  * @file   wheather.c
  * @author Sifli software development team
  ******************************************************************************
*/
/**
 * @attention
 * Copyright (c) 2024 - 2025,  Sifli Technology
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Sifli integrated circuit
 *    in a product or a software update for such product, must reproduce the above
 *    copyright notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of Sifli nor the names of its contributors may be used to endorse
 *    or promote products derived from this software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Sifli integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY SIFLI TECHNOLOGY "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SIFLI TECHNOLOGY OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <rtthread.h>
#include "lwip/api.h"
#include "lwip/apps/websocket_client.h"
#include "lwip/apps/mqtt_priv.h"
#include "lwip/apps/mqtt.h"
#include "lwip/tcpip.h"
#include "mbedtls/base64.h"
#include "xiaozhi.h"
#include "bf0_hal.h"
#include "bts2_global.h"
#include "bts2_app_pan.h"
#include <cJSON.h>
#include "button.h"
#include "audio_server.h"

#define MAX_WSOCK_HDR_LEN 512
#define MAX_AUDIO_DATA_LEN 4096

#define TTS_HOST            "ai-gateway.vei.volces.com"
#define TTS_WSPATH          "/v1/realtime?model=doubao-tts"

// Please use your own tts token, applied in https://console.volcengine.com/vei/aigateway/tokens-list 
#define TTS_TOKEN           "sk-e1fxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"


/**
 * @brief xiaozhi websocket cntext 数据结构体
 */
typedef struct
{
    uint32_t  sample_rate;
    uint32_t frame_duration;
    uint32_t event_id;
    wsock_state_t  clnt;
    rt_sem_t sem;
    uint8_t  is_connected;
} tts_ws_t;

tts_ws_t g_tts_ws;
static enum DeviceState g_ws_state;

void parse_response(const u8_t *data, u16_t len);

static err_t my_wsapp_fn(int code, char *buf, size_t len)
{
    if (code == WS_CONNECT)
    {
        int status = (uint16_t)(uint32_t)buf;
        if (status == 101)  // wss setup success
        {
            rt_sem_release(g_tts_ws.sem);
            g_tts_ws.is_connected = 1;
        }
    }
    else if (code == WS_DISCONNECT)
    {
        rt_kprintf("WebSocket closed\n");
        g_tts_ws.is_connected = 0;
        rt_sem_release(g_tts_ws.sem);
    }
    else if (code == WS_TEXT)
    {
        static uint8_t text[WSMSG_MAXSIZE];        
        rt_kprintf("Got Text:%d", len);
        RT_ASSERT(len<(WSMSG_MAXSIZE-1));
        memcpy(text, buf, len);
        text[len]='\0';        
        parse_response(buf, len);
    }
    else
    {
        // Receive Audio Data
        xz_audio_downlink(buf, len, NULL, 0);
    }
    return 0;
}

static void xz_button_event_handler(int32_t pin, button_action_t action)
{
    rt_kprintf("button(%d) %d:", pin, action);

    if (action == BUTTON_PRESSED)
    {
        xz_mic(1);
    }
    else if (action == BUTTON_RELEASED)
    {
        xz_mic(0);
    }
}


static void xz_button_init(void)
{
    static int initialized = 0;

    if (initialized == 0)
    {
        button_cfg_t cfg;
        cfg.pin = BSP_KEY1_PIN;

        cfg.active_state = BSP_KEY1_ACTIVE_HIGH;
        cfg.mode = PIN_MODE_INPUT;
        cfg.button_handler = xz_button_event_handler;
        int32_t id = button_init(&cfg);
        RT_ASSERT(id >= 0);
        RT_ASSERT(SF_EOK == button_enable(id));
        initialized = 1;
    }
}

static void xz_ws_audio_init()
{
    rt_kprintf("xz_audio_init\n");
    audio_server_set_private_volume(AUDIO_TYPE_LOCAL_MUSIC, 15);
    xz_audio_decoder_encoder_open(1);
    xz_button_init();
}

static char *my_json_string(cJSON *json, char *key)
{
    return cJSON_GetObjectItem(json, key)->valuestring;
}

void parse_response(const u8_t *data, u16_t len)
{
    cJSON *item = NULL;
    cJSON *root = NULL;
    rt_kputs(data);
    rt_kputs("--\r\n");
    root = cJSON_Parse(data);   /*json_data 为MQTT的原始数据*/
    if (!root)
    {
        rt_kprintf("Error before: [%s]\n", cJSON_GetErrorPtr());
        return;
    }

    char *type = my_json_string(root, "type");
    if (strcmp(type, "tts_session.updated") == 0)
    {
        item = cJSON_GetObjectItem(root, "sesson");
        g_tts_ws.sample_rate = atoi(my_json_string(item,"output_audio_rate"));
        g_tts_ws.frame_duration = 60;
        g_state = kDeviceStateSpeaking;
        rt_sem_release(g_tts_ws.sem);
        xz_ws_audio_init();
        xz_speaker(1);
    }
    else if (strcmp(type, "response.audio.done") == 0)
    {
        g_state = kDeviceStateIdle;
        xz_speaker(0);
        rt_sem_release(g_tts_ws.sem);
        rt_kprintf("session ended\n");
    }
    else if (strcmp(type, "response.audio.delta") == 0)
    {
        char *delta = my_json_string(root, "delta");
        static uint8_t audio_data[MAX_AUDIO_DATA_LEN];
        int size=0;
        if (ERR_OK==mbedtls_base64_decode(audio_data,MAX_AUDIO_DATA_LEN,&size,delta,strlen(delta))) {
            rt_kprintf("Audio data:%d\r\n",size);
            xz_audio_downlink(audio_data, size, NULL, 0);
        }
    }
    else
    {
        rt_kprintf("Unkown type: %s\n", type);
    }
    cJSON_Delete(root);
}

static const char *config_message = 
"{"
    "\"type\": \"tts_session.update\","
    "\"session\": {"
        "\"voice\":\"zh_female_kailangjiejie_moon_bigtts\","
        "\"output_audio_format\": \"opus\","
        "\"output_audio_sample_rate\": 16000 ,"
        "\"text_to_speech\": {"
          "\"model\": \"doubao-tts\" "
        "}"
    "}"
"}" ;


static const char *tts_req_fmt = 
"{"
    "\"event_id\": \"event_%08d\", "
    "\"type\": \"input_text.append\","
    "\"delta\": \"%s\" "
"}";


static char tts_request[256];
void send_tts_request(char * text)
{
    rt_sprintf(tts_request, tts_req_fmt, g_tts_ws.event_id++, text);
    rt_kprintf("Web socket write tts request %s\r\n", tts_request);
    wsock_write(&g_tts_ws.clnt, tts_request, strlen(tts_request),OPCODE_TEXT);
}

void tts(int argc, char **argv)
{

    err_t err;

    if (g_tts_ws.sem == NULL)
        g_tts_ws.sem = rt_sem_create("xz_ws", 0, RT_IPC_FLAG_FIFO);

    wsock_init(&g_tts_ws.clnt, 1, 1, my_wsapp_fn);
    err = wsock_connect(&g_tts_ws.clnt, MAX_WSOCK_HDR_LEN, TTS_HOST, TTS_WSPATH,
                        LWIP_IANA_PORT_HTTPS, TTS_TOKEN, NULL,
                        "Content-Type: application/json\r\n");
    rt_kprintf("Web socket connection %d\r\n", err);
    if (err == 0)
    {
        if (RT_EOK == rt_sem_take(g_tts_ws.sem, 5000))
        {
            if (g_tts_ws.is_connected)
            {
                rt_kprintf("Web socket write config %s\r\n", config_message);
                err = wsock_write(&g_tts_ws.clnt, config_message, strlen(config_message), OPCODE_TEXT);
                if (ERR_OK==err && rt_sem_take(g_tts_ws.sem, 5000)==RT_EOK) {   
                    LOCK_TCPIP_CORE();
                    send_tts_request(argv[1]);                    
                    UNLOCK_TCPIP_CORE();                    
                }
                if (RT_EOK==rt_sem_take(g_tts_ws.sem, 50000)==RT_EOK)
                    rt_kprintf("Finish TTS");
                else
                    rt_kprintf("TTS timeout");
            }
            else
            {
                rt_kprintf("Web socket disconnected\r\n");
            }
        }
        else
        {
            rt_kprintf("Web socket connected timeout\r\n");
        }
        wsock_close(&g_tts_ws.clnt, WSOCK_RESULT_OK,ERR_OK);
    }
}
MSH_CMD_EXPORT(tts, Text to speech)



/************************ (C) COPYRIGHT Sifli Technology *******END OF FILE****/

