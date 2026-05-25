#include "settings.h"
#include "ui.h"
#include "data.h"
#include "str.h"
#include "utils.h"
#include <ncurses.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

static char *settings_path(void)
{
    const char *home = getenv("HOME");
    if (!home) return NULL;
    size_t len = strlen(home) + 20;
    char *path = (char *)malloc(len);
    if (!path) return NULL;
    snprintf(path, len, "%s/.terquiz/settings", home);
    return path;
}

void settings_defaults(Settings *s)
{
    s->categories = 0x7FF;
    s->sources = 1;
    s->guess_mode = GUESS_MIXED;
    s->quiz_length = 20;
    s->timer_seconds = TIMER_OFF;
    s->lives = 3;
    s->practice_mode = false;
    s->language = LANG_ENGLISH;
}

void settings_load(Settings *s)
{
    settings_defaults(s);
    char *path = settings_path();
    if (!path) return;
    FILE *f = fopen(path, "r");
    free(path);
    if (!f) return;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        utils_strip_newline(line);
        if (line[0] == '#' || line[0] == '\0') continue;
        char *key = strtok(line, "|");
        char *val = strtok(NULL, "|");
        if (!key || !val) continue;
        int v = atoi(val);
        if (strcmp(key, "categories") == 0) s->categories = v;
        else if (strcmp(key, "sources") == 0) s->sources = v;
        else if (strcmp(key, "guess_mode") == 0) s->guess_mode = v;
        else if (strcmp(key, "quiz_length") == 0) s->quiz_length = v;
        else if (strcmp(key, "timer_seconds") == 0) s->timer_seconds = v;
        else if (strcmp(key, "lives") == 0) s->lives = v;
        else if (strcmp(key, "practice_mode") == 0) s->practice_mode = (v != 0);
        else if (strcmp(key, "language") == 0) s->language = v;
    }
    fclose(f);
}

void settings_save(const Settings *s)
{
    const char *home = getenv("HOME");
    if (!home) return;
    size_t dlen = strlen(home) + 12;
    char *dir = (char *)malloc(dlen);
    if (!dir) return;
    snprintf(dir, dlen, "%s/.terquiz", home);
    mkdir(dir, 0700);
    free(dir);

    char *path = settings_path();
    if (!path) return;
    FILE *f = fopen(path, "w");
    free(path);
    if (!f) return;

    fprintf(f, "categories|%d\n", s->categories);
    fprintf(f, "sources|%d\n", s->sources);
    fprintf(f, "guess_mode|%d\n", s->guess_mode);
    fprintf(f, "quiz_length|%d\n", s->quiz_length);
    fprintf(f, "timer_seconds|%d\n", s->timer_seconds);
    fprintf(f, "lives|%d\n", s->lives);
    fprintf(f, "practice_mode|%d\n", s->practice_mode ? 1 : 0);
    fprintf(f, "language|%d\n", s->language);
    fclose(f);
}

static void draw_checkbox(int y, int x, bool checked)
{
    mvaddch(y, x, checked ? '[' | A_BOLD : '[');
    if (checked) {
        attron(A_BOLD);
        mvaddch(y, x + 1, 'X');
        attroff(A_BOLD);
    } else {
        mvaddch(y, x + 1, ' ');
    }
    mvaddch(y, x + 2, ']');
}

static void draw_label(int y, int x, const char *label, bool active)
{
    if (active)
        attron(COLOR_PAIR(COLOR_HIGHLIGHT));
    mvprintw(y, x, "%s", label);
    if (active)
        attroff(COLOR_PAIR(COLOR_HIGHLIGHT));
}

static void draw_option_row(int y, int x, const char *label, int label_w,
                            const char **opt_labels, const int *opt_vals,
                            int n_opts, int spacing, int current_val, bool active)
{
    (void)spacing;
    draw_label(y, x, label, active);
    int cx = x + label_w + 2;
    for (int i = 0; i < n_opts; i++) {
        if (current_val == opt_vals[i])
            attron(A_REVERSE);
        mvprintw(y, cx, "(%s)", opt_labels[i]);
        if (current_val == opt_vals[i])
            attroff(A_REVERSE);
        cx += (int)strlen(opt_labels[i]) + 4 + 2;
    }
}

static int calc_label_w(void)
{
    const char *labels[] = {
        S(STR_SETTINGS_MODE),
        S(STR_SETTINGS_LENGTH),
        S(STR_SETTINGS_TIMER),
        S(STR_SETTINGS_LIVES),
        S(STR_SETTINGS_PRACTICE),
        S(STR_SETTINGS_LANGUAGE),
    };
    int max = 0;
    for (size_t i = 0; i < sizeof(labels) / sizeof(labels[0]); i++) {
        int len = (int)strlen(labels[i]);
        if (len > max) max = len;
    }
    return max;
}

