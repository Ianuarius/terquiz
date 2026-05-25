#define _GNU_SOURCE
#include "ui.h"
#include "data.h"
#include "settings.h"
#include "game.h"
#include "progress.h"
#include "str.h"
#include "difficulty.h"
#include "topics.h"
#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <libgen.h>

#define DATA_MAX 4096

static char data_dir[DATA_MAX];

static void resolve_data_dir(void)
{
    char buf[DATA_MAX];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len != -1) {
        buf[len] = '\0';
        char *dir = dirname(buf);
        snprintf(data_dir, sizeof(data_dir), "%s/data", dir);
    } else {
        strcpy(data_dir, "data");
    }
}

static const char *data_path(const char *fname)
{
    static char buf[DATA_MAX];
    snprintf(buf, sizeof(buf), "%s/%s", data_dir, fname);
    return buf;
}

static const int source_flags[NUM_SOURCES] = {
    SOURCE_CORE, SOURCE_HOTKEYS, SOURCE_BUILTINS, SOURCE_REDIR,
    SOURCE_DEBIAN, SOURCE_RHEL, SOURCE_ARCH, SOURCE_MACOS
};

static const char *master_name(int i)
{
    static const char *names[] = {
        "core.txt", "hotkeys.txt", "builtins.txt", "redir.txt",
        "debian.txt", "rhel.txt", "arch.txt", "macos.txt"
    };
    return data_path(names[i]);
}

static const char *lang_name(int language, int i)
{
    static const char *base[] = {
        "core", "hotkeys", "builtins", "redir",
        "debian", "rhel", "arch", "macos"
    };
    static const char *suffix[] = {"-eng.txt", "-fi.txt"};
    char buf[64];
    snprintf(buf, sizeof(buf), "%s%s", base[i], suffix[language]);
    return data_path(buf);
}

static const char *strings_file(int language)
{
    return data_path(language == 0 ? "strings-eng.txt" : "strings-fi.txt");
}

static Pool *load_all(int language)
{
    Pool *pool = pool_create();
    if (!pool) return NULL;

    for (int i = 0; i < NUM_SOURCES; i++) {
        Pool *sp = pool_load_master(master_name(i), source_flags[i]);
        if (sp) {
            pool_apply_lang(sp, lang_name(language, i));
            for (int j = 0; j < sp->count; j++)
                pool_add(pool, sp->entries[j]);
            free(sp->entries);
            free(sp);
        }
    }

    return pool;
}

static void show_progress(Progress *prog)
{
    if (!prog || progress_count(prog) == 0) {
        ui_message(ui_center_rect(40, 6), S(STR_PROGRESS_EMPTY));
        return;
    }

    int maxy, maxx;
    getmaxyx(stdscr, maxy, maxx);
    int width = maxx - 4;
    if (width > 60) width = 60;
    int height = maxy - 4;
    if (height > 24) height = 24;
    Rect r = ui_center_rect(width, height);

    int offset = 0;
    int count = progress_count(prog);
    int visible = height - 4;

    while (1) {
        ui_clear();
        ui_draw_border(r, S(STR_PROGRESS_TITLE));

        if (offset < 0) offset = 0;
        if (offset > count - visible) offset = count - visible > 0 ? count - visible : 0;

        ui_draw_text(r, 0, 0, COLOR_TITLE, "%s",
                     S(STR_PROGRESS_HEADER));

        for (int i = 0; i < visible && (offset + i) < count; i++) {
            int idx = offset + i;
            const char *status = prog[idx].mastered ? S(STR_PROGRESS_MASTERED) :
                                 (prog[idx].streak > 0 ? S(STR_PROGRESS_LEARNING) : S(STR_PROGRESS_NEW));
            int color = prog[idx].mastered ? COLOR_CORRECT :
                        (prog[idx].streak > 0 ? COLOR_INFO : COLOR_NORMAL);
            ui_draw_text(r, 2 + i, 0, color, "%-20s %-7d %-5d %s",
                         prog[idx].command, prog[idx].streak,
                         prog[idx].total, status);
        }

        if (count > visible) {
            if (offset > 0) ui_draw_text(r, 0, width - 2, COLOR_DIM, "^");
            if (offset + visible < count) ui_draw_text(r, height - 1, width - 2, COLOR_DIM, "v");
        }

        ui_draw_text(r, height - 1, 0, COLOR_DIM, "%s", S(STR_PROGRESS_NAV));
        ui_refresh();

        int ch = ui_getch();
        if (ch == 'q' || ch == 'Q') break;
        if (ch == KEY_DOWN || ch == 'j') offset++;
        if (ch == KEY_UP || ch == 'k') offset--;
    }
}

static void show_results(GameResult *res, Rect r)
{
    ui_clear();
    ui_draw_border(r, S(STR_RESULTS_TITLE));

    int pct = res->total > 0 ? (res->correct * 100) / res->total : 0;

    ui_draw_centered(r, 1, COLOR_TITLE | A_BOLD, "%s", S(STR_RESULTS_HEADER));
    ui_draw_text(r, 3, 0, COLOR_NORMAL, S(STR_RESULTS_SCORE), res->score);
    ui_draw_text(r, 4, 0, COLOR_NORMAL, S(STR_RESULTS_CORRECT), res->correct, res->total);
    ui_draw_text(r, 5, 0, COLOR_NORMAL, S(STR_RESULTS_ACCURACY), pct);
    ui_draw_text(r, 6, 0, COLOR_NORMAL, S(STR_RESULTS_BEST_STREAK), res->best_streak);
    if (res->lives_used > 0)
        ui_draw_text(r, 7, 0, COLOR_NORMAL, S(STR_RESULTS_LIVES_LOST), res->lives_used);

    const char *grade = S(STR_GRADE_F);
    if (pct >= 90) grade = S(STR_GRADE_A);
    else if (pct >= 80) grade = S(STR_GRADE_B);
    else if (pct >= 70) grade = S(STR_GRADE_C);
    else if (pct >= 60) grade = S(STR_GRADE_D);

    attron(A_BOLD);
    ui_draw_centered(r, 9, COLOR_TITLE, S(STR_RESULTS_GRADE), grade);
    attroff(A_BOLD);

    ui_draw_centered(r, 11, COLOR_DIM, "%s", S(STR_RESULTS_CONTINUE));
    ui_refresh();
    while (ui_getch() == ERR);
}

