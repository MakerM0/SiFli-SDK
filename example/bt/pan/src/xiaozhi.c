/**
  ******************************************************************************
  * @file   xiaozhi.c
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
#include "lwip/dns.h"
#include "lwip/apps/websocket_client.h"
#include "lwip/apps/mqtt_priv.h"
#include "lwip/apps/mqtt.h"
#include "bf0_hal.h"
#include "bts2_global.h"
#include "bts2_app_pan.h"

#ifdef LWIP_ALTCP_TLS
    #include <lwip/altcp_tls.h>
#endif

#include <webclient.h>
#include <cJSON.h>
#include "xiaozhi.h"

xiaozhi_context_t g_xz_context;
enum DeviceState g_state;
int g_keep_listening;

static char message[256];

static const char *ota_version = "{\r\n "
                                 "\"version\": 2,\r\n"
                                 "\"flash_size\": 4194304,\r\n"
                                 "\"psram_size\": 0,\r\n"
                                 "\"minimum_free_heap_size\": 123456,\r\n"
                                 "\"mac_address\": \"%s\",\r\n"
                                 "\"uuid\": \"12345678-1234-1234-1234-123456789012\",\r\n"
                                 "\"chip_model_name\": \"sf32lb563\",\r\n"
                                 "\"chip_info\": {\r\n"
                                 "    \"model\": 1,\r\n"
                                 "    \"cores\": 2,\r\n"
                                 "    \"revision\": 0,\r\n"
                                 "    \"features\": 0\r\n"
                                 "},\r\n"
                                 "\"application\": {\r\n"
                                 "    \"name\": \"my-app\",\r\n"
                                 "    \"version\": \"1.0.0\",\r\n"
                                 "    \"compile_time\": \"2021-01-01T00:00:00Z\",\r\n"
                                 "    \"idf_version\": \"4.2-dev\",\r\n"
                                 "    \"elf_sha256\": \"\"\r\n"
                                 "},\r\n"
                                 "\"partition_table\": [\r\n"
                                 "    {\r\n"
                                 "        \"label\": \"app\",\r\n"
                                 "        \"type\": 1,\r\n"
                                 "        \"subtype\": 2,\r\n"
                                 "        \"address\": 10000,\r\n"
                                 "        \"size\": 100000\r\n"
                                 "    }\r\n"
                                 "],\r\n"
                                 "\"ota\": {\r\n"
                                 "    \"label\": \"ota_0\"\r\n"
                                 "},\r\n"
                                 "\"board\": {\r\n"
                                 "    \"type\":\"hdk563\",\r\n"
                                 "    \"mac\": \"%s\"\r\n"
                                 "}\r\n"
                                 "}\r\n"
                                 ;

static const char *hello_message = "{"
                                   "\"type\":\"hello\","
                                   "\"version\": 3,"
                                   "\"transport\":\"udp\","
                                   "\"audio_params\":{"
                                   "\"format\":\"opus\", \"sample_rate\":16000, \"channels\":1, \"frame_duration\":60"
                                   "}}";

const char *mode_str[] =
{
    "auto",
    "manual",
    "realtime"
};

char mac_address_string[20];

char *get_mac_address()
{
    if (mac_address_string[0] == '\0')
    {
        BTS2S_ETHER_ADDR   addr = bt_pan_get_mac_address(NULL);
        uint8_t *p = (uint8_t *) & (addr);

        rt_snprintf((char *)mac_address_string, 20, "%02x:%02x:%02x:%02x:%02x:%02x",
                    *p, *(p + 1), *(p + 2), *(p + 3), *(p + 4), *(p + 5));
    }
    return (&(mac_address_string[0]));
}

static void svr_found_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
    if (ipaddr != NULL)
    {
        rt_kprintf("DNS lookup succeeded, IP: %s\n", ipaddr_ntoa(ipaddr));
    }
}

void my_mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
    xiaozhi_context_t *ctx = (xiaozhi_context_t *)arg;
    rt_kprintf("my_mqtt_connection_cb:%d\n", status);
    if (status == MQTT_CONNECT_ACCEPTED)
    {
        rt_sem_release(ctx->sem);
    }
    else
    {
        // TODO: Reset MQTT parameters.
    }

}

static void mqtt_found_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
    if (ipaddr != NULL)
    {
        xiaozhi_context_t *ctx = (xiaozhi_context_t *)callback_arg;
        rt_kprintf("DNS lookup succeeded, IP: %s\n", ipaddr_ntoa(ipaddr));
        memcpy(&(ctx->mqtt_addr), ipaddr, sizeof(ip_addr_t));
        rt_sem_release(ctx->sem);
    }
}

static int check_internet_access()
{
    int r = 0;
    const char *hostname = XIAOZHI_HOST;
    ip_addr_t addr = {0};

    {
        err_t err = dns_gethostbyname(hostname, &addr, svr_found_callback, NULL);
        if (err != ERR_OK && err != ERR_INPROGRESS)
        {
            rt_kprintf("Coud not find %s, please check PAN connection\n", hostname);
        }
        else
            r = 1;
    }

    return r;
}

void my_mqtt_request_cb(void *arg, err_t err)
{
    xiaozhi_context_t *ctx = (xiaozhi_context_t *)arg;
    rt_kprintf("MQTT Request : %d\n", err);
}

void my_mqtt_request_cb2(void *arg, err_t err)
{
    xiaozhi_context_t *ctx = (xiaozhi_context_t *)arg;
    rt_kprintf("MQTT Request2 : %d\n", err);
    rt_sem_release(ctx->sem);
}


void my_mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{
    rt_kprintf("MQTT incoming topic : %d\n", tot_len);
    rt_kputs(topic);
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

void my_mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
    xiaozhi_context_t *ctx = (xiaozhi_context_t *)arg;
    rt_kprintf("MQTT incoming pub data : %d, %x\n", len, flags);
    rt_kputs(data);


    cJSON *item = NULL;
    cJSON *root = NULL;
    rt_kputs(data);
    rt_kputs("\r\n");
    root = cJSON_Parse(data);   /*json_data 为MQTT的原始数据*/
    if (!root)
    {
        rt_kprintf("Error before: [%s]\n", cJSON_GetErrorPtr());
        return;
    }

    char *type = my_json_string(root, "type");
    if (strcmp(type, "hello") == 0)
    {
        cJSON *udp = cJSON_GetObjectItem(root, "udp");
        char *server = my_json_string(udp, "server");
        char *port = my_json_string(udp, "port");
        char *key = my_json_string(udp, "key");
        char *nonce = my_json_string(udp, "nonce");

        ip4addr_aton(server, &(ctx->udp_addr));
        ctx->port = atoi(port);
        hex2data(key, ctx->key, 16);
        hex2data(nonce, ctx->nonce, 16);

        cJSON *audio_param = cJSON_GetObjectItem(root, "audio_params");
        char *sample_rate = my_json_string(audio_param, "sample_rate");
        char *duration = my_json_string(audio_param, "duration");
        ctx->sample_rate = atoi(sample_rate);
        ctx->frame_duration = atoi(duration);

        char *session_id = my_json_string(root, "session_id");
        strncpy(ctx->session_id, session_id, 9);
        g_state = kDeviceStateIdle;
        xz_audio_init();
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
            if (g_keep_listening)
            {
                mqtt_listen_start(ctx, kListeningModeAutoStop);
                g_state = kDeviceStateListening;
            }
            else
            {
                g_state = kDeviceStateIdle;
            }
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
    cJSON_Delete(root);/*每次调用cJSON_Parse函数后，都要释放内存*/
}

