#include "ui.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef enum { MODE_CLOCK = 0, MODE_STOPWATCH, MODE_TIMER } clock_mode_t;

static lv_obj_t *s_content = NULL;
static lv_obj_t *s_big = NULL;      /* big time label */
static lv_obj_t *s_sub = NULL;      /* date / status label */
static lv_obj_t *s_btn_primary = NULL;
static lv_obj_t *s_btn_secondary = NULL;

static clock_mode_t s_mode = MODE_CLOCK;

static uint32_t s_last_tick = 0;

/* stopwatch */
static uint32_t s_sw_ms = 0;
static bool s_sw_running = false;

/* countdown */
static uint32_t s_timer_total_ms = 0;
static uint32_t s_timer_remaining_ms = 0;
static bool s_timer_running = false;

/* ------------------------------------------------------------------------- */

static void fmt_hms(uint32_t ms, char *buf, size_t n, bool with_cs)
{
    uint32_t s = ms / 1000;
    uint32_t m = s / 60;
    uint32_t h = m / 60;
    if (h > 0) {
        snprintf(buf, n, "%lu:%02lu:%02lu", h, m % 60, s % 60);
    } else if (with_cs) {
        snprintf(buf, n, "%02lu:%02lu.%02lu", m % 60, s % 60, (ms % 1000) / 10);
    } else {
        snprintf(buf, n, "%02lu:%02lu", m % 60, s % 60);
    }
}

