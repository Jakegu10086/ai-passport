// main/app_physics.c —— “物理题”三键答题应用。
//
// 画面:首页 → 答题(题干 + 四个选项)→ 解析反馈 → 本轮成绩。
// 按键:上/下切换选项,OK 确认/下一题,长按 OK 返回首页。
// 界面为本应用独立设计:深色卡片 + 青蓝强调色,不复用基线测试菜单。
#include "app_physics.h"

#include "quiz_model.h"
#include "bsp_battery.h"
#include "bsp_display.h"
#include "lvgl.h"
#include "esp_random.h"

#include <stdio.h>

LV_FONT_DECLARE(physics_font_16);
LV_FONT_DECLARE(physics_font_22);

#define COLOR_BG     0x0B1220
#define COLOR_CARD   0x141F33
#define COLOR_LINE   0x22304A
#define COLOR_TEXT   0xF1F5F9
#define COLOR_MUTED  0x94A3B8
#define COLOR_ACCENT 0x38BDF8
#define COLOR_AMBER  0xFBBF24
#define COLOR_OK     0x34D399
#define COLOR_ERR    0xFB7185
#define COLOR_INK    0x061420

#define OPTION_TOP 160
#define OPTION_STEP 38
#define OPTION_H 35

typedef enum {
    SCREEN_HOME = 0,
    SCREEN_QUIZ,
    SCREEN_RESULT,
} screen_id_t;

static screen_id_t s_screen = SCREEN_HOME;
static lv_obj_t *s_scr;
static quiz_session_t s_session;

static lv_obj_t *s_index_label;
static lv_obj_t *s_topic_label;
static lv_obj_t *s_prompt_label;
static lv_obj_t *s_progress_fill;
static lv_obj_t *s_opt_card[QUIZ_OPTION_COUNT];
static lv_obj_t *s_opt_text[QUIZ_OPTION_COUNT];
static lv_obj_t *s_feedback_box;
static lv_obj_t *s_feedback_title;
static lv_obj_t *s_feedback_hint;
static lv_obj_t *s_feedback_answer;
static lv_obj_t *s_feedback_detail;

static lv_obj_t *make_rect(lv_obj_t *parent, int x, int y, int w, int h,
                           int radius, uint32_t bg, uint32_t border, int border_w) {
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(bg), 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(border), 0);
    lv_obj_set_style_border_width(obj, border_w, 0);
    return obj;
}

static lv_obj_t *make_label(lv_obj_t *parent, const lv_font_t *font,
                            uint32_t color, const char *text) {
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    return label;
}

static lv_obj_t *screen_create(void) {
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, lv_color_hex(COLOR_BG), 0);
    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    return scr;
}

static void screen_show(lv_obj_t *next) {
    lv_obj_t *old = s_scr;
    s_scr = next;
    lv_screen_load(next);
    if (old) lv_obj_delete(old);
}

static lv_obj_t *add_battery(lv_obj_t *scr) {
    char buf[12];
    int soc = bsp_battery_soc();
    if (soc < 0) snprintf(buf, sizeof(buf), "--");
    else snprintf(buf, sizeof(buf), "%d%%", soc);
    lv_obj_t *label = make_label(scr, &physics_font_16, COLOR_MUTED, buf);
    lv_obj_align(label, LV_ALIGN_TOP_RIGHT, -16, 8);
    return label;
}