void mqtt_hello(xiaozhi_context_t *ctx)
{
    rt_kprintf("Publish topic:");
    rt_kputs(ctx->publish_topic);

    rt_kprintf("\r\nhello_message:");
    rt_kputs(hello_message);
    rt_kprintf("\r\n");
    mqtt_set_inpub_callback(&(ctx->clnt), my_mqtt_incoming_publish_cb, my_mqtt_incoming_data_cb, ctx);
    mqtt_publish(&(ctx->clnt), ctx->publish_topic, hello_message, strlen(hello_message), 0, 0, my_mqtt_request_cb, ctx);
}

void mqtt_listen_start(xiaozhi_context_t *ctx, int mode)
{
    rt_snprintf(message, 256, "{\"session_id\":\"%s\",\"type\":\"listen\",\"state\":\"start\",\"mode\":\"%s\"}",
                ctx->session_id, mode_str[mode]);
    mqtt_publish(&(ctx->clnt), ctx->publish_topic, message, strlen(message), 0, 0, my_mqtt_request_cb2, ctx);
    rt_sem_take(ctx->sem, 5000);
}

void ws_send_listen_start(void *ws, char *session_id, enum ListeningMode mode)
{
    rt_snprintf(message, 256, "{\"session_id\":\"%s\",\"type\":\"listen\",\"state\":\"start\",\"mode\":\"%s\"}",
                session_id, mode_str[mode]);
    //rt_kputs("\r\n");
    //rt_kputs(message);
    //rt_kputs("\r\n");
    wsock_write((wsock_state_t *)ws, message, strlen(message), OPCODE_TEXT);
}