static int option_row_w(const char **opt_labels, int n_opts)
{
    int w = 0;
    for (int i = 0; i < n_opts; i++)
        w += (int)strlen(opt_labels[i]) + 4 + 2;
    return w > 0 ? w - 2 : 0;
}

int settings_menu(Settings *s)
{
    const char *guess_labels[3];
    guess_labels[0] = S(STR_SETTINGS_GUESS_CMD);
    guess_labels[1] = S(STR_SETTINGS_GUESS_DEF);
    guess_labels[2] = S(STR_SETTINGS_GUESS_MIXED);
    const int guess_vals[] = {GUESS_CMD, GUESS_DEF, GUESS_MIXED};

    const char *length_labels[4];
    length_labels[0] = S(STR_SETTINGS_LENGTH_10);
    length_labels[1] = S(STR_SETTINGS_LENGTH_20);
    length_labels[2] = S(STR_SETTINGS_LENGTH_50);
    length_labels[3] = S(STR_SETTINGS_LENGTH_ENDLESS);
    const int length_vals[] = {10, 20, 50, LENGTH_ENDLESS};
    const int num_lengths = 4;

    const char *timer_labels[5];
    timer_labels[0] = S(STR_SETTINGS_TIMER_OFF);
    timer_labels[1] = S(STR_SETTINGS_TIMER_5);
    timer_labels[2] = S(STR_SETTINGS_TIMER_10);
    timer_labels[3] = S(STR_SETTINGS_TIMER_15);
    timer_labels[4] = S(STR_SETTINGS_TIMER_30);
    const int timer_vals[] = {TIMER_OFF, 5, 10, 15, 30};
    const int num_timers = 5;

    const char *lives_labels[4];
    lives_labels[0] = S(STR_SETTINGS_LIVES_OFF);
    lives_labels[1] = S(STR_SETTINGS_LIVES_1);
    lives_labels[2] = S(STR_SETTINGS_LIVES_3);
    lives_labels[3] = S(STR_SETTINGS_LIVES_5);
    const int lives_vals[] = {LIVES_OFF, 1, 3, 5};
    const int num_lives = 4;

    const char *lang_labels[2];
    lang_labels[0] = S(STR_SETTINGS_LANG_ENG);
    lang_labels[1] = S(STR_SETTINGS_LANG_FI);
    const int lang_vals[] = {LANG_ENGLISH, LANG_FINNISH};
    const int num_langs = 2;

    enum {
        ROW_MODE,
        ROW_LENGTH,
        ROW_TIMER,
        ROW_LIVES,
        ROW_PRACTICE,
        ROW_LANG,
        ROW_TOTAL
    };

    int label_w = calc_label_w();

    int content_w = label_w + 2 + option_row_w(guess_labels, 3);
    content_w = MAX(content_w, label_w + 2 + option_row_w(length_labels, 4));
    content_w = MAX(content_w, label_w + 2 + option_row_w(timer_labels, 5));
    content_w = MAX(content_w, label_w + 2 + option_row_w(lives_labels, 4));
    content_w = MAX(content_w, label_w + 2 + option_row_w(lang_labels, 2));
    content_w = MAX(content_w, label_w + 2 + 4 + (int)strlen(S(STR_SETTINGS_PRACTICE_OFF)));
    content_w = MAX(content_w, (int)strlen(S(STR_SETTINGS_NAV)));
    content_w = MAX(content_w, (int)strlen(S(STR_SETTINGS_BACK)));

    int current_row = 0;

    while (1) {
        ui_clear();
        int maxy, maxx;
        getmaxyx(stdscr, maxy, maxx);
        int width = content_w + 6;
        if (width > maxx - 2) width = maxx - 2;
        if (width < 40) width = 40;

        int height = 16;
        if (height > maxy - 2) height = maxy - 2;
        if (height < 12) height = 12;

        int ox = (maxx - width) / 2;
        int oy = (maxy - height) / 2;
        if (ox < 0) ox = 0;
        if (oy < 0) oy = 0;

        Rect r = {ox, oy, width, height};
        ui_draw_border(r, S(STR_SETTINGS_TITLE));

        int y = oy + 1;
        int x = ox + 2;
        int lx = x + label_w + 2;

        (void)lx;

        bool active;

        active = (current_row == ROW_MODE);
        draw_option_row(y, x, S(STR_SETTINGS_MODE), label_w,
                        guess_labels, guess_vals, 3, 7, s->guess_mode, active);
        y++;

        active = (current_row == ROW_LENGTH);
        draw_option_row(y, x, S(STR_SETTINGS_LENGTH), label_w,
                        length_labels, length_vals, num_lengths, 9, s->quiz_length, active);
        y++;

        active = (current_row == ROW_TIMER);
        draw_option_row(y, x, S(STR_SETTINGS_TIMER), label_w,
                        timer_labels, timer_vals, num_timers, 6, s->timer_seconds, active);
        y++;

        active = (current_row == ROW_LIVES);
        draw_option_row(y, x, S(STR_SETTINGS_LIVES), label_w,
                        lives_labels, lives_vals, num_lives, 6, s->lives, active);
        y++;

        active = (current_row == ROW_PRACTICE);
        draw_label(y, x, S(STR_SETTINGS_PRACTICE), active);
        draw_checkbox(y, lx, s->practice_mode);
        mvprintw(y, lx + 4, "%s", S(STR_SETTINGS_PRACTICE_OFF));
        y++;

        active = (current_row == ROW_LANG);
        draw_option_row(y, x, S(STR_SETTINGS_LANGUAGE), label_w,
                        lang_labels, lang_vals, num_langs, 9, s->language, active);
        y++;

        y++;
        mvprintw(y, x, "%s", S(STR_SETTINGS_NAV));
        y++;
        attron(COLOR_PAIR(COLOR_TITLE) | A_BOLD);
        mvprintw(y, x, "%s", S(STR_SETTINGS_BACK));
        attroff(COLOR_PAIR(COLOR_TITLE) | A_BOLD);

        ui_refresh();

        int ch = ui_getch();

        if (ch == 'q' || ch == 'Q') break;

        if (ch == '\t' || ch == KEY_DOWN || ch == 'j') {
            current_row++;
            if (current_row >= ROW_TOTAL) current_row = 0;
        } else if (ch == KEY_BTAB || ch == KEY_UP || ch == 'k') {
            current_row--;
            if (current_row < 0) current_row = ROW_TOTAL - 1;
        } else if (ch == ' ' || ch == '\n') {
            if (current_row == ROW_PRACTICE)
                s->practice_mode = !s->practice_mode;
        } else if (ch == KEY_LEFT) {
            if (current_row == ROW_MODE)
                s->guess_mode = (s->guess_mode + 2) % 3;
            else if (current_row == ROW_LENGTH) {
                for (int i = num_lengths - 1; i >= 0; i--)
                    if (s->quiz_length == length_vals[i]) {
                        s->quiz_length = length_vals[(i + num_lengths - 1) % num_lengths];
                        break;
                    }
            } else if (current_row == ROW_TIMER) {
                for (int i = num_timers - 1; i >= 0; i--)
                    if (s->timer_seconds == timer_vals[i]) {
                        s->timer_seconds = timer_vals[(i + num_timers - 1) % num_timers];
                        break;
                    }
            } else if (current_row == ROW_LIVES) {
                for (int i = num_lives - 1; i >= 0; i--)
                    if (s->lives == lives_vals[i]) {
                        s->lives = lives_vals[(i + num_lives - 1) % num_lives];
                        break;
                    }
            } else if (current_row == ROW_LANG) {
                for (int i = num_langs - 1; i >= 0; i--)
                    if (s->language == lang_vals[i]) {
                        s->language = lang_vals[(i + num_langs - 1) % num_langs];
                        break;
                    }
            }
        } else if (ch == KEY_RIGHT) {
            if (current_row == ROW_MODE)
                s->guess_mode = (s->guess_mode + 1) % 3;
            else if (current_row == ROW_LENGTH) {
                for (int i = 0; i < num_lengths; i++)
                    if (s->quiz_length == length_vals[i]) {
                        s->quiz_length = length_vals[(i + 1) % num_lengths];
                        break;
                    }
            } else if (current_row == ROW_TIMER) {
                for (int i = 0; i < num_timers; i++)
                    if (s->timer_seconds == timer_vals[i]) {
                        s->timer_seconds = timer_vals[(i + 1) % num_timers];
                        break;
                    }
            } else if (current_row == ROW_LIVES) {
                for (int i = 0; i < num_lives; i++)
                    if (s->lives == lives_vals[i]) {
                        s->lives = lives_vals[(i + 1) % num_lives];
                        break;
                    }
            } else if (current_row == ROW_LANG) {
                for (int i = 0; i < num_langs; i++)
                    if (s->language == lang_vals[i]) {
                        s->language = lang_vals[(i + 1) % num_langs];
                        break;
                    }
            }
        }
    }

    return 0;
}
