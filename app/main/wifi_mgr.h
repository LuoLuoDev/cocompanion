#pragma once

typedef enum {
    WIFI_DISCONNECTED = 0,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
} wifi_state_t;

void wifi_mgr_start(void);
wifi_state_t wifi_mgr_get_state(void);
const char *wifi_mgr_get_ip(void);