void ws_send_listen_stop(void *ws, char *session_id)
{
    rt_snprintf(message, 256, "{\"session_id\":\"%s\",\"type\":\"listen\",\"state\":\"stop\"}",
                session_id);
    wsock_write((wsock_state_t *)ws, message, strlen(message), OPCODE_TEXT);
}

void mqtt_listen_stop(xiaozhi_context_t *ctx)
{
    rt_snprintf(message, 256, "{\"session_id\":\"%s\",\"type\":\"listen\",\"state\":\"stop\"}",
                ctx->session_id);
    mqtt_publish(&(ctx->clnt), ctx->publish_topic, message, strlen(message), 0, 0, my_mqtt_request_cb2, ctx);
    rt_sem_take(ctx->sem, 5000);
}

void mqtt_speak_abort(xiaozhi_context_t *ctx, int reason)
{
    rt_snprintf(message, 256, "{\"session_id\":\"%s\",\"type\":\"abort\"",
                ctx->session_id);
    if (reason)
        strcat(message, ",\"reason\":\"wake_word_detected\"}");
    else
        strcat(message, "}");
    mqtt_publish(&(ctx->clnt), ctx->publish_topic, message, strlen(message), 0, 0, my_mqtt_request_cb2, ctx);
    rt_sem_take(ctx->sem, 5000);
}

void mqtt_wake_word_detected(xiaozhi_context_t *ctx, char *wakeword)
{
    rt_snprintf(message, 256, "{\"session_id\":\"%s\",\"type\":\"listen\", \"state\":\"detected\",\"text\":\"%s\"",
                ctx->session_id, wakeword);
    mqtt_publish(&(ctx->clnt), ctx->publish_topic, message, strlen(message), 0, 0, my_mqtt_request_cb2, ctx);
    rt_sem_take(ctx->sem, 5000);
}

void mqtt_iot_descriptor(xiaozhi_context_t *ctx, char *descriptors)
{
    rt_snprintf(message, 256, "{\"session_id\":\"%s\",\"type\":\"iot\", \"descriptor\":\"%s\"",
                ctx->session_id, descriptors);
    mqtt_publish(&(ctx->clnt), ctx->publish_topic, message, strlen(message), 0, 0, my_mqtt_request_cb2, ctx);
    rt_sem_take(ctx->sem, 5000);
}

