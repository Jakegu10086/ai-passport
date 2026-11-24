// main/quiz_model.h —— “物理题”答题会话的纯逻辑模型(不依赖 LVGL/ESP-IDF,便于 host 测试)。
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define QUIZ_OPTION_COUNT 4
#define QUIZ_ROUND_SIZE 10
#define QUIZ_BANK_MAX 96

typedef struct {
    const char *topic;                       // 力学 / 电学 / 光学 ...
    const char *prompt;                      // 题干
    const char *options[QUIZ_OPTION_COUNT];  // 四个选项
    uint8_t answer;                          // 正确选项下标 0..3
    const char *explanation;                 // 一句话解析
} quiz_question_t;

typedef struct {
    int order[QUIZ_BANK_MAX];  // 本轮抽到的题目在题库中的下标
    int count;                 // 本轮题数
    int index;                 // 当前第几题(0 起)
    int selection;             // 当前选项
    int score;                 // 已答对题数
    bool answered;             // 当前题是否已作答
    bool finished;             // 本轮是否结束
    bool last_correct;         // 最近一题是否正确
} quiz_session_t;

typedef enum {
    QUIZ_EVENT_NONE = 0,
    QUIZ_EVENT_MOVED,     // 选项移动
    QUIZ_EVENT_ANSWERED,  // 已判分,等待进入下一题
    QUIZ_EVENT_ADVANCED,  // 已进入下一题
    QUIZ_EVENT_FINISHED,  // 本轮结束
} quiz_event_t;

// 题库与题数。
const quiz_question_t *quiz_bank(int *count);

// 用 Fisher-Yates 从题库中抽取 round_size 道题,写入 order(需至少 round_size 空间)。
// 同一 seed 结果确定,便于测试;越界参数会被收敛到合法范围。
void quiz_build_order(int *order, int bank_count, int round_size, uint32_t seed);

// 开始一轮答题。order 会被复制,count 超过题库上限时截断。
void quiz_session_start(quiz_session_t *s, const int *order, int count);

// 上下键移动选项(仅在未作答且未结束时生效),首尾循环。
void quiz_session_move(quiz_session_t *s, int delta);

// 确定键:未作答时判分;已作答时进入下一题或结束本轮。
quiz_event_t quiz_session_confirm(quiz_session_t *s);

// 当前题目及正确率(0..100,用于结果页)。
const quiz_question_t *quiz_session_question(const quiz_session_t *s);
int quiz_session_accuracy(const quiz_session_t *s);
