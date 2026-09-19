#include "ui.h"
#include <string.h>

static const ui_app_t s_apps[] = {
    { "AI Chat",    "AI",  app_ai_create },
    { "Calculator", "123", app_calculator_create },
    { "Clock",      "CLK", app_clock_create },
    { "Settings",   "SET", app_settings_create },
};

#define APP_COUNT (sizeof(s_apps) / sizeof(s_apps[0]))

static void tile_cb(lv_event_t *e)
{
    const ui_app_t *app = (const ui_app_t *)lv_event_get_user_data(e);
    if (app != NULL && app->create != NULL) {
        app->create();
    }
}

void app_launcher_create(void)
{
    lv_obj_t *scr = ui_screen_create();

    /* Header / title */
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "cocompanion");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_30, 0);
    lv_obj_set_style_text_color(title, UI_TEXT, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 20, 18);

    lv_obj_t *subtitle = lv_label_create(scr);
    lv_label_set_text(subtitle, "ESP32-S3 Touch AMOLED");
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(subtitle, UI_TEXT_DIM, 0);
    lv_obj_align(subtitle, LV_ALIGN_TOP_LEFT, 20, 56);

    /* 2x2 grid of app tiles */
    const int margin = 12;
    const int grid_top = 88;
    const int grid_bottom = 16;
    const int tile_w = (480 - margin * 2 - margin) / 2;
    const int tile_h = (480 - grid_top - grid_bottom - margin) / 2;

    for (int i = 0; i < (int)APP_COUNT; i++) {
        int col = i % 2;
        int row = i / 2;
        int x = margin + col * (tile_w + margin);
        int y = grid_top + row * (tile_h + margin);

        lv_obj_t *tile = lv_button_create(scr);
        lv_obj_set_size(tile, tile_w, tile_h);
        lv_obj_set_pos(tile, x, y);
        lv_obj_set_style_bg_color(tile, UI_CARD, 0);
        lv_obj_set_style_bg_color(tile, UI_CARD2, LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(tile, 20, 0);
        lv_obj_set_style_shadow_width(tile, 0, 0);
        lv_obj_set_style_border_width(tile, 0, 0);
        lv_obj_add_event_cb(tile, tile_cb, LV_EVENT_CLICKED, (void *)&s_apps[i]);

        lv_obj_t *icon = lv_label_create(tile);
        lv_label_set_text(icon, s_apps[i].tag);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_40, 0);
        lv_obj_set_style_text_color(icon, UI_ACCENT, 0);
        lv_obj_align(icon, LV_ALIGN_CENTER, 0, -14);

        lv_obj_t *name = lv_label_create(tile);
        lv_label_set_text(name, s_apps[i].name);
        lv_obj_set_style_text_font(name, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(name, UI_TEXT, 0);
        lv_obj_align(name, LV_ALIGN_CENTER, 0, 26);
    }

    ui_set_launcher_screen(scr);
    lv_screen_load(scr);
}