mqtt_client_t *mqtt_xiaozhi(xiaozhi_context_t *ctx)
{
    mqtt_client_t *clnt = &ctx->clnt;
    struct mqtt_connect_client_info_t *info = &ctx->info;
    err_t err;

    info->client_id = ctx->client_id;
    info->client_user = ctx->username;
    info->client_pass = ctx->password;
    info->keep_alive = 90;

    err = dns_gethostbyname(ctx->endpoint, &ctx->mqtt_addr, mqtt_found_callback, ctx);
    if (err != ERR_OK && err != ERR_INPROGRESS)
    {
        rt_kprintf("Coud not find %s, please check PAN connection\n", ctx->endpoint);
        clnt = NULL;
    }
    else if (RT_EOK == rt_sem_take(ctx->sem, 5000))
    {
        g_state = kDeviceStateConnecting;
        // TODO free config when finish
        info->tls_config = altcp_tls_create_config_client(NULL, 0);
        mqtt_client_connect(&(ctx->clnt), &(ctx->mqtt_addr), LWIP_IANA_PORT_SECURE_MQTT, my_mqtt_connection_cb, ctx, &ctx->info);
        if (RT_EOK == rt_sem_take(ctx->sem, 5000))
        {
            g_state = kDeviceStateIdle;
            ctx->info.tls_config = altcp_tls_create_config_client(NULL, 0);
            mqtt_hello(ctx);
        }
    }
    else
        clnt = NULL;

    return clnt;
}


int http_xiaozhi_data_parse(char *json_data)
{

    uint8_t i, j;
    uint8_t result_array_size = 0;

    cJSON *item = NULL;
    cJSON *root = NULL;

    rt_kputs(json_data);
    root = cJSON_Parse(json_data);   /*json_data 为MQTT的原始数据*/
    if (!root)
    {
        rt_kprintf("Error before: [%s]\n", cJSON_GetErrorPtr());
        return  -1;
    }


    cJSON *Presult = cJSON_GetObjectItem(root, "mqtt");  /*mqtt的键值对为数组，*/
    result_array_size = cJSON_GetArraySize(Presult);  /*求results键值对数组中有多少个元素*/
    item = cJSON_GetObjectItem(Presult, "endpoint");
    g_xz_context.endpoint = cJSON_Print(item);
    item = cJSON_GetObjectItem(Presult, "client_id");
    g_xz_context.client_id = cJSON_Print(item);
    item = cJSON_GetObjectItem(Presult, "username");
    g_xz_context.username = cJSON_Print(item);
    item = cJSON_GetObjectItem(Presult, "password");
    g_xz_context.password = cJSON_Print(item);
    item = cJSON_GetObjectItem(Presult, "publish_topic");
    g_xz_context.publish_topic = cJSON_Print(item);

    // Skip the "..." in string
    g_xz_context.endpoint++;
    g_xz_context.endpoint[strlen(g_xz_context.endpoint) - 1] = '\0';
    g_xz_context.client_id++;
    g_xz_context.client_id[strlen(g_xz_context.client_id) - 1] = '\0';
    g_xz_context.username++;
    g_xz_context.username[strlen(g_xz_context.username) - 1] = '\0';
    g_xz_context.password++;
    g_xz_context.password[strlen(g_xz_context.password) - 1] = '\0';
    g_xz_context.publish_topic++;
    g_xz_context.publish_topic[strlen(g_xz_context.publish_topic) - 1] = '\0';

    rt_kprintf("\r\nmqtt:\r\n\t%s\r\n\t%s\r\n\r\n", g_xz_context.endpoint, g_xz_context.client_id);
    rt_kprintf("\t%s\r\n\t%s\r\n", g_xz_context.username, g_xz_context.password);
    rt_kprintf("\t%s\r\n", g_xz_context.publish_topic);
    mqtt_xiaozhi(&g_xz_context);
    cJSON_Delete(root);/*每次调用cJSON_Parse函数后，都要释放内存*/
    return  0;
}

