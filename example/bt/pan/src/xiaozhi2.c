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
#include "xiaozhi.h"
#include "bf0_hal.h"
#include "bts2_global.h"
#include "bts2_app_pan.h"
#include <cJSON.h>
#include "button.h"
#include "audio_server.h"

#define MAX_WSOCK_HDR_LEN 512

/**
 * @brief xiaozhi websocket cntext 数据结构体
 */
typedef struct
{
    uint32_t  sample_rate;
    uint32_t frame_duration;
    uint8_t  session_id[12];
    wsock_state_t  clnt;
    rt_sem_t sem;
    uint8_t  is_connected;
} xiaozhi_ws_t;

xiaozhi_ws_t g_xz_ws;
enum DeviceState g_ws_state;

static const char *hello_message = "{"
                                   "\"type\":\"hello\","
                                   "\"version\": 3,"
                                   "\"transport\":\"websocket\","
                                   "\"audio_params\":{"
                                   "\"format\":\"opus\", \"sample_rate\":16000, \"channels\":1, \"frame_duration\":60"
                                   "}}";

void parse_helLo(const u8_t *data, u16_t len);

void xz_audio_send_using_websocket(uint8_t *data, int len)
{
    err_t err = wsock_write(&g_xz_ws.clnt, data, len, OPCODE_BINARY);
    rt_kprintf("send audio = %d len=%d\n", err, len);
}

err_t my_wsapp_fn(int code, char *buf, size_t len)
{
    if (code == WS_CONNECT)
    {
        int status = (uint16_t)(uint32_t)buf;
        if (status == 101)  // wss setup success
        {
            rt_sem_release(g_xz_ws.sem);
            g_xz_ws.is_connected = 1;
        }
    }
    else if (code == WS_DISCONNECT)
    {
        rt_kprintf("WebSocket closed\n");
        g_xz_ws.is_connected = 0;
        rt_sem_release(g_xz_ws.sem);
    }
    else if (code == WS_TEXT)
    {
        rt_kprintf("Got Text:");
        parse_helLo(buf, len);
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
        ws_send_listen_start(&g_xz_ws.clnt, g_xz_ws.session_id, kListeningModeAutoStop);
        xz_mic(1);
    }
    else if (action == BUTTON_RELEASED)
    {
        ws_send_listen_stop(&g_xz_ws.clnt, g_xz_ws.session_id);
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


void xz_ws_audio_init()
{
    rt_kprintf("xz_audio_init\n");
    audio_server_set_private_volume(AUDIO_TYPE_LOCAL_MUSIC, 15);
    xz_audio_decoder_encoder_open(1);
    xz_button_init();
}


static char *my_json_string(cJSON *json, char *key)
{
    cJSON *item  = cJSON_GetObjectItem(json, key);
    char *r = cJSON_Print(item);

    if (r && ((*r) == '\"'))
    {
        r++;
        r[strlen(r) - 1] = '\0';
    }
    return r;
}

void parse_helLo(const u8_t *data, u16_t len)
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
    if (strcmp(type, "hello") == 0)
    {
        char *session_id = my_json_string(root, "session_id");
        cJSON *audio_param = cJSON_GetObjectItem(root, "audio_params");
        char *sample_rate = my_json_string(audio_param, "sample_rate");
        char *duration = my_json_string(audio_param, "duration");
        g_xz_ws.sample_rate = atoi(sample_rate);
        g_xz_ws.frame_duration = atoi(duration);
        strncpy(g_xz_ws.session_id, session_id, 9);
        g_state = kDeviceStateIdle;
        xz_ws_audio_init();
    }
    else if (strcmp(type, "goodbye") == 0)
    {
        g_state = kDeviceStateUnknown;
        rt_kprintf("session ended\n");
    }
    else if (strcmp(type, "tts") == 0)
    {
        char *state = my_json_string(root, "state");

        if (strcmp(state, "start") == 0)
        {
            if (g_state == kDeviceStateIdle || g_state == kDeviceStateListening)
            {
                g_state = kDeviceStateSpeaking;
                xz_speaker(1);
            }
        }
        else if (strcmp(state, "stop") == 0)
        {
            g_state = kDeviceStateIdle;
            xz_speaker(0);
        }
        else if (strcmp(state, "sentence_start") == 0)
        {
            char *txt = my_json_string(root, "text");
            rt_kputs(txt);
        }
    }
    else
    {
        rt_kprintf("Unkown type: %s\n", type);
    }
}

void xiaozhi2(int argc, char **argv)
{

    err_t err;

    if (g_xz_ws.sem == NULL)
        g_xz_ws.sem = rt_sem_create("xz_ws", 0, RT_IPC_FLAG_FIFO);

    wsock_init(&g_xz_ws.clnt, 1, 1, my_wsapp_fn);
    err = wsock_connect(&g_xz_ws.clnt, MAX_WSOCK_HDR_LEN, XIAOZHI_HOST, XIAOZHI_WSPATH,
                        LWIP_IANA_PORT_HTTPS, XIAOZHI_TOKEN, NULL,
                        "Protocol-Version: 1\r\nDevice-Id: %s\r\nClient-Id: 12345678-1234-1234-1234-123456789012\r\n", get_mac_address());
    rt_kprintf("Web socket connection %d\r\n", err);
    if (err == 0)
    {
        if (RT_EOK == rt_sem_take(g_xz_ws.sem, 5000))
        {
            if (g_xz_ws.is_connected)
            {
                err = wsock_write(&g_xz_ws.clnt, hello_message, strlen(hello_message), OPCODE_TEXT);
                rt_kprintf("Web socket write %d\r\n", err);
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
    }
}
MSH_CMD_EXPORT(xiaozhi2, Get Xiaozhi)



/************************ (C) COPYRIGHT Sifli Technology *******END OF FILE****/

