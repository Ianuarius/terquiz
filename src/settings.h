#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>

#define GUESS_CMD 0
#define GUESS_DEF 1
#define GUESS_MIXED 2

#define LENGTH_ENDLESS 0

#define TIMER_OFF 0
#define LIVES_OFF 0

#define LANG_ENGLISH 0
#define LANG_FINNISH 1

typedef struct {
    int categories;         /* 11-bit bitmask */
    int sources;            /* 8-bit bitmask */
    int guess_mode;         /* GUESS_CMD, GUESS_DEF, GUESS_MIXED */
    int quiz_length;
    int timer_seconds;
    int lives;
    bool practice_mode;
    int language;
} Settings;

void settings_defaults(Settings *s);
void settings_load(Settings *s);
void settings_save(const Settings *s);
int settings_menu(Settings *s);

#endif
