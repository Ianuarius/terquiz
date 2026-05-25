#include "topics.h"
#include "ui.h"
#include "data.h"
#include "str.h"
#include <ncurses.h>
#include <string.h>

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

static const int cat_str_ids[NUM_CATEGORIES] = {
    STR_CAT_FILE_OPERATIONS,
    STR_CAT_TEXT_PROCESSING,
    STR_CAT_NETWORKING,
    STR_CAT_PROCESS_MANAGEMENT,
    STR_CAT_SYSTEM_INFORMATION,
    STR_CAT_USERS_PERMISSIONS,
    STR_CAT_PACKAGE_MANAGEMENT,
    STR_CAT_DEVELOPMENT_TOOLS,
    STR_CAT_SHELL_BASICS,
    STR_CAT_TERMINAL_HOTKEYS,
    STR_CAT_REDIR_PIPES,
};

static const int src_str_ids[NUM_SOURCES] = {
    STR_SOURCE_CORE,
    STR_SOURCE_HOTKEYS,
    STR_SOURCE_BUILTINS,
    STR_SOURCE_REDIR,
    STR_SOURCE_DEBIAN,
    STR_SOURCE_RHEL,
    STR_SOURCE_ARCH,
    STR_SOURCE_MACOS,
};

/* Total conceptual rows: header, blank, 11 cats, blank, header, 8 sources = 23 */
#define TOP_HEADER_ROW  0
#define TOP_BLANK1_ROW  1
#define CAT_START_ROW   2
#define CAT_END_ROW     (CAT_START_ROW + NUM_CATEGORIES - 1)
#define TOP_BLANK2_ROW  (CAT_END_ROW + 1)
#define SRC_HEADER_ROW  (TOP_BLANK2_ROW + 1)
#define SRC_START_ROW   (SRC_HEADER_ROW + 1)
#define SRC_END_ROW     (SRC_START_ROW + NUM_SOURCES - 1)
#define TOTAL_ROWS      (SRC_END_ROW + 1)

int topics_menu(Settings *s)
{
    int current_row = CAT_START_ROW;
    int scroll = 0;

    while (1) {
        ui_clear();
        int maxy, maxx;
        getmaxyx(stdscr, maxy, maxx);

        int width = 54;
        if (width > maxx - 4) width = maxx - 4;
        int height = maxy - 2;
        if (height > 28) height = 28;
        if (height < 10) height = 10;

        int content_rows = height - 4;
        if (content_rows < 1) content_rows = 1;

        if (current_row < scroll) scroll = current_row;
        if (current_row >= scroll + content_rows)
            scroll = current_row - content_rows + 1;

        int ox = (maxx - width) / 2;
        int oy = (maxy - height) / 2;
        if (ox < 0) ox = 0;
        if (oy < 0) oy = 0;

        Rect r = {ox, oy, width, height};
        ui_draw_border(r, S(STR_TOPICS_TITLE));

        int y = oy + 1;
        int x = ox + 2;

        for (int i = 0; i < content_rows; i++) {
            int row = scroll + i;
            if (row >= TOTAL_ROWS) break;

            if (row == TOP_HEADER_ROW) {
                attron(COLOR_PAIR(COLOR_INFO) | A_BOLD);
                mvprintw(y, x, "%s", S(STR_TOPICS_CATEGORIES));
                attroff(COLOR_PAIR(COLOR_INFO) | A_BOLD);
            } else if (row == TOP_BLANK1_ROW || row == TOP_BLANK2_ROW) {
                /* blank line */
            } else if (row == SRC_HEADER_ROW) {
                attron(COLOR_PAIR(COLOR_INFO) | A_BOLD);
                mvprintw(y, x, "%s", S(STR_TOPICS_PACKS));
                attroff(COLOR_PAIR(COLOR_INFO) | A_BOLD);
            } else if (row >= CAT_START_ROW && row <= CAT_END_ROW) {
                int idx = row - CAT_START_ROW;
                bool checked = (s->categories >> idx) & 1;
                bool active = (current_row == row);

                mvaddch(y, x, checked ? '[' | A_BOLD : '[');
                if (checked) {
                    attron(A_BOLD);
                    mvaddch(y, x + 1, 'X');
                    attroff(A_BOLD);
                } else {
                    mvaddch(y, x + 1, ' ');
                }
                mvaddch(y, x + 2, ']');

                if (active) attron(COLOR_PAIR(COLOR_HIGHLIGHT));
                mvprintw(y, x + 4, "%s", S(cat_str_ids[idx]));
                if (active) attroff(COLOR_PAIR(COLOR_HIGHLIGHT));
            } else if (row >= SRC_START_ROW && row <= SRC_END_ROW) {
                int idx = row - SRC_START_ROW;
                bool checked = (s->sources >> idx) & 1;
                bool active = (current_row == row);

                mvaddch(y, x, checked ? '[' | A_BOLD : '[');
                if (checked) {
                    attron(A_BOLD);
                    mvaddch(y, x + 1, 'X');
                    attroff(A_BOLD);
                } else {
                    mvaddch(y, x + 1, ' ');
                }
                mvaddch(y, x + 2, ']');

                if (active) attron(COLOR_PAIR(COLOR_HIGHLIGHT));
                mvprintw(y, x + 4, "%s", S(src_str_ids[idx]));
                if (active) attroff(COLOR_PAIR(COLOR_HIGHLIGHT));
            }
            y++;
        }

        if (TOTAL_ROWS > content_rows) {
            Rect sr = {ox, oy + 1, width, content_rows};
            ui_draw_scrollbar(sr, TOTAL_ROWS, content_rows, scroll);
        }

        y = oy + height - 3;
        attron(COLOR_PAIR(COLOR_DIM));
        mvprintw(y, x, "%s", S(STR_TOPICS_NAV));
        y++;
        attron(COLOR_PAIR(COLOR_TITLE) | A_BOLD);
        mvprintw(y, x, "%s", S(STR_TOPICS_BACK));
        attroff(COLOR_PAIR(COLOR_TITLE) | A_BOLD);

        ui_refresh();

        int ch = ui_getch();
        if (ch == 'q' || ch == 'Q') break;

        if (ch == KEY_DOWN || ch == 'j') {
            do {
                current_row++;
                if (current_row >= TOTAL_ROWS) current_row = CAT_START_ROW;
            } while (current_row == TOP_HEADER_ROW || current_row == TOP_BLANK1_ROW ||
                     current_row == TOP_BLANK2_ROW || current_row == SRC_HEADER_ROW);
        } else if (ch == KEY_UP || ch == 'k') {
            do {
                current_row--;
                if (current_row < 0) current_row = SRC_END_ROW;
            } while (current_row == TOP_HEADER_ROW || current_row == TOP_BLANK1_ROW ||
                     current_row == TOP_BLANK2_ROW || current_row == SRC_HEADER_ROW);
        } else if (ch == ' ' || ch == '\n') {
            if (current_row >= CAT_START_ROW && current_row <= CAT_END_ROW) {
                int idx = current_row - CAT_START_ROW;
                s->categories ^= (1 << idx);
            } else if (current_row >= SRC_START_ROW && current_row <= SRC_END_ROW) {
                int idx = current_row - SRC_START_ROW;
                s->sources ^= (1 << idx);
            }
        }
    }

    return 0;
}
