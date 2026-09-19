#include "ui.h"
#include "wifi_mgr.h"
#include "bsp/esp-bsp.h"
#include <stdio.h>

static lv_obj_t *s_brightness_val = NULL;
static lv_obj_t *s_wifi_label = NULL;
static lv_timer_t *s_wifi_timer = NULL;
static bool s_backlight_on = true;

static void slider_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int v = lv_slider_get_value(slider);
    bsp_display_brightness_set(v);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", v);
    lv_label_set_text(s_brightness_val, buf);
}

static void backlight_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    s_backlight_on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    if (s_backlight_on) {
        bsp_display_backlight_on();
    } else {
        bsp_display_backlight_off();
    }
}

static void wifi_refresh_cb(lv_timer_t *t)
{
    (void)t;
    if (s_wifi_label == NULL) {
        return;
    }
    wifi_state_t st = wifi_mgr_get_state();
    const char *ip = wifi_mgr_get_ip();
    char buf[64];
    switch (st) {
        case WIFI_CONNECTED:
            snprintf(buf, sizeof(buf), "Connected  %s", ip != NULL ? ip : "");
            break;
        case WIFI_CONNECTING:
            snprintf(buf, sizeof(buf), "Connecting...");
            break;
        default:
            snprintf(buf, sizeof(buf), "Disconnected");
            break;
    }
    lv_label_set_text(s_wifi_label, buf);
}

static lv_obj_t *row_label(lv_obj_t *parent, const char *text, int y)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(l, UI_TEXT_DIM, 0);
    lv_obj_set_pos(l, 16, y);
    return l;
}

void app_settings_create(void)
{
    lv_obj_t *scr = ui_screen_create();
    lv_screen_load(scr);
    ui_header_create(scr, "Settings");

    lv_obj_t *content = lv_obj_create(scr);
    lv_obj_remove_style_all(content);
    lv_obj_set_pos(content, 0, 64);
    lv_obj_set_size(content, LV_PCT(100), 480 - 64);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    int y = 0;

    /* ---- Brightness ---- */
    lv_obj_t *card1 = ui_card_create(content);
    lv_obj_set_size(card1, 480 - 24, 96);
    lv_obj_set_pos(card1, 12, y);

    row_label(card1, "Brightness", 14);
    lv_obj_t *slider = lv_slider_create(card1);
    lv_obj_set_size(slider, 320, 8);
    lv_obj_set_pos(slider, 16, 44);
    lv_slider_set_range(slider, 5, 100);
    lv_slider_set_value(slider, bsp_display_brightness_get(), LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider, UI_CARD2, LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, UI_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, UI_TEXT, LV_PART_KNOB);
    lv_obj_add_event_cb(slider, slider_cb, LV_EVENT_VALUE_CHANGED, NULL);

    s_brightness_val = lv_label_create(card1);
    lv_obj_set_style_text_font(s_brightness_val, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(s_brightness_val, UI_TEXT, 0);
    lv_obj_align(s_brightness_val, LV_ALIGN_RIGHT_MID, -16, 0);

    char bbuf[8];
    snprintf(bbuf, sizeof(bbuf), "%d%%", bsp_display_brightness_get());
    lv_label_set_text(s_brightness_val, bbuf);

    y += 96 + 12;

    /* ---- Backlight ---- */
    lv_obj_t *card2 = ui_card_create(content);
    lv_obj_set_size(card2, 480 - 24, 64);
    lv_obj_set_pos(card2, 12, y);

    lv_obj_t *bl_lbl = lv_label_create(card2);
    lv_label_set_text(bl_lbl, "Backlight");
    lv_obj_set_style_text_font(bl_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(bl_lbl, UI_TEXT_DIM, 0);
    lv_obj_align(bl_lbl, LV_ALIGN_LEFT_MID, 16, 0);

    lv_obj_t *sw = lv_switch_create(card2);
    lv_obj_align(sw, LV_ALIGN_RIGHT_MID, -16, 0);
    lv_obj_set_size(sw, 52, 28);
    if (s_backlight_on) {
        lv_obj_add_state(sw, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(sw, backlight_cb, LV_EVENT_VALUE_CHANGED, NULL);

    y += 64 + 12;

    /* ---- WiFi ---- */
    lv_obj_t *card3 = ui_card_create(content);
    lv_obj_set_size(card3, 480 - 24, 64);
    lv_obj_set_pos(card3, 12, y);

    lv_obj_t *w_lbl = lv_label_create(card3);
    lv_label_set_text(w_lbl, "Wi-Fi");
    lv_obj_set_style_text_font(w_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(w_lbl, UI_TEXT_DIM, 0);
    lv_obj_align(w_lbl, LV_ALIGN_LEFT_MID, 16, 0);

    s_wifi_label = lv_label_create(card3);
    lv_label_set_text(s_wifi_label, "Disconnected");
    lv_obj_set_style_text_font(s_wifi_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_wifi_label, UI_ACCENT, 0);
    lv_obj_align(s_wifi_label, LV_ALIGN_RIGHT_MID, -16, 0);

    y += 64 + 12;

    /* ---- About ---- */
    lv_obj_t *card4 = ui_card_create(content);
    lv_obj_set_size(card4, 480 - 24, 110);
    lv_obj_set_pos(card4, 12, y);

    lv_obj_t *a1 = lv_label_create(card4);
    lv_label_set_text(a1, "cocompanion v0.1.0");
    lv_obj_set_style_text_font(a1, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(a1, UI_TEXT, 0);
    lv_obj_set_pos(a1, 16, 12);

    lv_obj_t *a2 = lv_label_create(card4);
    lv_label_set_text(a2, "Waveshare ESP32-S3 Touch AMOLED 2.16\"\n480x480 CO5300 + CST9217");
    lv_obj_set_style_text_font(a2, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(a2, UI_TEXT_DIM, 0);
    lv_obj_set_pos(a2, 16, 44);

    /* refresh wifi status periodically */
    s_wifi_timer = lv_timer_create(wifi_refresh_cb, 500, NULL);
    wifi_refresh_cb(NULL);
}