// ---------------------------------------------------------------------------
// 首页
// ---------------------------------------------------------------------------
static void show_home(void) {
    lv_obj_t *scr = screen_create();

    lv_obj_t *title = make_label(scr, &physics_font_22, COLOR_TEXT, "物理题");
    lv_obj_set_pos(title, 16, 16);
    lv_obj_t *subtitle = make_label(scr, &physics_font_16, COLOR_MUTED, "离线物理选择题");
    lv_obj_set_pos(subtitle, 16, 48);
    add_battery(scr);

    lv_obj_t *hero = make_rect(scr, 12, 86, 216, 102, 18, COLOR_CARD, COLOR_LINE, 1);
    lv_obj_t *hero_title = make_label(hero, &physics_font_22, COLOR_ACCENT, "每轮 10 题");
    lv_obj_set_pos(hero_title, 16, 14);
    int bank_total = 0;
    (void)quiz_bank(&bank_total);
    char bank_buf[24];
    snprintf(bank_buf, sizeof(bank_buf), "题库 %d 道", bank_total);
    lv_obj_t *hero_note = make_label(hero, &physics_font_16, COLOR_MUTED, bank_buf);
    lv_obj_set_pos(hero_note, 16, 50);
    lv_obj_t *hero_note2 = make_label(hero, &physics_font_16, COLOR_MUTED, "上下键选择");
    lv_obj_set_pos(hero_note2, 16, 72);

    // 右侧原子装饰:圆环 + 中心点,呼应物理主题。
    lv_obj_t *ring = make_rect(hero, 150, 18, 50, 50, 25, COLOR_CARD, COLOR_ACCENT, 2);
    lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, 0);
    lv_obj_t *dot = make_rect(ring, 16, 16, 18, 18, 9, COLOR_AMBER, COLOR_AMBER, 0);
    (void)dot;
    lv_obj_t *orbit = make_rect(hero, 162, 34, 26, 18, 9, COLOR_ACCENT, COLOR_ACCENT, 0);
    lv_obj_set_style_bg_opa(orbit, LV_OPA_30, 0);
    (void)orbit;

    lv_obj_t *start = make_rect(scr, 12, 204, 216, 50, 14, COLOR_ACCENT, COLOR_ACCENT, 0);
    lv_obj_t *start_label = make_label(start, &physics_font_22, COLOR_INK, "开始答题");
    lv_obj_center(start_label);

    lv_obj_t *hint = make_label(scr, &physics_font_16, COLOR_MUTED, "按 OK 开始");
    lv_obj_set_width(hint, 216);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(hint, 12, 270);

    s_screen = SCREEN_HOME;
    screen_show(scr);
}

// ---------------------------------------------------------------------------
// 答题页
// ---------------------------------------------------------------------------
static void apply_option_style(int i, int mode) {
    // mode: 0 普通, 1 选中, 2 正确, 3 错误
    uint32_t bg = COLOR_CARD;
    uint32_t border = COLOR_LINE;
    if (mode == 1) {
        bg = 0x17324A;
        border = COLOR_ACCENT;
    } else if (mode == 2) {
        bg = 0x12352B;
        border = COLOR_OK;
    } else if (mode == 3) {
        bg = 0x3A1A22;
        border = COLOR_ERR;
    }
    lv_obj_set_style_bg_color(s_opt_card[i], lv_color_hex(bg), 0);
    lv_obj_set_style_border_color(s_opt_card[i], lv_color_hex(border), 0);
}

static void quiz_refresh_question(void) {
    const quiz_question_t *q = quiz_session_question(&s_session);
    if (!q) return;

    char buf[32];
    snprintf(buf, sizeof(buf), "第 %d/%d 题", s_session.index + 1, s_session.count);
    lv_label_set_text(s_index_label, buf);
    lv_label_set_text(s_topic_label, q->topic);
    lv_label_set_text(s_prompt_label, q->prompt);

    int fill_w = 224 * (s_session.index + 1) / (s_session.count > 0 ? s_session.count : 1);
    lv_obj_set_width(s_progress_fill, fill_w);

    for (int i = 0; i < QUIZ_OPTION_COUNT; i++) {
        lv_label_set_text(s_opt_text[i], q->options[i]);
        apply_option_style(i, i == s_session.selection ? 1 : 0);
    }

    lv_obj_add_flag(s_feedback_box, LV_OBJ_FLAG_HIDDEN);
    for (int i = 0; i < QUIZ_OPTION_COUNT; i++) {
        lv_obj_remove_flag(s_opt_card[i], LV_OBJ_FLAG_HIDDEN);
    }
}

static void quiz_apply_selection(void) {
    for (int i = 0; i < QUIZ_OPTION_COUNT; i++) {
        apply_option_style(i, i == s_session.selection ? 1 : 0);
    }
}

