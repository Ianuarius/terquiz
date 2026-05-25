#ifndef UI_H
#define UI_H

#include <stdbool.h>

#define COLOR_NORMAL 1
#define COLOR_HIGHLIGHT 2
#define COLOR_CORRECT 3
#define COLOR_WRONG 4
#define COLOR_TITLE 5
#define COLOR_INFO 6
#define COLOR_SELECTED 7
#define COLOR_DIM 8

typedef struct {
    int x, y, w, h;
} Rect;

void ui_init(void);
void ui_cleanup(void);
int ui_getch(void);
void ui_clear(void);
void ui_refresh(void);

Rect ui_center_rect(int width, int height);
void ui_draw_border(Rect r, const char *title);
void ui_draw_text(Rect r, int line, int col, int color, const char *fmt, ...);
void ui_draw_centered(Rect r, int line, int color, const char *fmt, ...);
void ui_draw_fill(Rect r, int color);
void ui_draw_scrollbar(Rect r, int total, int visible, int offset);
int ui_menu(Rect r, const char *title, const char **items, int n, int selected);
bool ui_confirm(Rect r, const char *msg);
void ui_message(Rect r, const char *msg);
void ui_color_on(int c);
void ui_color_off(int c);

#endif
