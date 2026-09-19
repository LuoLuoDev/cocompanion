#include "ai_client.h"
#include "sdkconfig.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "ai";

static char s_rx[4096];
static int s_rx_len = 0;

static bool s_busy = false;

typedef struct {
    char question[256];
    ai_reply_cb cb;
    void *user_data;
} ai_request_t;

static ai_request_t s_req;

bool ai_client_busy(void)
{
    return s_busy;
}

bool ai_client_configured(void)
{
    return CONFIG_COCOMPANION_AI_API_KEY[0] != '\0' && CONFIG_COCOMPANION_AI_ENDPOINT[0] != '\0';
}

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        if (s_rx_len + evt->data_len < (int)sizeof(s_rx) - 1) {
            memcpy(s_rx + s_rx_len, evt->data, evt->data_len);
            s_rx_len += evt->data_len;
            s_rx[s_rx_len] = '\0';
        }
    }
    return ESP_OK;
}

static void do_request(ai_request_t *r, char *reply_out, size_t reply_size)
{
    reply_out[0] = '\0';

    /* Build JSON body */
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "model", CONFIG_COCOMPANION_AI_MODEL);

    cJSON *msgs = cJSON_AddArrayToObject(root, "messages");
    cJSON *sys = cJSON_CreateObject();
    cJSON_AddStringToObject(sys, "role", "system");
    cJSON_AddStringToObject(sys, "content", "You are a helpful assistant on a small smartwatch-like device. Reply concisely in English only.");
    cJSON_AddItemToArray(msgs, sys);

    cJSON *usr = cJSON_CreateObject();
    cJSON_AddStringToObject(usr, "role", "user");
    cJSON_AddStringToObject(usr, "content", r->question);
    cJSON_AddItemToArray(msgs, usr);

    cJSON_AddNumberToObject(root, "max_tokens", CONFIG_COCOMPANION_AI_MAX_TOKENS);
    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (body == NULL) {
        return;
    }

    esp_http_client_config_t cfg = {
        .url = CONFIG_COCOMPANION_AI_ENDPOINT,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 30000,
        .event_handler = http_event_handler,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == NULL) {
        free(body);
        return;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    char auth[300];
    snprintf(auth, sizeof(auth), "Bearer %s", CONFIG_COCOMPANION_AI_API_KEY);
    esp_http_client_set_header(client, "Authorization", auth);

    s_rx_len = 0;
    s_rx[0] = '\0';

    esp_http_client_set_post_field(client, body, (int)strlen(body));
    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    free(body);

    if (err != ESP_OK) {
        snprintf(reply_out, reply_size, "Network error (%s)", esp_err_to_name(err));
        return;
    }

    if (status != 200) {
        /* try to extract error.message from body */
        cJSON *ejson = cJSON_Parse(s_rx);
        if (ejson != NULL) {
            cJSON *errmsg = cJSON_GetObjectItem(ejson, "error");
            if (errmsg != NULL) {
                cJSON *msg = cJSON_GetObjectItem(errmsg, "message");
                const char *t = cJSON_GetStringValue(msg);
                if (t != NULL) {
                    snprintf(reply_out, reply_size, "HTTP %d: %s", status, t);
                } else {
                    snprintf(reply_out, reply_size, "HTTP %d", status);
                }
            } else {
                snprintf(reply_out, reply_size, "HTTP %d", status);
            }
            cJSON_Delete(ejson);
        } else {
            snprintf(reply_out, reply_size, "HTTP %d", status);
        }
        return;
    }

    cJSON *resp = cJSON_Parse(s_rx);
    if (resp == NULL) {
        snprintf(reply_out, reply_size, "Bad response");
        return;
    }
    cJSON *choices = cJSON_GetObjectItem(resp, "choices");
    cJSON *first = choices != NULL ? cJSON_GetArrayItem(choices, 0) : NULL;
    cJSON *msg = first != NULL ? cJSON_GetObjectItem(first, "message") : NULL;
    cJSON *content = msg != NULL ? cJSON_GetObjectItem(msg, "content") : NULL;
    const char *text = cJSON_GetStringValue(content);
    if (text != NULL) {
        snprintf(reply_out, reply_size, "%s", text);
    } else {
        snprintf(reply_out, reply_size, "Empty reply");
    }
    cJSON_Delete(resp);
}

static void ai_task(void *arg)
{
    (void)arg;
    char reply[2048];
    do_request(&s_req, reply, sizeof(reply));

    if (s_req.cb != NULL) {
        s_req.cb(reply, reply[0] != '\0', s_req.user_data);
    }
    s_busy = false;
    vTaskDelete(NULL);
}

void ai_client_ask(const char *question, ai_reply_cb cb, void *user_data)
{
    if (s_busy) {
        return;
    }
    if (!ai_client_configured()) {
        if (cb != NULL) {
            cb("API key not configured (menuconfig -> cocompanion -> AI Chat).", false, user_data);
        }
        return;
    }
    s_busy = true;
    strncpy(s_req.question, question, sizeof(s_req.question) - 1);
    s_req.question[sizeof(s_req.question) - 1] = '\0';
    s_req.cb = cb;
    s_req.user_data = user_data;

    ESP_LOGI(TAG, "asking: %s", question);
    xTaskCreate(ai_task, "ai_task", 16384, NULL, 5, NULL);
}

void ai_client_init(void)
{
    /* nothing to allocate; HTTP client is created per-request */
}