static void quiz_show_feedback(void) {
    const quiz_question_t *q = quiz_session_question(&s_session);
    if (!q) return;

    for (int i = 0; i < QUIZ_OPTION_COUNT; i++) {
        lv_obj_add_flag(s_opt_card[i], LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_remove_flag(s_feedback_box, LV_OBJ_FLAG_HIDDEN);

    char buf[112];
    if (s_session.last_correct) {
        lv_label_set_text(s_feedback_title, "回答正确");
        lv_obj_set_style_text_color(s_feedback_title, lv_color_hex(COLOR_OK), 0);
    } else {
        lv_label_set_text(s_feedback_title, "回答错误");
        lv_obj_set_style_text_color(s_feedback_title, lv_color_hex(COLOR_ERR), 0);
    }

    static const char kLetters[] = "ABCD";
    snprintf(buf, sizeof(buf), "正确答案：%c. %s", kLetters[q->answer],
             q->options[q->answer]);
    lv_label_set_text(s_feedback_answer, buf);
    lv_label_set_text(s_feedback_detail, q->explanation);
    lv_label_set_text(s_feedback_hint, s_session.index + 1 >= s_session.count
                                           ? "OK 看成绩"
                                           : "OK 下一题");
}

static void show_quiz(void) {
    lv_obj_t *scr = screen_create();

    lv_obj_t *kicker = make_label(scr, &physics_font_16, COLOR_ACCENT, "物理题");
    lv_obj_set_pos(kicker, 16, 6);
    add_battery(scr);

    s_index_label = make_label(scr, &physics_font_16, COLOR_MUTED, "");
    lv_obj_set_pos(s_index_label, 16, 30);

    lv_obj_t *chip = make_rect(scr, 152, 28, 72, 22, 11, 0x1B2A40, 0x1B2A40, 0);
    s_topic_label = make_label(chip, &physics_font_16, COLOR_ACCENT, "");
    lv_obj_set_width(s_topic_label, 72);
    lv_obj_set_style_text_align(s_topic_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(s_topic_label, LV_ALIGN_CENTER, 0, 0);

    make_rect(scr, 8, 54, 224, 5, 2, COLOR_LINE, COLOR_LINE, 0);
    s_progress_fill = make_rect(scr, 8, 54, 22, 5, 2, COLOR_ACCENT, COLOR_ACCENT, 0);

    lv_obj_t *prompt_card = make_rect(scr, 8, 64, 224, 92, 14, COLOR_CARD, COLOR_LINE, 1);
    s_prompt_label = make_label(prompt_card, &physics_font_16, COLOR_TEXT, "");
    lv_obj_set_width(s_prompt_label, 208);
    lv_label_set_long_mode(s_prompt_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_line_space(s_prompt_label, 0, 0);
    lv_obj_set_pos(s_prompt_label, 8, 6);

    static const char kLetters[] = "ABCD";
    for (int i = 0; i < QUIZ_OPTION_COUNT; i++) {
        lv_obj_t *card = make_rect(scr, 8, OPTION_TOP + i * OPTION_STEP, 224, OPTION_H,
                                   10, COLOR_CARD, COLOR_LINE, 2);
        s_opt_card[i] = card;
        char letter[3] = { kLetters[i], '.', '\0' };
        lv_obj_t *letter_label = make_label(card, &physics_font_16, COLOR_ACCENT, letter);
        lv_obj_align(letter_label, LV_ALIGN_LEFT_MID, 8, 0);
        s_opt_text[i] = make_label(card, &physics_font_16, COLOR_TEXT, "");
        lv_obj_set_width(s_opt_text[i], 190);
        lv_label_set_long_mode(s_opt_text[i], LV_LABEL_LONG_DOT);
        lv_obj_align(s_opt_text[i], LV_ALIGN_LEFT_MID, 28, 0);
    }

    s_feedback_box = make_rect(scr, 8, OPTION_TOP - 2, 224, 152, 14, COLOR_CARD, COLOR_LINE, 1);
    s_feedback_title = make_label(s_feedback_box, &physics_font_22, COLOR_OK, "");
    lv_obj_set_pos(s_feedback_title, 14, 6);
    s_feedback_hint = make_label(s_feedback_box, &physics_font_16, COLOR_ACCENT, "");
    lv_obj_align(s_feedback_hint, LV_ALIGN_TOP_RIGHT, -14, 10);
    s_feedback_answer = make_label(s_feedback_box, &physics_font_16, COLOR_TEXT, "");
    lv_obj_set_width(s_feedback_answer, 196);
    lv_label_set_long_mode(s_feedback_answer, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(s_feedback_answer, 14, 38);
    s_feedback_detail = make_label(s_feedback_box, &physics_font_16, COLOR_MUTED, "");
    lv_obj_set_width(s_feedback_detail, 196);
    lv_label_set_long_mode(s_feedback_detail, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_line_space(s_feedback_detail, 1, 0);
    lv_obj_set_pos(s_feedback_detail, 14, 84);
    lv_obj_add_flag(s_feedback_box, LV_OBJ_FLAG_HIDDEN);

    s_screen = SCREEN_QUIZ;
    screen_show(scr);
    quiz_refresh_question();
}

// ---------------------------------------------------------------------------
// 成绩页
// ---------------------------------------------------------------------------
static void show_result(void) {
    lv_obj_t *scr = screen_create();

    lv_obj_t *title = make_label(scr, &physics_font_22, COLOR_TEXT, "本轮结束");
    lv_obj_set_pos(title, 16, 16);
    add_battery(scr);

    lv_obj_t *card = make_rect(scr, 12, 68, 216, 138, 18, COLOR_CARD, COLOR_LINE, 1);
    char buf[64];
    snprintf(buf, sizeof(buf), "%d / %d", s_session.score, s_session.count);
    lv_obj_t *score = make_label(card, &physics_font_22, COLOR_ACCENT, buf);
    lv_obj_set_width(score, 216);
    lv_obj_set_style_text_align(score, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(score, 0, 20);

    snprintf(buf, sizeof(buf), "正确率 %d%%", quiz_session_accuracy(&s_session));
    lv_obj_t *accuracy = make_label(card, &physics_font_16, COLOR_TEXT, buf);
    lv_obj_set_width(accuracy, 216);
    lv_obj_set_style_text_align(accuracy, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(accuracy, 0, 56);

    const char *message;
    if (s_session.score >= 9) message = "太棒了，物理直觉在线！";
    else if (s_session.score >= 6) message = "不错，再练一轮会更稳。";
    else message = "别急，回到概念再试一次。";
    lv_obj_t *tail = make_label(card, &physics_font_16, COLOR_MUTED, message);
    lv_obj_set_width(tail, 192);
    lv_obj_set_style_text_align(tail, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(tail, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(tail, 12, 90);

    lv_obj_t *again = make_rect(scr, 12, 224, 216, 46, 13, COLOR_AMBER, COLOR_AMBER, 0);
    lv_obj_t *again_label = make_label(again, &physics_font_22, COLOR_INK, "再来一轮");
    lv_obj_center(again_label);

    lv_obj_t *hint = make_label(scr, &physics_font_16, COLOR_MUTED, "长按 OK 返回首页");
    lv_obj_set_width(hint, 216);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(hint, 12, 280);

    s_screen = SCREEN_RESULT;
    screen_show(scr);
}

// ---------------------------------------------------------------------------
// 流程控制
// ---------------------------------------------------------------------------
static void start_round(void) {
    int bank_count = 0;
    (void)quiz_bank(&bank_count);
    int order[QUIZ_ROUND_SIZE];
    quiz_build_order(order, bank_count, QUIZ_ROUND_SIZE, esp_random());
    quiz_session_start(&s_session, order, QUIZ_ROUND_SIZE);
    show_quiz();
}

void app_physics_start(void) {
    if (!bsp_lvgl_lock(1000)) return;
    show_home();
    bsp_lvgl_unlock();
}

void app_physics_handle_key(bsp_btn_t btn, bsp_btn_ev_t ev) {
    if (!bsp_lvgl_lock(500)) return;

    if (ev == BSP_BTN_LONG) {
        if (btn == BSP_BTN_OK && s_screen != SCREEN_HOME) {
            show_home();
        }
        bsp_lvgl_unlock();
        return;
    }

    if (ev != BSP_BTN_CLICK) {
        bsp_lvgl_unlock();
        return;
    }

    switch (s_screen) {
    case SCREEN_HOME:
        if (btn == BSP_BTN_OK) start_round();
        break;
    case SCREEN_QUIZ:
        if (btn == BSP_BTN_UP || btn == BSP_BTN_DOWN) {
            quiz_session_move(&s_session, btn == BSP_BTN_UP ? -1 : 1);
            quiz_apply_selection();
        } else if (btn == BSP_BTN_OK) {
            quiz_event_t event = quiz_session_confirm(&s_session);
            if (event == QUIZ_EVENT_ANSWERED) quiz_show_feedback();
            else if (event == QUIZ_EVENT_ADVANCED) quiz_refresh_question();
            else if (event == QUIZ_EVENT_FINISHED) show_result();
        }
        break;
    case SCREEN_RESULT:
        if (btn == BSP_BTN_OK) start_round();
        break;
    default:
        break;
    }

    bsp_lvgl_unlock();
}