char *get_xiaozhi()
{
    char *buffer = RT_NULL;
    int resp_status;
    struct webclient_session *session = RT_NULL;
    char *xiaozhi_url = RT_NULL;
    int content_length = -1, bytes_read = 0;
    int content_pos = 0;

    if (check_internet_access() == 0)
        return buffer;

    int size = strlen(ota_version) + sizeof(mac_address_string) * 2 + 16;
    char *ota_formatted = rt_malloc(size);
    if (!ota_formatted)
    {
        goto __exit;
    }
    rt_snprintf(ota_formatted, size, ota_version, get_mac_address(), get_mac_address());

    /* 为 weather_url 分配空间 */
    xiaozhi_url = rt_calloc(1, GET_URL_LEN_MAX);
    if (xiaozhi_url == RT_NULL)
    {
        rt_kprintf("No memory for xiaozhi_url!\n");
        goto __exit;
    }
    /* 拼接 GET 网址 */
    rt_snprintf(xiaozhi_url, GET_URL_LEN_MAX, GET_URI, XIAOZHI_HOST);

    /* 创建会话并且设置响应的大小 */
    session = webclient_session_create(GET_HEADER_BUFSZ);
    if (session == RT_NULL)
    {
        rt_kprintf("No memory for get header!\n");
        goto __exit;
    }

    webclient_header_fields_add(session, "Device-Id: %s \r\n", get_mac_address());
    webclient_header_fields_add(session, "Client-Id: 12345678-1234-1234-1234-123456789012 \r\n");
    webclient_header_fields_add(session, "Content-Type: application/json \r\n");
    webclient_header_fields_add(session, "Content-length: %d \r\n", strlen(ota_formatted));
    //webclient_header_fields_add(session, "X-language:");

    /* 发送 GET 请求使用默认的头部 */
    if ((resp_status = webclient_post(session, xiaozhi_url, ota_formatted, strlen(ota_formatted))) != 200)
    {
        rt_kprintf("webclient Post request failed, response(%d) error.\n", resp_status);
        //goto __exit;
    }

    /* 分配用于存放接收数据的缓冲 */
    buffer = rt_calloc(1, GET_RESP_BUFSZ);
    if (buffer == RT_NULL)
    {
        rt_kprintf("No memory for data receive buffer!\n");
        goto __exit;
    }

    content_length = webclient_content_length_get(session);
    if (content_length > 0)
    {
        do
        {
            bytes_read = webclient_read(session, buffer + content_pos,
                                        content_length - content_pos > GET_RESP_BUFSZ ?
                                        GET_RESP_BUFSZ : content_length - content_pos);
            if (bytes_read <= 0)
            {
                break;
            }
            content_pos += bytes_read;
        }
        while (content_pos < content_length);
    }
    else
    {
        rt_free(buffer);
        buffer = NULL;
    }
__exit:
    /* 释放网址空间 */
    if (xiaozhi_url != RT_NULL)
    {
        rt_free(xiaozhi_url);
        xiaozhi_url = RT_NULL;
    }

    /* 关闭会话 */
    if (session != RT_NULL)
        webclient_close(session);

    if (ota_formatted)
    {
        rt_free(ota_formatted);
    }
    return buffer;
}

void xiaozhi(int argc, char **argv)
{
    char *my_ota_version;

    my_ota_version = get_xiaozhi();

    if (g_xz_context.info.tls_config)
    {
        mqtt_disconnect(&(g_xz_context.clnt));
        if (g_xz_context.info.tls_config)
        {
            altcp_tls_free_config(g_xz_context.info.tls_config);
            g_xz_context.info.tls_config = NULL;
        }
    }
    if (my_ota_version)
    {
        if (g_xz_context.sem == NULL)
            g_xz_context.sem = rt_sem_create("xz_sem", 0, RT_IPC_FLAG_FIFO);
        http_xiaozhi_data_parse(my_ota_version);
        rt_free(my_ota_version);
    }
}
MSH_CMD_EXPORT(xiaozhi, Get Xiaozhi)



/************************ (C) COPYRIGHT Sifli Technology *******END OF FILE****/

