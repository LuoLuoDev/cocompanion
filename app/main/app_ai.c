#include "ui.h"
#include "ai_client.h"
#include "bsp/esp-bsp.h"
#include <string.h>
#include "freertos/FreeRTOS.h"

static lv_obj_t *s_chat = NULL;
static lv_obj_t *s_status = NULL;
static lv_coord_t s_cursor_y = 0;

static void set_status(const char *text)
{
    lv_label_set_text(s_status, text);
}

static void add_bubble(const char *text, bool user)
{
    lv_obj_t *b = lv_obj_create(s_chat);
    lv_obj_remove_style_all(b);
    lv_obj_set_style_bg_color(b, user ? UI_ACCENT : UI_CARD2, 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(b, 14, 0);
    lv_obj_set_style_pad_all(b, 10, 0);
    lv_obj_set_width(b, 312);

    lv_obj_t *lbl = lv_label_create(b);
    lv_label_set_text(lbl, text);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl, 292);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl, user ? lv_color_hex(0x10101A) : UI_TEXT, 0);

    lv_obj_update_layout(b);
    lv_coord_t lh = lv_obj_get_height(lbl);
    lv_obj_set_height(b, lh + 20);

    lv_obj_set_pos(b, user ? (480 - 312 - 12) : 12, s_cursor_y);
    s_cursor_y += lh + 20 + 8;

    lv_obj_scroll_to_view(b, LV_ANIM_ON);
}

static void reply_cb(const char *reply, bool ok, void *user_data)
{
    (void)user_data;
    (void)ok;
    if (bsp_display_lock(portMAX_DELAY)) {
        add_bubble(reply, false);
        set_status("");
        bsp_display_unlock();
    }
}

static void ask(const char *question)
{
    add_bubble(question, true);
    set_status("Thinking...");
    ai_client_ask(question, reply_cb, NULL);
}

static void preset_cb(lv_event_t *e)
{
    const char *q = (const char *)lv_event_get_user_data(e);
    if (q != NULL) {
        ask(q);
    }
}

static lv_obj_t *make_preset(lv_obj_t *parent, const char *text, int col, int row)
{
    int bw = (480 - 24 - 8) / 2;
    lv_obj_t *b = lv_button_create(parent);
    lv_obj_set_size(b, bw, 44);
    lv_obj_set_pos(b, 12 + col * (bw + 8), 6 + row * (44 + 6));
    lv_obj_set_style_bg_color(b, UI_CARD, 0);
    lv_obj_set_style_bg_color(b, UI_ACCENT, LV_STATE_PRESSED);
    lv_obj_set_style_radius(b, 12, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_set_style_border_width(b, 0, 0);
    lv_obj_add_event_cb(b, preset_cb, LV_EVENT_CLICKED, (void *)text);

    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, text);
    lv_obj_center(l);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(l, UI_TEXT, 0);
    return b;
}

void app_ai_create(void)
{
    lv_obj_t *scr = ui_screen_create();
    lv_screen_load(scr);
    ui_header_create(scr, "AI Chat");

    /* status line */
    s_status = lv_label_create(scr);
    lv_label_set_text(s_status, "");
    lv_obj_set_style_text_font(s_status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_status, UI_ACCENT2, 0);
    lv_obj_set_pos(s_status, 16, 64);

    /* chat area */
    s_chat = lv_obj_create(scr);
    lv_obj_remove_style_all(s_chat);
    lv_obj_set_pos(s_chat, 0, 84);
    lv_obj_set_size(s_chat, LV_PCT(100), 480 - 84 - 112);
    lv_obj_set_style_pad_all(s_chat, 0, 0);
    lv_obj_add_flag(s_chat, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(s_chat, LV_SCROLLBAR_MODE_OFF);

    /* preset buttons */
    lv_obj_t *presets = lv_obj_create(scr);
    lv_obj_remove_style_all(presets);
    lv_obj_set_pos(presets, 0, 480 - 112);
    lv_obj_set_size(presets, LV_PCT(100), 112);
    lv_obj_clear_flag(presets, LV_OBJ_FLAG_SCROLLABLE);

    make_preset(presets, "Tell me a joke", 0, 0);
    make_preset(presets, "Explain AI simply", 1, 0);
    make_preset(presets, "Give me a tip", 0, 1);
    make_preset(presets, "Write a haiku", 1, 1);

    /* welcome */
    s_cursor_y = 4;
    add_bubble("Hi! I'm your AI companion.\nTap a question below to chat.", false);

    if (!ai_client_configured()) {
        set_status("API key not set");
    }
}