int main(void)
{
    resolve_data_dir();

    ui_init();

    Progress *prog = progress_load();

    Settings settings;
    settings_load(&settings);

    str_load(strings_file(settings.language));

    Pool *pool = load_all(settings.language);
    if (!pool || pool->count == 0) {
        ui_message(ui_center_rect(40, 6), S(STR_MSG_FAILED_LOAD));
        ui_cleanup();
        return 1;
    }

    DifficultyState ds;
    difficulty_init(&ds, pool, &settings, prog);

    int last_language = settings.language;
    int n_items = 5;
    int selected = 0;

    while (1) {
        const char *menu_items[] = {
            S(STR_MENU_PLAY),
            S(STR_MENU_TOPICS),
            S(STR_MENU_SETTINGS),
            S(STR_MENU_PROGRESS),
            S(STR_MENU_QUIT)
        };

        ui_clear();
        int width = 36;
        int height = n_items + 8;
        Rect r = ui_center_rect(width, height);

        ui_draw_border(r, S(STR_MENU_TITLE));
        ui_draw_centered(r, 1, COLOR_INFO, "%s", S(STR_MENU_SUBTITLE));
        ui_draw_centered(r, 2, COLOR_DIM, "%s", S(STR_MENU_VERSION));

        int y = 4;
        for (int i = 0; i < n_items; i++) {
            if (i == selected) {
                ui_draw_centered(r, y + i, COLOR_HIGHLIGHT | A_BOLD, "> %s", menu_items[i]);
            } else {
                ui_draw_centered(r, y + i, COLOR_NORMAL, "  %s", menu_items[i]);
            }
        }

        int mastered = 0;
        if (prog) {
            int cnt = progress_count(prog);
            for (int i = 0; i < cnt; i++)
                if (prog[i].mastered) mastered++;
            if (cnt > 0) {
                ui_draw_text(r, y + n_items + 1, 0, COLOR_DIM,
                    S(STR_MENU_MASTERED), mastered, cnt);
            }
        }

        if (ds.current_max_diff > 1) {
            ui_draw_text(r, y + n_items + 2, 0, COLOR_INFO,
                S(STR_DIFFICULTY_LABEL), ds.current_max_diff);
        }

        ui_draw_centered(r, height - 1, COLOR_DIM, "%s", S(STR_MENU_NAV));
        ui_refresh();

        int ch = ui_getch();
        switch (ch) {
            case KEY_UP:
            case 'k':
                selected = (selected - 1 + n_items) % n_items;
                break;
            case KEY_DOWN:
            case 'j':
                selected = (selected + 1) % n_items;
                break;
            case '\n':
            case ' ':
                if (selected == 0) {
                    if (settings.language != last_language) {
                        str_load(strings_file(settings.language));
                        pool_free(pool);
                        pool = load_all(settings.language);
                        last_language = settings.language;
                        if (!pool) {
                            ui_message(ui_center_rect(40, 6), S(STR_MSG_FAILED_LOAD));
                            pool = load_all(LANG_ENGLISH);
                            last_language = LANG_ENGLISH;
                            settings.language = LANG_ENGLISH;
                        }
                        difficulty_init(&ds, pool, &settings, prog);
                    }
                    difficulty_update(&ds, pool, &settings, prog);
                    Pool *qp = pool_filter(pool, settings.categories, settings.sources);
                    if (!qp || qp->count == 0) {
                        ui_message(ui_center_rect(40, 6), S(STR_MSG_NO_COMMANDS));
                        pool_free(qp);
                        break;
                    }
                    pool_free(qp);
                    GameResult res = game_run(&settings, pool, &prog, &ds);
                    Rect res_r = ui_center_rect(46, 14);
                    show_results(&res, res_r);
                } else if (selected == 1) {
                    topics_menu(&settings);
                    settings_save(&settings);
                    difficulty_init(&ds, pool, &settings, prog);
                } else if (selected == 2) {
                    settings_menu(&settings);
                    settings_save(&settings);
                    if (settings.language != last_language) {
                        str_load(strings_file(settings.language));
                        pool_free(pool);
                        pool = load_all(settings.language);
                        last_language = settings.language;
                        if (!pool) {
                            ui_message(ui_center_rect(40, 6), S(STR_MSG_FAILED_LOAD));
                            pool = load_all(LANG_ENGLISH);
                            last_language = LANG_ENGLISH;
                            settings.language = LANG_ENGLISH;
                        }
                        difficulty_init(&ds, pool, &settings, prog);
                    }
                } else if (selected == 3) {
                    show_progress(prog);
                } else if (selected == 4) {
                    goto done;
                }
                break;
            case 'q':
            case 'Q':
                goto done;
        }
    }

done:
    settings_save(&settings);
    pool_free(pool);
    if (prog) progress_save(prog);
    progress_free(prog);
    ui_cleanup();
    return 0;
}
