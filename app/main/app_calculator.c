#include "ui.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
    double acc;          /* accumulator */
    double current;      /* number being entered */
    char pending_op;     /* '+', '-', '*', '/' or 0 */
    bool fresh;          /* true => next digit starts a new number */
    bool has_current;
    bool error;
} calc_t;

static calc_t s_calc;
static lv_obj_t *s_disp = NULL;   /* main display */
static lv_obj_t *s_op = NULL;     /* pending operator indicator */

static void disp_update(void)
{
    char buf[64];
    if (s_calc.error) {
        lv_label_set_text(s_disp, "Error");
        return;
    }
    double v = s_calc.has_current ? s_calc.current : s_calc.acc;
    if (fabs(v) >= 1e10 || (v != 0.0 && fabs(v) < 1e-9)) {
        snprintf(buf, sizeof(buf), "%.8e", v);
    } else {
        snprintf(buf, sizeof(buf), "%.10g", v);
    }
    lv_label_set_text(s_disp, buf);

    if (s_calc.pending_op != 0) {
        char opbuf[2] = { s_calc.pending_op, 0 };
        lv_label_set_text(s_op, opbuf);
    } else {
        lv_label_set_text(s_op, "");
    }
}

static void calc_clear(void)
{
    s_calc.acc = 0.0;
    s_calc.current = 0.0;
    s_calc.pending_op = 0;
    s_calc.fresh = true;
    s_calc.has_current = false;
    s_calc.error = false;
    disp_update();
}

static void calc_backspace(void)
{
    if (s_calc.error) {
        calc_clear();
        return;
    }
    if (!s_calc.has_current) {
        return;
    }
    /* drop the last digit by string manipulation on the current text */
    char buf[64];
    snprintf(buf, sizeof(buf), "%.10g", s_calc.current);
    size_t len = strlen(buf);
    if (len <= 1) {
        s_calc.current = 0.0;
        s_calc.has_current = false;
    } else {
        buf[len - 1] = '\0';
        s_calc.current = atof(buf);
    }
    disp_update();
}

static void calc_digit(char d)
{
    if (s_calc.error) {
        calc_clear();
    }
    if (s_calc.fresh || !s_calc.has_current) {
        s_calc.current = 0.0;
        s_calc.has_current = false;
        s_calc.fresh = false;
    }
    /* append digit via string to avoid float precision issues */
    char buf[64];
    if (!s_calc.has_current) {
        buf[0] = d;
        buf[1] = '\0';
    } else {
        snprintf(buf, sizeof(buf), "%.10g", s_calc.current);
        size_t len = strlen(buf);
        if (len < sizeof(buf) - 2) {
            buf[len] = d;
            buf[len + 1] = '\0';
        }
    }
    s_calc.current = atof(buf);
    s_calc.has_current = true;
    disp_update();
}

static void calc_dot(void)
{
    if (s_calc.error) {
        calc_clear();
    }
    if (s_calc.fresh || !s_calc.has_current) {
        s_calc.current = 0.0;
        s_calc.fresh = false;
    }
    char buf[64];
    if (!s_calc.has_current) {
        snprintf(buf, sizeof(buf), "0.");
    } else {
        snprintf(buf, sizeof(buf), "%.10g", s_calc.current);
        if (strchr(buf, '.') == NULL) {
            strncat(buf, ".", sizeof(buf) - strlen(buf) - 1);
        }
    }
    s_calc.current = atof(buf);
    s_calc.has_current = true;
    disp_update();
}

static double calc_apply(double a, double b, char op)
{
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return b == 0.0 ? NAN : a / b;
        default: return b;
    }
}

static void calc_op(char op)
{
    if (s_calc.error) {
        calc_clear();
        return;
    }
    if (s_calc.pending_op != 0 && s_calc.has_current) {
        s_calc.acc = calc_apply(s_calc.acc, s_calc.current, s_calc.pending_op);
        if (isnan(s_calc.acc)) {
            s_calc.error = true;
            disp_update();
            return;
        }
    } else if (s_calc.has_current) {
        s_calc.acc = s_calc.current;
    }
    s_calc.pending_op = op;
    s_calc.fresh = true;
    s_calc.has_current = false;
    disp_update();
}

static void calc_equal(void)
{
    if (s_calc.error) {
        return;
    }
    if (s_calc.pending_op != 0) {
        double rhs = s_calc.has_current ? s_calc.current : s_calc.acc;
        s_calc.acc = calc_apply(s_calc.acc, rhs, s_calc.pending_op);
        if (isnan(s_calc.acc)) {
            s_calc.error = true;
            s_calc.pending_op = 0;
            s_calc.has_current = false;
            disp_update();
            return;
        }
        s_calc.pending_op = 0;
        s_calc.current = s_calc.acc;
        s_calc.has_current = true;
        s_calc.fresh = true;
    }
    disp_update();
}

