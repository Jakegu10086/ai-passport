// tests/test_quiz_model.c —— “物理题”会话模型的 host 测试(无 LVGL/ESP-IDF 依赖)。
#include "quiz_model.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

static void test_bank(void) {
    int count = 0;
    const quiz_question_t *bank = quiz_bank(&count);
    assert(bank != NULL);
    assert(count >= QUIZ_ROUND_SIZE);
    assert(count <= QUIZ_BANK_MAX);

    bool seen_answer[QUIZ_OPTION_COUNT] = { false };
    for (int i = 0; i < count; i++) {
        assert(bank[i].topic && bank[i].prompt && bank[i].explanation);
        assert(bank[i].answer < QUIZ_OPTION_COUNT);
        seen_answer[bank[i].answer] = true;
        for (int k = 0; k < QUIZ_OPTION_COUNT; k++) {
            assert(bank[i].options[k] && bank[i].options[k][0] != '\0');
        }
    }
    // 正确选项应覆盖 A/B/C/D,避免可以靠猜同一个字母得满分。
    for (int k = 0; k < QUIZ_OPTION_COUNT; k++) assert(seen_answer[k]);
}

static void test_order(void) {
    int a[QUIZ_ROUND_SIZE];
    int b[QUIZ_ROUND_SIZE];
    quiz_build_order(a, 20, QUIZ_ROUND_SIZE, 1234u);
    quiz_build_order(b, 20, QUIZ_ROUND_SIZE, 1234u);
    for (int i = 0; i < QUIZ_ROUND_SIZE; i++) {
        assert(a[i] == b[i]);
        assert(a[i] >= 0 && a[i] < 20);
        for (int j = 0; j < i; j++) assert(a[j] != a[i]);
    }

    // 轮次大于题库时收敛到题库大小。
    int all[QUIZ_BANK_MAX];
    quiz_build_order(all, 5, QUIZ_BANK_MAX, 9u);
    for (int i = 0; i < 5; i++) {
        assert(all[i] >= 0 && all[i] < 5);
        for (int j = 0; j < i; j++) assert(all[j] != all[i]);
    }
}

static void test_session_scoring(void) {
    int order[QUIZ_ROUND_SIZE];
    for (int i = 0; i < QUIZ_ROUND_SIZE; i++) order[i] = i;

    quiz_session_t s;
    quiz_session_start(&s, order, QUIZ_ROUND_SIZE);
    assert(s.index == 0 && s.score == 0 && !s.answered && !s.finished);

    // 上下键循环。
    quiz_session_move(&s, -1);
    assert(s.selection == QUIZ_OPTION_COUNT - 1);
    quiz_session_move(&s, 1);
    assert(s.selection == 0);

    const quiz_question_t *q = quiz_session_question(&s);
    assert(q != NULL);

    // 选错:不得分,作答后不能再移动。
    s.selection = (q->answer + 1) % QUIZ_OPTION_COUNT;
    assert(quiz_session_confirm(&s) == QUIZ_EVENT_ANSWERED);
    assert(s.answered && !s.last_correct && s.score == 0);
    quiz_session_move(&s, 1);
    assert(s.selection == (int)((q->answer + 1) % QUIZ_OPTION_COUNT));
    assert(quiz_session_confirm(&s) == QUIZ_EVENT_ADVANCED);
    assert(s.index == 1 && !s.answered);

    // 选对:得分。
    q = quiz_session_question(&s);
    s.selection = q->answer;
    assert(quiz_session_confirm(&s) == QUIZ_EVENT_ANSWERED);
    assert(s.last_correct && s.score == 1);

    // 依次再答对剩余题目,最后一题确认后结束。
    quiz_event_t event = QUIZ_EVENT_NONE;
    for (int i = 1; i < QUIZ_ROUND_SIZE; i++) {
        event = quiz_session_confirm(&s);
        if (i + 1 >= QUIZ_ROUND_SIZE) {
            assert(event == QUIZ_EVENT_FINISHED);
        } else {
            assert(event == QUIZ_EVENT_ADVANCED);
            q = quiz_session_question(&s);
            s.selection = q->answer;
            assert(quiz_session_confirm(&s) == QUIZ_EVENT_ANSWERED);
        }
    }
    assert(s.finished);
    assert(s.score == QUIZ_ROUND_SIZE - 1);  // 首题故意答错,其余全对。
    assert(s.finished);
    assert(quiz_session_question(&s) == NULL);
    assert(quiz_session_confirm(&s) == QUIZ_EVENT_NONE);
}

static void test_accuracy(void) {
    quiz_session_t s;
    int order[4] = { 0, 1, 2, 3 };
    quiz_session_start(&s, order, 4);
    assert(quiz_session_accuracy(&s) == 0);

    // 直接调整计数验证四舍五入。
    s.score = 3;
    assert(quiz_session_accuracy(&s) == 75);
    s.score = 1;
    s.count = 3;
    assert(quiz_session_accuracy(&s) == 33);
    s.score = 2;
    assert(quiz_session_accuracy(&s) == 67);
}

int main(void) {
    test_bank();
    test_order();
    test_session_scoring();
    test_accuracy();
    puts("test_quiz_model: PASS");
    return 0;
}
