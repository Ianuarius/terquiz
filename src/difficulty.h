#ifndef DIFFICULTY_H
#define DIFFICULTY_H

#include "data.h"
#include "progress.h"
#include "settings.h"

#define NUM_DIFFICULTIES 20
#define UNLOCK_PCT 80
#define MIN_MASTERED_FOR_UNLOCK 3

typedef struct {
    int current_max_diff;
    int total_at_diff[NUM_DIFFICULTIES + 1];
    int mastered_at_diff[NUM_DIFFICULTIES + 1];
} DifficultyState;

void difficulty_init(DifficultyState *ds, const Pool *pool, const Settings *s, Progress *prog);
void difficulty_update(DifficultyState *ds, const Pool *pool, const Settings *s, Progress *prog);
int difficulty_pick_level(const DifficultyState *ds, const Pool *pool, Progress *prog);

#endif
