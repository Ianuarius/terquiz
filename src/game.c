#include "game.h"
#include "quiz.h"
#include "ui.h"
#include "utils.h"
#include "str.h"
#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>


static void get_typed_pos(bool is_retry, const char *prev_result, int *out_y, int *out_x)
{
    int maxy, maxx;
    getmaxyx(stdscr, maxy, maxx);
    int width = maxx - 4;
    if (width > 70) width = 70;
    int height = maxy - 2;
    if (height > 24) height = 24;
    int ox = (maxx - width) / 2;
    int oy = (maxy - height) / 2;

    int line = 7;
    if (is_retry) line++;
    if (prev_result && prev_result[0]) line++;

    *out_y = oy + 1 + line;
    *out_x = ox + 1 + (int)strlen(S(STR_GAME_TYPE_PROMPT));
}

GameResult game_run(const Settings *s, Pool *pool, Progress **prog, DifficultyState *ds)
{
    GameResult result = {0, 0, 0, 0, 0, 0};

    Pool *qp = pool_filter(pool, s->categories, s->sources);
    if (!qp || qp->count == 0) {
        ui_message(ui_center_rect(40, 6), S(STR_MSG_NO_COMMANDS));
        pool_free(qp);
        return result;
    }

    bool endless = (s->quiz_length == LENGTH_ENDLESS);
    int max_questions = endless ? 999999 : s->quiz_length;

    result.lives_used = 0;
    int lives = s->lives;
    int timer = s->timer_seconds;

    Entry *first_entries[MAX_COMMANDS];
    int first_count = 0;

    Entry *retry[MAX_COMMANDS];
    int retry_count = 0;

    int question_number = 0;
    char prev_result[128] = "";
    int display_total = endless ? 0 : max_questions;

    /* ---- PHASE 1: First-attempt questions ---- */
    while (first_count < max_questions) {
        int max_diff = difficulty_pick_level(ds, qp, prog ? *prog : NULL);
        bool prefer_unmastered = (prog != NULL && *prog != NULL);

        Question *q = NULL;
        int attempts = 0;
        int diff = max_diff;
        do {
            q = question_generate(qp, prog ? *prog : NULL, prefer_unmastered,
                                  GUESS_MIXED, diff);
            if (!q) break;
            bool used = false;
            for (int i = 0; i < first_count; i++) {
                if (q->correct == first_entries[i]) {
                    used = true;
                    break;
                }
            }
            if (!used) break;
            question_free(q);
            q = NULL;
            attempts++;
            if (attempts > 100) {
                if (diff < 20) {
                    diff++;
                    attempts = 0;
                } else {
                    break;
                }
            }
        } while (1);

        if (!q) break;

        first_entries[first_count] = q->correct;
        first_count++;
        question_number++;

        question_print(q, question_number, display_total,
                       result.score, result.current_streak,
                       s->practice_mode ? -1 : lives, timer,
                       prev_result[0] ? prev_result : NULL, false);
        ui_refresh();

        /* ---- FIRST ATTEMPT PATH: scoring ---- */
        int answer = -1;
        bool timed_out = false;
        bool correct = false;
        char typed[256] = "";
        typed[0] = '\0';

        if (q->guess_command) {
            int iy, ix;
            get_typed_pos(false, prev_result[0] ? prev_result : NULL, &iy, &ix);
            int tr = question_get_typed_answer(typed, sizeof(typed), iy, ix);
            if (tr == -1) { question_free(q); break; }

            size_t tlen = strlen(typed);
            while (tlen > 0 && typed[tlen - 1] == ' ') typed[--tlen] = '\0';

            if (strcmp(typed, q->correct->command) == 0) {
                answer = q->correct_index;
                correct = true;
            }
        } else if (timer > 0) {
            timeout(200);
            time_t start = time(NULL);
            while (1) {
                int remaining = timer - (int)(time(NULL) - start);
                if (remaining < 0) remaining = 0;

                question_print(q, question_number, display_total,
                               result.score, result.current_streak,
                               s->practice_mode ? -1 : lives, remaining,
                               prev_result, false);
                ui_refresh();

                int ch = ui_getch();
                if (ch >= '1' && ch <= '6') {
                    answer = ch - '1';
                    break;
                }
                if (ch == 'q' || ch == 'Q') { answer = -2; break; }
                if (remaining <= 0) { timed_out = true; break; }
            }
            timeout(-1);

            if (answer == -2) { question_free(q); break; }
            if (!timed_out && answer >= 0 && answer == q->correct_index)
                correct = true;
        } else {
            answer = question_get_answer();
            if (answer == -1) { question_free(q); break; }
            if (answer == q->correct_index)
                correct = true;
        }

        /* Build prev_result for next screen */
        if (timed_out) {
            snprintf(prev_result, sizeof(prev_result),
                     S(STR_RESULT_TIMED_OUT), q->correct->command);
        } else if (correct) {
            if (q->guess_command)
                snprintf(prev_result, sizeof(prev_result),
                         S(STR_RESULT_CORRECT), q->correct->command);
            else
                snprintf(prev_result, sizeof(prev_result),
                         S(STR_RESULT_CORRECT), q->correct->description);
        } else {
            if (q->guess_command)
                snprintf(prev_result, sizeof(prev_result),
                         S(STR_RESULT_WRONG_CMD),
                         q->correct->command, typed);
            else
                snprintf(prev_result, sizeof(prev_result),
                         S(STR_RESULT_WRONG_DEF),
                         q->options[answer]->description,
                         q->correct->description);
        }

        /* Score this first attempt */
        if (correct) {
            result.correct++;
            result.score += 10 + result.current_streak * 2;
            result.current_streak++;
            if (result.current_streak > result.best_streak)
                result.best_streak = result.current_streak;

            if (prog && *prog) {
                char *base = utils_base_command(q->correct->command);
                *prog = progress_update(*prog, base, 1, q->guess_command ? 1 : 0);
                free(base);
            }
        } else {
            bool already = false;
            for (int i = 0; i < retry_count; i++)
                if (retry[i] == q->correct) { already = true; break; }
            if (!already && retry_count < MAX_COMMANDS)
                retry[retry_count++] = q->correct;

            if (!s->practice_mode) {
                result.current_streak = 0;
                if (lives > 0) {
                    lives--;
                    result.lives_used++;
                    if (lives <= 0) {
                        question_free(q);
                        break;
                    }
                }
            }

            if (prog && *prog) {
                char *base = utils_base_command(q->correct->command);
                *prog = progress_update(*prog, base, 0, 0);
                free(base);
            }
        }

        question_free(q);
        difficulty_update(ds, pool, s, prog ? *prog : NULL);
    }

    /* ---- PHASE 2: Retry wrong answers ---- */
    prev_result[0] = '\0';
    while (retry_count > 0) {
        Entry *e = retry[--retry_count];
        Question *q = question_generate_for_entry(qp, e, GUESS_MIXED, prog ? *prog : NULL);
        if (!q) continue;

        question_number++;

        question_print(q, question_number, 0,
                       result.score, result.current_streak,
                       -1, timer,
                       prev_result[0] ? prev_result : NULL, true);
        ui_refresh();

        /* ---- RETRY PATH: no scoring, no lives ---- */
        bool correct = false;
        bool timed_out = false;

        if (q->guess_command) {
            char typed[256] = "";
            int iy, ix;
            get_typed_pos(true, prev_result[0] ? prev_result : NULL, &iy, &ix);
            int tr = question_get_typed_answer(typed, sizeof(typed), iy, ix);
            if (tr == -1) { question_free(q); break; }

            size_t tlen = strlen(typed);
            while (tlen > 0 && typed[tlen - 1] == ' ') typed[--tlen] = '\0';

            if (strcmp(typed, q->correct->command) == 0)
                correct = true;

            if (correct)
                snprintf(prev_result, sizeof(prev_result),
                         S(STR_RESULT_RETRY_CORRECT), q->correct->command);
            else
                snprintf(prev_result, sizeof(prev_result),
                         S(STR_RESULT_RETRY_WRONG_CMD),
                         q->correct->command, typed);
        } else {
            int chosen = -1;

            if (timer > 0) {
                timeout(200);
                time_t start = time(NULL);
                bool quit = false;
                while (1) {
                    int remaining = timer - (int)(time(NULL) - start);
                    if (remaining < 0) remaining = 0;

                    question_print(q, question_number, 0,
                                   result.score, result.current_streak,
                                   -1, remaining,
                                   prev_result, true);
                    ui_refresh();

                    int ch = ui_getch();
                    if (ch >= '1' && ch <= '6') {
                        chosen = ch - '1';
                        correct = (chosen == q->correct_index);
                        break;
                    }
                    if (ch == 'q' || ch == 'Q') { quit = true; break; }
                    if (remaining <= 0) { timed_out = true; break; }
                }
                timeout(-1);
                if (quit) { question_free(q); break; }
            } else {
                int ans = question_get_answer();
                if (ans == -1) { question_free(q); break; }
                chosen = ans;
                correct = (ans == q->correct_index);
            }

            if (timed_out) {
                snprintf(prev_result, sizeof(prev_result),
                         S(STR_RESULT_RETRY_TIMED_OUT),
                         q->correct->command);
            } else if (correct) {
                snprintf(prev_result, sizeof(prev_result),
                         S(STR_RESULT_RETRY_CORRECT), q->correct->description);
            } else {
                snprintf(prev_result, sizeof(prev_result),
                         S(STR_RESULT_RETRY_WRONG_DEF),
                         q->options[chosen]->description,
                         q->correct->description);
            }
        }

        if (correct) {
            if (prog && *prog) {
                char *base = utils_base_command(q->correct->command);
                *prog = progress_update(*prog, base, 1, 0);
                free(base);
            }
        } else {
            bool already = false;
            for (int i = 0; i < retry_count; i++)
                if (retry[i] == q->correct) { already = true; break; }
            if (!already && retry_count < MAX_COMMANDS)
                retry[retry_count++] = q->correct;

            if (prog && *prog) {
                char *base = utils_base_command(q->correct->command);
                *prog = progress_update(*prog, base, 0, 0);
                free(base);
            }
        }

        difficulty_update(ds, pool, s, prog ? *prog : NULL);
        question_free(q);
    }

    result.total = first_count;

    pool_free(qp);
    return result;
}
