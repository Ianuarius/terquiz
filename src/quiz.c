#include "quiz.h"
#include "utils.h"
#include "ui.h"
#include "settings.h"
#include "str.h"
#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

static char first_char(const char *s)
{
    while (*s == ' ') s++;
    return (char)tolower((unsigned char)*s);
}

static void make_distractors(const Pool *pool, const Entry *correct,
                             Entry **distractors, int *n, bool pick_desc)
{
    *n = 0;
    char fc = first_char(correct->command);

    for (int i = 0; i < pool->count && *n < 5; i++) {
        Entry *e = pool->entries[i];
        if (e == correct) continue;
        if (first_char(e->command) != fc) continue;
        if (pick_desc) {
            if (strcmp(e->description, correct->description) == 0) continue;
        } else {
            if (strcmp(e->command, correct->command) == 0) continue;
        }
        distractors[(*n)++] = e;
    }

    if (*n < 5) {
        for (int i = 0; i < pool->count && *n < 5; i++) {
            Entry *e = pool->entries[i];
            if (e == correct) continue;
            if (strcmp(e->category, correct->category) != 0) continue;
            bool dup = false;
            for (int j = 0; j < *n; j++)
                if (distractors[j] == e) { dup = true; break; }
            if (dup) continue;
            distractors[(*n)++] = e;
        }
    }

    if (*n < 5) {
        for (int i = 0; i < pool->count && *n < 5; i++) {
            Entry *e = pool->entries[i];
            if (e == correct) continue;
            bool dup = false;
            for (int j = 0; j < *n; j++)
                if (distractors[j] == e) { dup = true; break; }
            if (dup) continue;
            distractors[(*n)++] = e;
        }
    }

    utils_shuffle(distractors, (size_t)*n, sizeof(Entry *));
}

static int count_eligible(const Pool *pool, int max_diff)
{
    if (max_diff <= 0) return pool->count;
    int n = 0;
    for (int i = 0; i < pool->count; i++)
        if (pool->entries[i]->difficulty <= max_diff) n++;
    return n;
}

static int find_eligible_index(const Pool *pool, int max_diff, int nth)
{
    for (int i = 0; i < pool->count; i++) {
        if (max_diff <= 0 || pool->entries[i]->difficulty <= max_diff) {
            if (nth == 0) return i;
            nth--;
        }
    }
    return 0;
}

static Question *question_make(const Pool *pool, const Entry *correct, int guess_mode, Progress *prog)
{
    Question *q = (Question *)calloc(1, sizeof(Question));
    if (!q) return NULL;
    q->correct = (Entry *)correct;

    if (guess_mode == GUESS_CMD)
        q->guess_command = true;
    else if (guess_mode == GUESS_DEF)
        q->guess_command = false;
    else
        q->guess_command = (utils_randint(0, 1) == 0);

    if (prog && !q->guess_command) {
        char *base = utils_base_command(correct->command);
        int typed = progress_typed_correct(prog, base);
        int total = progress_total(prog, base);
        free(base);
        if (total - typed >= 4 && typed == 0)
            q->guess_command = true;
    }

    {
        Entry *distractors[5];
        int ndist = 0;
        make_distractors(pool, correct, distractors, &ndist, !q->guess_command);

        q->options[0] = (Entry *)correct;
        for (int i = 0; i < ndist && i < 5; i++)
            q->options[i + 1] = distractors[i];

        int n_opts = ndist + 1;
        for (int i = n_opts; i < 6; i++) {
            for (int attempt = 0; attempt < pool->count; attempt++) {
                Entry *e = pool->entries[utils_randint(0, pool->count - 1)];
                bool already = false;
                for (int j = 0; j < i; j++)
                    if (q->options[j] == e) { already = true; break; }
                if (!already) {
                    q->options[i] = e;
                    break;
                }
            }
            if (!q->options[i]) q->options[i] = (Entry *)correct;
        }

        int indices[6];
        for (int i = 0; i < 6; i++) indices[i] = i;
        utils_shuffle(indices, 6, sizeof(int));

        Entry *shuffled[6];
        for (int i = 0; i < 6; i++)
            shuffled[i] = q->options[indices[i]];
        for (int i = 0; i < 6; i++)
            q->options[i] = shuffled[i];

        q->correct_index = -1;
        for (int i = 0; i < 6; i++)
            if (q->options[i] == correct) { q->correct_index = i; break; }
    }

    return q;
}

Question *question_generate(const Pool *pool, Progress *prog, bool prefer_unmastered, int guess_mode, int max_diff)
{
    if (!pool || pool->count == 0) return NULL;

    int eligible = count_eligible(pool, max_diff);
    if (eligible == 0) return NULL;

    Entry *correct = NULL;

    if (prefer_unmastered && prog) {
        for (int attempt = 0; attempt < 20; attempt++) {
            int idx = find_eligible_index(pool, max_diff,
                utils_randint(0, eligible - 1));
            Entry *e = pool->entries[idx];
            char *base = utils_base_command(e->command);
            int streak = progress_streak(prog, base);
            free(base);
            if (streak < 5) {
                correct = e;
                break;
            }
        }
    }

    if (!correct) {
        int idx = find_eligible_index(pool, max_diff,
            utils_randint(0, eligible - 1));
        correct = pool->entries[idx];
    }

    return question_make(pool, correct, guess_mode, prog);
}

Question *question_generate_for_entry(const Pool *pool, const Entry *correct, int guess_mode, Progress *prog)
{
    if (!pool || pool->count == 0 || !correct) return NULL;
    return question_make(pool, correct, guess_mode, prog);
}

void question_free(Question *q)
{
    free(q);
}

static bool is_hotkey_entry(const Entry *e)
{
    return e->source & SOURCE_HOTKEYS;
}

