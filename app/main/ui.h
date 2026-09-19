#pragma once

#include "lvgl.h"

/* ---------- Theme colors (dark AMOLED-friendly) ---------- */
#define UI_BG       lv_color_hex(0x10101A)
#define UI_CARD     lv_color_hex(0x1E1E2E)
#define UI_CARD2    lv_color_hex(0x2A2A3E)
#define UI_ACCENT   lv_color_hex(0x89B4FA)
#define UI_ACCENT2  lv_color_hex(0xA6E3A1)
#define UI_TEXT     lv_color_hex(0xCDD6F4)
#define UI_TEXT_DIM lv_color_hex(0x9399B2)
#define UI_DANGER   lv_color_hex(0xF38BA8)
#define UI_WARN     lv_color_hex(0xF9E2AF)

/* ---------- App abstraction ---------- */
typedef struct ui_app ui_app_t;

typedef void (*ui_app_create_cb)(void);

struct ui_app {
    const char *name;
    const char *tag;      /* short icon label shown on the launcher tile */
    ui_app_create_cb create;
};

/* ---------- Framework ---------- */
void ui_init(void);
void ui_launcher_show(void);
void ui_set_launcher_screen(lv_obj_t *scr);

/* ---------- Shared helpers ---------- */
lv_obj_t *ui_screen_create(void);                       /* blank dark screen (auto-loaded) */
lv_obj_t *ui_header_create(lv_obj_t *parent, const char *title); /* top bar with back button */
lv_obj_t *ui_card_create(lv_obj_t *parent);             /* rounded card container */
void ui_set_bg(lv_obj_t *obj, lv_color_t color);        /* solid background on an object */

/* ---------- App entry points ---------- */
void app_launcher_create(void);
void app_calculator_create(void);
void app_clock_create(void);
void app_settings_create(void);
void app_ai_create(void);
