#ifndef QUIZ_H
#define QUIZ_H

#include <stdbool.h>
#include "data.h"
#include "progress.h"

#define NUM_OPTIONS 6

typedef struct {
    Entry *correct;
    Entry *options[NUM_OPTIONS];
    int correct_index;
    bool guess_command;
} Question;

Question *question_generate(const Pool *pool, Progress *prog, bool prefer_unmastered, int guess_mode, int max_diff);
Question *question_generate_for_entry(const Pool *pool, const Entry *correct, int guess_mode, Progress *prog);
void question_free(Question *q);
void question_print(const Question *q, int num, int total, int score, int streak, int lives, int timer, const char *prev_result, bool is_retry);
int question_get_answer(void);
int question_get_typed_answer(char *buf, int bufsize, int input_y, int input_x);

#endif