static void refresh_ui(void)
{
    char buf[32];
    switch (s_mode) {
        case MODE_CLOCK: {
            time_t now = time(NULL);
            struct tm t;
            localtime_r(&now, &t);
            snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
            lv_label_set_text(s_big, buf);
            snprintf(buf, sizeof(buf), "%04d-%02d-%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
            lv_label_set_text(s_sub, buf);
            break;
        }
        case MODE_STOPWATCH:
            fmt_hms(s_sw_ms, buf, sizeof(buf), true);
            lv_label_set_text(s_big, buf);
            lv_label_set_text(s_sub, s_sw_running ? "Running" : "Stopped");
            lv_label_set_text(s_btn_primary, s_sw_running ? "Pause" : "Start");
            break;
        case MODE_TIMER:
            fmt_hms(s_timer_remaining_ms, buf, sizeof(buf), false);
            lv_label_set_text(s_big, buf);
            if (s_timer_remaining_ms == 0 && s_timer_total_ms > 0 && !s_timer_running) {
                lv_label_set_text(s_sub, "Time's up!");
            } else {
                lv_label_set_text(s_sub, s_timer_running ? "Counting down" : "Ready");
            }
            lv_label_set_text(s_btn_primary, s_timer_running ? "Pause" : "Start");
            break;
    }
}

static void tick_cb(lv_timer_t *t)
{
    uint32_t now = lv_tick_get();
    uint32_t delta = now - s_last_tick;
    s_last_tick = now;

    if (s_mode == MODE_STOPWATCH && s_sw_running) {
        s_sw_ms += delta;
    } else if (s_mode == MODE_TIMER && s_timer_running) {
        if (delta >= s_timer_remaining_ms) {
            s_timer_remaining_ms = 0;
            s_timer_running = false;
        } else {
            s_timer_remaining_ms -= delta;
        }
    }

    refresh_ui();
}

/* ------------------------------------------------------------------------- */

static void btn_primary_cb(lv_event_t *e)
{
    (void)e;
    if (s_mode == MODE_STOPWATCH) {
        s_sw_running = !s_sw_running;
    } else if (s_mode == MODE_TIMER) {
        if (!s_timer_running && s_timer_remaining_ms == 0 && s_timer_total_ms > 0) {
            s_timer_remaining_ms = s_timer_total_ms;
        }
        if (s_timer_remaining_ms > 0) {
            s_timer_running = !s_timer_running;
        }
    }
    refresh_ui();
}

static void btn_secondary_cb(lv_event_t *e)
{
    (void)e;
    if (s_mode == MODE_STOPWATCH) {
        s_sw_ms = 0;
        s_sw_running = false;
    } else if (s_mode == MODE_TIMER) {
        s_timer_remaining_ms = 0;
        s_timer_total_ms = 0;
        s_timer_running = false;
    }
    refresh_ui();
}

static void preset_cb(lv_event_t *e)
{
    uint32_t mins = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
    s_timer_total_ms = mins * 60000;
    s_timer_remaining_ms = s_timer_total_ms;
    s_timer_running = true;
    refresh_ui();
}

/* ------------------------------------------------------------------------- */

static void tab_cb(lv_event_t *e);

static void add_action_buttons(lv_obj_t *parent, bool show_presets)
{
    if (show_presets) {
        const uint32_t presets[] = { 1, 5, 10 };
        const char *labels[] = { "1 min", "5 min", "10 min" };
        int n = 3;
        int bw = (480 - 24 - 8 * (n - 1)) / n;
        for (int i = 0; i < n; i++) {
            lv_obj_t *b = lv_button_create(parent);
            lv_obj_set_size(b, bw, 52);
            lv_obj_set_pos(b, 12 + i * (bw + 8), 0);
            lv_obj_set_style_bg_color(b, UI_CARD2, 0);
            lv_obj_set_style_bg_color(b, UI_ACCENT, LV_STATE_PRESSED);
            lv_obj_set_style_radius(b, 12, 0);
            lv_obj_set_style_shadow_width(b, 0, 0);
            lv_obj_set_style_border_width(b, 0, 0);
            lv_obj_add_event_cb(b, preset_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)presets[i]);
            lv_obj_t *l = lv_label_create(b);
            lv_label_set_text(l, labels[i]);
            lv_obj_center(l);
            lv_obj_set_style_text_font(l, &lv_font_montserrat_18, 0);
            lv_obj_set_style_text_color(l, UI_TEXT, 0);
        }
        return;
    }

    int bw = (480 - 24 - 8) / 2;
    s_btn_primary = lv_button_create(parent);
    lv_obj_set_size(s_btn_primary, bw, 52);
    lv_obj_set_pos(s_btn_primary, 12, 0);
    lv_obj_set_style_bg_color(s_btn_primary, UI_ACCENT, 0);
    lv_obj_set_style_radius(s_btn_primary, 12, 0);
    lv_obj_set_style_shadow_width(s_btn_primary, 0, 0);
    lv_obj_set_style_border_width(s_btn_primary, 0, 0);
    lv_obj_add_event_cb(s_btn_primary, btn_primary_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *pl = lv_label_create(s_btn_primary);
    lv_label_set_text(pl, "Start");
    lv_obj_center(pl);
    lv_obj_set_style_text_font(pl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(pl, lv_color_hex(0x10101A), 0);

    s_btn_secondary = lv_button_create(parent);
    lv_obj_set_size(s_btn_secondary, bw, 52);
    lv_obj_set_pos(s_btn_secondary, 12 + bw + 8, 0);
    lv_obj_set_style_bg_color(s_btn_secondary, UI_DANGER, 0);
    lv_obj_set_style_radius(s_btn_secondary, 12, 0);
    lv_obj_set_style_shadow_width(s_btn_secondary, 0, 0);
    lv_obj_set_style_border_width(s_btn_secondary, 0, 0);
    lv_obj_add_event_cb(s_btn_secondary, btn_secondary_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *sl = lv_label_create(s_btn_secondary);
    lv_label_set_text(sl, "Reset");
    lv_obj_center(sl);
    lv_obj_set_style_text_font(sl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(sl, lv_color_hex(0x10101A), 0);
}

static void rebuild_content(void)
{
    if (s_content != NULL) {
        lv_obj_delete(s_content);
    }

    s_content = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(s_content);
    lv_obj_set_size(s_content, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(s_content, 0, 0);
    lv_obj_clear_flag(s_content, LV_OBJ_FLAG_SCROLLABLE);

    /* tab bar */
    static const char *tabs[] = { "Clock", "Stopwatch", "Timer" };
    int n = 3;
    int bw = (480 - 24 - 8 * (n - 1)) / n;
    for (int i = 0; i < n; i++) {
        lv_obj_t *t = lv_button_create(s_content);
        lv_obj_set_size(t, bw, 40);
        lv_obj_set_pos(t, 12 + i * (bw + 8), 68);
        lv_obj_set_style_radius(t, 10, 0);
        lv_obj_set_style_shadow_width(t, 0, 0);
        lv_obj_set_style_border_width(t, 0, 0);
        if ((int)s_mode == i) {
            lv_obj_set_style_bg_color(t, UI_ACCENT, 0);
        } else {
            lv_obj_set_style_bg_color(t, UI_CARD2, 0);
        }
        lv_obj_add_event_cb(t, tab_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        lv_obj_t *l = lv_label_create(t);
        lv_label_set_text(l, tabs[i]);
        lv_obj_center(l);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(l, UI_TEXT, 0);
    }

    /* big time label */
    s_big = lv_label_create(s_content);
    lv_label_set_text(s_big, "--:--:--");
    lv_obj_set_style_text_font(s_big, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(s_big, UI_TEXT, 0);
    lv_obj_align(s_big, LV_ALIGN_CENTER, 0, -10);

    s_sub = lv_label_create(s_content);
    lv_label_set_text(s_sub, "");
    lv_obj_set_style_text_font(s_sub, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(s_sub, UI_TEXT_DIM, 0);
    lv_obj_align(s_sub, LV_ALIGN_CENTER, 0, 46);

    /* action buttons */
    lv_obj_t *actions = lv_obj_create(s_content);
    lv_obj_remove_style_all(actions);
    lv_obj_set_size(actions, LV_PCT(100), 52);
    lv_obj_align(actions, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_clear_flag(actions, LV_OBJ_FLAG_SCROLLABLE);

    if (s_mode == MODE_CLOCK) {
        /* no action buttons for the clock */
    } else if (s_mode == MODE_TIMER && !s_timer_running && s_timer_total_ms == 0) {
        add_action_buttons(actions, true);
    } else {
        add_action_buttons(actions, false);
    }

    refresh_ui();
}

static void tab_cb(lv_event_t *e)
{
    clock_mode_t m = (clock_mode_t)(uintptr_t)lv_event_get_user_data(e);
    if (m != s_mode) {
        s_mode = m;
        rebuild_content();
    }
}

void app_clock_create(void)
{
    lv_obj_t *scr = ui_screen_create();
    lv_screen_load(scr);
    ui_header_create(scr, "Clock");

    s_mode = MODE_CLOCK;
    s_sw_ms = 0;
    s_sw_running = false;
    s_timer_total_ms = 0;
    s_timer_remaining_ms = 0;
    s_timer_running = false;

    rebuild_content();

    s_last_tick = lv_tick_get();
    lv_timer_create(tick_cb, 100, NULL);
}