static void key_cb(lv_event_t *e)
{
    const char *key = (const char *)lv_event_get_user_data(e);
    if (key == NULL) {
        return;
    }
    char c = key[0];
    if (c >= '0' && c <= '9') {
        calc_digit(c);
    } else if (c == '.') {
        calc_dot();
    } else if (c == '+' || c == '-' || c == '*' || c == '/') {
        calc_op(c);
    } else if (c == '=') {
        calc_equal();
    } else if (c == 'C') {
        calc_clear();
    } else if (c == 'D') { /* DEL */
        calc_backspace();
    }
}

static lv_obj_t *make_key(lv_obj_t *grid, const char *text, int col, int row, int cs, int rs)
{
    lv_obj_t *btn = lv_button_create(grid);
    lv_obj_set_style_bg_color(btn, UI_CARD2, 0);
    lv_obj_set_style_bg_color(btn, UI_ACCENT, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn, 14, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, col, cs, LV_GRID_ALIGN_STRETCH, row, rs);
    lv_obj_add_event_cb(btn, key_cb, LV_EVENT_CLICKED, (void *)text);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_center(lbl);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl, UI_TEXT, 0);

    return btn;
}

void app_calculator_create(void)
{
    lv_obj_t *scr = ui_screen_create();
    lv_screen_load(scr);

    ui_header_create(scr, "Calculator");

    /* Display area */
    lv_obj_t *disp_area = lv_obj_create(scr);
    lv_obj_remove_style_all(disp_area);
    lv_obj_set_size(disp_area, LV_PCT(100), 140);
    lv_obj_set_pos(disp_area, 0, 56);
    lv_obj_clear_flag(disp_area, LV_OBJ_FLAG_SCROLLABLE);

    s_op = lv_label_create(disp_area);
    lv_label_set_text(s_op, "");
    lv_obj_set_style_text_font(s_op, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(s_op, UI_ACCENT, 0);
    lv_obj_align(s_op, LV_ALIGN_TOP_RIGHT, -20, 6);

    s_disp = lv_label_create(disp_area);
    lv_label_set_text(s_disp, "0");
    lv_obj_set_style_text_font(s_disp, &lv_font_montserrat_44, 0);
    lv_obj_set_style_text_color(s_disp, UI_TEXT, 0);
    lv_obj_align(s_disp, LV_ALIGN_BOTTOM_RIGHT, -20, -6);
    lv_label_set_long_mode(s_disp, LV_LABEL_LONG_SCROLL_CIRCULAR);

    /* Keypad grid: 4 columns x 5 rows */
    lv_obj_t *grid = lv_obj_create(scr);
    lv_obj_remove_style_all(grid);
    lv_obj_set_pos(grid, 12, 56 + 140 + 6);
    lv_obj_set_size(grid, 480 - 24, 480 - (56 + 140 + 6) - 12);
    lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(grid, 0, 0);

    static lv_coord_t col_dsc[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    static lv_coord_t row_dsc[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);

    make_key(grid, "C",   0, 0, 1, 1);
    make_key(grid, "DEL", 1, 0, 1, 1);
    make_key(grid, "/",   2, 0, 1, 1);
    make_key(grid, "*",   3, 0, 1, 1);

    make_key(grid, "7", 0, 1, 1, 1);
    make_key(grid, "8", 1, 1, 1, 1);
    make_key(grid, "9", 2, 1, 1, 1);
    make_key(grid, "-", 3, 1, 1, 1);

    make_key(grid, "4", 0, 2, 1, 1);
    make_key(grid, "5", 1, 2, 1, 1);
    make_key(grid, "6", 2, 2, 1, 1);
    make_key(grid, "+", 3, 2, 1, 1);

    make_key(grid, "1", 0, 3, 1, 1);
    make_key(grid, "2", 1, 3, 1, 1);
    make_key(grid, "3", 2, 3, 1, 1);

    make_key(grid, "0", 0, 4, 2, 1);
    make_key(grid, ".", 2, 4, 1, 1);

    /* "=" spans rows 3-4 in the last column */
    lv_obj_t *eq = make_key(grid, "=", 3, 3, 1, 2);
    lv_obj_set_style_bg_color(eq, UI_ACCENT, 0);
    lv_obj_t *eql = lv_obj_get_child(eq, 0);
    lv_obj_set_style_text_color(eql, lv_color_hex(0x10101A), 0);

    calc_clear();
}
