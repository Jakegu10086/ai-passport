#include "quiz_model.h"

#include <stddef.h>

static uint32_t lcg_next(uint32_t *state) {
    *state = *state * 1664525u + 1013904223u;
    return *state >> 16;
}

void quiz_build_order(int *order, int bank_count, int round_size, uint32_t seed) {
    if (!order) return;
    if (bank_count <= 0 || round_size <= 0) return;

    int pool[QUIZ_BANK_MAX];
    int pool_count = bank_count < QUIZ_BANK_MAX ? bank_count : QUIZ_BANK_MAX;
    for (int i = 0; i < pool_count; i++) pool[i] = i;

    uint32_t state = seed ? seed : 1u;
    for (int i = pool_count - 1; i > 0; i--) {
        int j = (int)(lcg_next(&state) % (uint32_t)(i + 1));
        int tmp = pool[i];
        pool[i] = pool[j];
        pool[j] = tmp;
    }

    if (round_size > pool_count) round_size = pool_count;
    for (int i = 0; i < round_size; i++) order[i] = pool[i];
}

void quiz_session_start(quiz_session_t *s, const int *order, int count) {
    if (!s) return;
    if (!order || count <= 0) count = 0;
    if (count > QUIZ_BANK_MAX) count = QUIZ_BANK_MAX;

    for (int i = 0; i < count; i++) s->order[i] = order[i];
    s->count = count;
    s->index = 0;
    s->selection = 0;
    s->score = 0;
    s->answered = false;
    s->finished = (count == 0);
    s->last_correct = false;
}

void quiz_session_move(quiz_session_t *s, int delta) {
    if (!s || s->finished || s->answered) return;
    int next = (s->selection + delta) % QUIZ_OPTION_COUNT;
    if (next < 0) next += QUIZ_OPTION_COUNT;
    s->selection = next;
}

quiz_event_t quiz_session_confirm(quiz_session_t *s) {
    if (!s || s->finished) return QUIZ_EVENT_NONE;

    if (!s->answered) {
        const quiz_question_t *q = quiz_session_question(s);
        if (!q) return QUIZ_EVENT_NONE;
        s->last_correct = (s->selection == (int)q->answer);
        if (s->last_correct) s->score++;
        s->answered = true;
        return QUIZ_EVENT_ANSWERED;
    }

    if (s->index + 1 >= s->count) {
        s->finished = true;
        return QUIZ_EVENT_FINISHED;
    }

    s->index++;
    s->selection = 0;
    s->answered = false;
    return QUIZ_EVENT_ADVANCED;
}

const quiz_question_t *quiz_session_question(const quiz_session_t *s) {
    if (!s || s->finished || s->index < 0 || s->index >= s->count) return NULL;
    return quiz_bank(NULL) + s->order[s->index];
}

int quiz_session_accuracy(const quiz_session_t *s) {
    if (!s || s->count <= 0) return 0;
    return (s->score * 100 + s->count / 2) / s->count;
}
