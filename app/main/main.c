#include <stdio.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "bsp/esp-bsp.h"
#include "ui.h"
#include "wifi_mgr.h"
#include "ai_client.h"

static const char *TAG = "main";

void app_main(void)
{
    /* NVS is required by Wi-Fi */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Display + LVGL + touch are initialized by the BSP */
    lv_display_t *disp = bsp_display_start();
    if (disp == NULL) {
        ESP_LOGE(TAG, "Display init failed");
        abort();
    }
    bsp_display_brightness_set(80);

    /* Build the UI (launcher + apps) */
    /* NOTE: bsp_display_lock(0) does NOT block (the BSP doc is wrong; 0 maps to
     * pdMS_TO_TICKS(0) = no wait). Use portMAX_DELAY to wait for the LVGL mutex. */
    if (bsp_display_lock(portMAX_DELAY)) {
        ui_init();
        bsp_display_unlock();
    }

    /* Networking */
    ai_client_init();
    wifi_mgr_start();

    ESP_LOGI(TAG, "cocompanion ready");
}
