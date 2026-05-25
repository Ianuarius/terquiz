#ifndef GAME_H
#define GAME_H

#include "settings.h"
#include "data.h"
#include "progress.h"
#include "difficulty.h"

typedef struct {
    int score;
    int total;
    int correct;
    int current_streak;
    int best_streak;
    int lives_used;
} GameResult;

GameResult game_run(const Settings *s, Pool *pool, Progress **prog, DifficultyState *ds);

#endif
