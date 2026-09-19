#include "wifi_mgr.h"
#include "sdkconfig.h"
#include <string.h>
#include <time.h>

#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_netif_sntp.h"
#include "esp_log.h"

static const char *TAG = "wifi";

static wifi_state_t s_state = WIFI_DISCONNECTED;
static char s_ip[16] = {0};

static void start_sntp(void)
{
    esp_sntp_config_t cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_netif_sntp_init(&cfg);
    ESP_LOGI(TAG, "SNTP started");
}

static void event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        s_state = WIFI_CONNECTING;
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        s_state = WIFI_DISCONNECTED;
        s_ip[0] = '\0';
        esp_wifi_connect(); /* keep retrying */
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)data;
        snprintf(s_ip, sizeof(s_ip), IPSTR, IP2STR(&ev->ip_info.ip));
        s_state = WIFI_CONNECTED;
        ESP_LOGI(TAG, "Got IP: %s", s_ip);
        start_sntp();
    }
}

wifi_state_t wifi_mgr_get_state(void)
{
    return s_state;
}

const char *wifi_mgr_get_ip(void)
{
    return s_ip;
}

void wifi_mgr_start(void)
{
    /* China Standard Time (UTC+8) */
    setenv("TZ", "CST-8", 1);
    tzset();

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    const char *ssid = CONFIG_COCOMPANION_WIFI_SSID;
    const char *pass = CONFIG_COCOMPANION_WIFI_PASSWORD;

    if (ssid == NULL || ssid[0] == '\0') {
        ESP_LOGW(TAG, "No Wi-Fi SSID configured (menuconfig -> cocompanion -> Wi-Fi). Staying offline.");
        s_state = WIFI_DISCONNECTED;
        return;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event_handler, NULL);

    wifi_config_t wc = {0};
    strncpy((char *)wc.sta.ssid, ssid, sizeof(wc.sta.ssid) - 1);
    strncpy((char *)wc.sta.password, pass, sizeof(wc.sta.password) - 1);
    wc.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wc);
    esp_wifi_start();
    s_state = WIFI_CONNECTING;
    ESP_LOGI(TAG, "Connecting to %s ...", ssid);
}
