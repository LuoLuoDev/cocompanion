#include "ui.h"
#include <string.h>

static lv_obj_t *s_launcher_scr = NULL;

/* ------------------------------------------------------------------------- */
/* Framework                                                                 */
/* ------------------------------------------------------------------------- */

void ui_init(void)
{
    app_launcher_create();
}

void ui_launcher_show(void)
{
    if (s_launcher_scr != NULL) {
        lv_screen_load(s_launcher_scr);
    }
}

void ui_set_launcher_screen(lv_obj_t *scr)
{
    s_launcher_scr = scr;
}

/* ------------------------------------------------------------------------- */
/* Shared helpers                                                            */
/* ------------------------------------------------------------------------- */

lv_obj_t *ui_screen_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, UI_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    return scr;
}

void ui_set_bg(lv_obj_t *obj, lv_color_t color)
{
    lv_obj_set_style_bg_color(obj, color, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
}

static void header_back_cb(lv_event_t *e)
{
    (void)e;
    ui_launcher_show();
}

lv_obj_t *ui_header_create(lv_obj_t *parent, const char *title)
{
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, LV_PCT(100), 56);
    lv_obj_set_style_bg_color(bar, UI_CARD, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    /* Back button */
    lv_obj_t *back = lv_button_create(bar);
    lv_obj_set_size(back, 44, 44);
    lv_obj_align(back, LV_ALIGN_LEFT_MID, 8, 0);
    lv_obj_set_style_bg_color(back, UI_CARD2, 0);
    lv_obj_set_style_bg_color(back, UI_ACCENT, LV_STATE_PRESSED);
    lv_obj_set_style_radius(back, 12, 0);
    lv_obj_add_event_cb(back, header_back_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_lbl = lv_label_create(back);
    lv_label_set_text(back_lbl, "<");
    lv_obj_center(back_lbl);
    lv_obj_set_style_text_font(back_lbl, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(back_lbl, UI_TEXT, 0);

    /* Title */
    lv_obj_t *lbl = lv_label_create(bar);
    lv_label_set_text(lbl, title);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(lbl, UI_TEXT, 0);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);

    return bar;
}

lv_obj_t *ui_card_create(lv_obj_t *parent)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_remove_style_all(card);
    lv_obj_set_style_bg_color(card, UI_CARD, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_set_style_pad_all(card, 16, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}