void question_print(const Question *q, int num, int total, int score,
                    int streak, int lives, int timer,
                    const char *prev_result, bool is_retry)
{
    ui_clear();
    int maxy, maxx;
    getmaxyx(stdscr, maxy, maxx);

    int width = maxx - 4;
    if (width > 70) width = 70;
    int height = maxy - 2;
    if (height > 24) height = 24;
    int ox = (maxx - width) / 2;
    int oy = (maxy - height) / 2;

    Rect r = {ox, oy, width, height};
    char title[64];
    if (total > 0)
        snprintf(title, sizeof(title), S(STR_GAME_QUESTION_FMT), num, total);
    else
        snprintf(title, sizeof(title), S(STR_GAME_QUESTION_N), num);
    ui_draw_border(r, title);

    int y = 0;

    if (is_retry) {
        ui_draw_text(r, y, 0, COLOR_INFO | A_BOLD, "%s", S(STR_GAME_RETRY_TAG));
        y++;
    }

    if (prev_result) {
        ui_draw_text(r, y, 0, COLOR_DIM, "%s", prev_result);
        y++;
    }

    char header[128];
    char buf[32];
    snprintf(header, sizeof(header), S(STR_GAME_SCORE), score);
    int hlen = (int)strlen(header);
    snprintf(buf, sizeof(buf), S(STR_GAME_STREAK), streak);
    snprintf(header + hlen, sizeof(header) - hlen, "  %s", buf);
    if (lives > 0) {
        hlen = (int)strlen(header);
        snprintf(buf, sizeof(buf), S(STR_GAME_LIVES), lives);
        snprintf(header + hlen, sizeof(header) - hlen, "  %s", buf);
    }
    ui_draw_text(r, y, 0, COLOR_INFO, "%s", header);
    y += 2;

    if (q->guess_command) {
        ui_draw_text(r, y++, 0, COLOR_TITLE | A_BOLD, "%s", S(STR_GAME_GUESS_CMD));
        y++;
        ui_draw_text(r, y++, 0, COLOR_NORMAL, "%s", q->correct->description);
    } else {
        ui_draw_text(r, y++, 0, COLOR_TITLE | A_BOLD, "%s", S(STR_GAME_GUESS_DEF));
        y++;
        if (is_hotkey_entry(q->correct)) {
            attron(A_REVERSE | A_BOLD);
            ui_draw_text(r, y++, 0, COLOR_NORMAL, "[ %s ]", q->correct->command);
            attroff(A_REVERSE | A_BOLD);
        } else {
            ui_draw_text(r, y++, 0, COLOR_NORMAL, "%s", q->correct->command);
        }
    }

    if (!q->guess_command) {
        y += 2;

        for (int i = 0; i < 6; i++) {
            if (!q->options[i]) {
                ui_draw_text(r, y++, 0, COLOR_DIM, "%d. %s", i + 1, S(STR_GAME_OPTION_NONE));
                continue;
            }
            ui_draw_text(r, y++, 0, COLOR_NORMAL, "%d. %s", i + 1, q->options[i]->description);
        }

        if (timer > 0) {
            y++;
            ui_draw_text(r, y, 0, COLOR_INFO, S(STR_GAME_TIME_LIMIT), timer);
        }

        y = height - 2;
        ui_draw_text(r, y, 0, COLOR_DIM, "%s", S(STR_GAME_SELECT));
    } else {
        y += 2;
        const char *prompt = S(STR_GAME_TYPE_PROMPT);
        ui_draw_text(r, y, 0, COLOR_TITLE | A_BOLD, "%s", prompt);

        if (timer > 0) {
            y++;
            ui_draw_text(r, y, 0, COLOR_INFO, S(STR_GAME_TIME_LIMIT), timer);
        }

        y = height - 2;
        ui_draw_text(r, y, 0, COLOR_DIM, "%s", S(STR_GAME_SELECT_TYPED));
    }
}

int question_get_answer(void)
{
    while (1) {
        int ch = ui_getch();
        if (ch >= '1' && ch <= '6') return ch - '1';
        if (ch == 'q' || ch == 'Q') return -1;
    }
}

int question_get_typed_answer(char *buf, int bufsize, int input_y, int input_x)
{
    int pos = 0;
    buf[0] = '\0';
    move(input_y, input_x);

    while (1) {
        int ch = ui_getch();

        if (ch == '\n' || ch == KEY_ENTER) {
            buf[pos] = '\0';
            return 0;
        }

        if (ch == 'q' || ch == 'Q') {
            buf[pos] = '\0';
            return -1;
        }

        if (ch == 127 || ch == KEY_BACKSPACE || ch == '\b') {
            if (pos > 0) {
                pos--;
                buf[pos] = '\0';
                mvaddch(input_y, input_x + pos, ' ');
                move(input_y, input_x + pos);
            }
            continue;
        }

        if (ch == 21 || ch == 11) {
            int old = pos;
            pos = 0;
            buf[0] = '\0';
            for (int i = 0; i < old; i++)
                mvaddch(input_y, input_x + i, ' ');
            move(input_y, input_x);
            continue;
        }

        if (ch == 23 && pos > 0) {
            int old = pos;
            while (pos > 0 && buf[pos - 1] == ' ') pos--;
            while (pos > 0 && buf[pos - 1] != ' ') pos--;
            buf[pos] = '\0';
            for (int i = pos; i < old; i++)
                mvaddch(input_y, input_x + i, ' ');
            move(input_y, input_x + pos);
            continue;
        }

        if (ch >= 32 && ch <= 126 && pos < bufsize - 1) {
            buf[pos++] = (char)ch;
            buf[pos] = '\0';
            mvaddch(input_y, input_x + pos - 1, ch);
            move(input_y, input_x + pos);
        }
    }
}
