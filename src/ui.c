#include "ui.h"
#include "str.h"
#include <ncurses.h>
#include <stdarg.h>
#include <string.h>
#include <locale.h>

static WINDOW *main_win;

void ui_init(void)
{
    setlocale(LC_ALL, "");
    main_win = initscr();
    cbreak();
    noecho();
    keypad(main_win, TRUE);
    curs_set(0);
    timeout(-1);

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(COLOR_NORMAL, -1, -1);
        init_pair(COLOR_HIGHLIGHT, COLOR_BLACK, COLOR_CYAN);
        init_pair(COLOR_CORRECT, COLOR_GREEN, -1);
        init_pair(COLOR_WRONG, COLOR_RED, -1);
        init_pair(COLOR_TITLE, COLOR_YELLOW, -1);
        init_pair(COLOR_INFO, COLOR_CYAN, -1);
        init_pair(COLOR_SELECTED, COLOR_BLACK, COLOR_WHITE);
        init_pair(COLOR_DIM, COLOR_WHITE, -1);
    }
}

void ui_cleanup(void)
{
    if (main_win) {
        endwin();
        main_win = NULL;
    }
}

int ui_getch(void)
{
    return getch();
}

void ui_clear(void)
{
    clear();
}

void ui_refresh(void)
{
    refresh();
}

Rect ui_center_rect(int width, int height)
{
    int maxy, maxx;
    getmaxyx(main_win, maxy, maxx);
    Rect r;
    r.w = width < maxx ? width : maxx;
    r.h = height < maxy ? height : maxy;
    r.x = (maxx - r.w) / 2;
    r.y = (maxy - r.h) / 2;
    return r;
}

void ui_draw_border(Rect r, const char *title)
{
    int maxy, maxx;
    getmaxyx(main_win, maxy, maxx);

    int x1 = r.x, y1 = r.y;
    int x2 = r.x + r.w - 1, y2 = r.y + r.h - 1;

    if (x2 >= maxx) x2 = maxx - 1;
    if (y2 >= maxy) y2 = maxy - 1;

    for (int y = y1; y <= y2; y++) {
        mvhline(y, x1, ' ', r.w);
    }

    wattron(main_win, A_BOLD);
    mvaddch(y1, x1, ACS_ULCORNER);
    mvhline(y1, x1 + 1, ACS_HLINE, r.w - 2);
    mvaddch(y1, x2, ACS_URCORNER);

    for (int y = y1 + 1; y < y2; y++) {
        mvaddch(y, x1, ACS_VLINE);
        mvaddch(y, x2, ACS_VLINE);
    }

    mvaddch(y2, x1, ACS_LLCORNER);
    mvhline(y2, x1 + 1, ACS_HLINE, r.w - 2);
    mvaddch(y2, x2, ACS_LRCORNER);
    wattroff(main_win, A_BOLD);

    if (title) {
        wattron(main_win, COLOR_PAIR(COLOR_TITLE) | A_BOLD);
        int tx = x1 + (r.w - (int)strlen(title)) / 2;
        if (tx < x1 + 1) tx = x1 + 1;
        mvaddstr(y1, tx, title);
        wattroff(main_win, COLOR_PAIR(COLOR_TITLE) | A_BOLD);
    }
}

void ui_draw_text(Rect r, int line, int col, int color, const char *fmt, ...)
{
    int maxy, maxx;
    getmaxyx(main_win, maxy, maxx);

    int y = r.y + 1 + line;
    int x = r.x + 1 + col;
    if (y >= maxy || x >= maxx) return;

    va_list args;
    char buf[512];
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    wattron(main_win, COLOR_PAIR(color));
    mvaddstr(y, x, buf);
    wattroff(main_win, COLOR_PAIR(color));
}

void ui_draw_centered(Rect r, int line, int color, const char *fmt, ...)
{
    va_list args;
    char buf[512];
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    int len = (int)strlen(buf);
    int col = (r.w - 2 - len) / 2;
    if (col < 0) col = 0;
    ui_draw_text(r, line, col, color, "%s", buf);
}

void ui_draw_fill(Rect r, int color)
{
    int maxy, maxx;
    getmaxyx(main_win, maxy, maxx);

    wattron(main_win, COLOR_PAIR(color));
    for (int y = r.y; y < r.y + r.h && y < maxy; y++)
        for (int x = r.x; x < r.x + r.w && x < maxx; x++)
            mvaddch(y, x, ' ');
    wattroff(main_win, COLOR_PAIR(color));
}

void ui_draw_scrollbar(Rect r, int total, int visible, int offset)
{
    if (total <= 0 || visible <= 0) return;
    int bar_h = r.h - 2;
    if (bar_h < 1) return;

    int thumb_size = (visible * bar_h) / total;
    if (thumb_size < 1) thumb_size = 1;
    int thumb_pos = (offset * (bar_h - thumb_size)) / (total - visible > 0 ? total - visible : 1);

    int x = r.x + r.w - 2;
    for (int i = 0; i < bar_h; i++) {
        int y = r.y + 1 + i;
        if (i >= thumb_pos && i < thumb_pos + thumb_size)
            mvaddch(y, x, ' ' | A_REVERSE);
        else
            mvaddch(y, x, ACS_CKBOARD);
    }
}

int ui_menu(Rect r, const char *title, const char **items, int n, int selected)
{
    ui_draw_border(r, title);

    int start = 0;
    int max_visible = r.h - 3;
    if (max_visible < 1) max_visible = 1;

    if (selected >= start + max_visible)
        start = selected - max_visible + 1;
    if (selected < start)
        start = selected;

    for (int i = 0; i < max_visible && i < n; i++) {
        int idx = start + i;
        int y = r.y + 1 + i;
        int x = r.x + 2;
        if (idx == selected) {
            wattron(main_win, COLOR_PAIR(COLOR_HIGHLIGHT) | A_BOLD);
            mvprintw(y, x - 1, "> %s", items[idx]);
            wattroff(main_win, COLOR_PAIR(COLOR_HIGHLIGHT) | A_BOLD);
        } else {
            mvprintw(y, x, "  %s", items[idx]);
        }
    }

    if (n > max_visible)
        ui_draw_scrollbar(r, n, max_visible, start);

    return selected;
}

bool ui_confirm(Rect r, const char *msg)
{
    ui_draw_border(r, S(STR_CONFIRM_TITLE));
    ui_draw_centered(r, 1, COLOR_NORMAL, "%s", msg);
    ui_draw_centered(r, 3, COLOR_INFO, "%s", S(STR_CONFIRM_YES));
    ui_refresh();

    while (1) {
        int ch = ui_getch();
        if (ch == '\n' || ch == 'y' || ch == 'Y') return true;
        if (ch == 'q' || ch == 'Q' || ch == 27) return false;
    }
}

void ui_message(Rect r, const char *msg)
{
    ui_draw_border(r, S(STR_INFO_TITLE));
    ui_draw_centered(r, 1, COLOR_NORMAL, "%s", msg);
    ui_draw_centered(r, 3, COLOR_INFO, "%s", S(STR_INFO_ANY_KEY));
    ui_refresh();
    while (ui_getch() == ERR);
}

void ui_color_on(int c)
{
    wattron(main_win, COLOR_PAIR(c));
}

void ui_color_off(int c)
{
    wattroff(main_win, COLOR_PAIR(c));
}
