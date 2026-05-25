#include "difficulty.h"
#include "utils.h"
#include <string.h>
#include <stdlib.h>

static void recount(DifficultyState *ds, const Pool *pool, const Settings *s, Progress *prog)
{
    for (int d = 1; d <= NUM_DIFFICULTIES; d++) {
        ds->total_at_diff[d] = 0;
        ds->mastered_at_diff[d] = 0;
    }

    for (int i = 0; i < pool->count; i++) {
        Entry *e = pool->entries[i];
        int idx = category_index(e->category);
        if (idx < 0) continue;
        if (!(s->categories & (1 << idx))) continue;
        if (!(s->sources & e->source)) continue;
        int d = e->difficulty;
        if (d < 1) d = 1;
        if (d > NUM_DIFFICULTIES) d = NUM_DIFFICULTIES;
        ds->total_at_diff[d]++;

        if (prog) {
            char *base = utils_base_command(e->command);
            int streak = progress_streak(prog, base);
            free(base);
            if (streak >= 5)
                ds->mastered_at_diff[d]++;
        }
    }
}

void difficulty_init(DifficultyState *ds, const Pool *pool, const Settings *s, Progress *prog)
{
    for (int d = 0; d <= NUM_DIFFICULTIES; d++) {
        ds->total_at_diff[d] = 0;
        ds->mastered_at_diff[d] = 0;
    }
    ds->current_max_diff = 1;
    recount(ds, pool, s, prog);
    for (int d = 1; d < NUM_DIFFICULTIES; d++) {
        int active = ds->total_at_diff[d];
        int mastered = ds->mastered_at_diff[d];
        if (active > 0 && mastered >= MIN_MASTERED_FOR_UNLOCK) {
            int pct = (mastered * 100) / active;
            if (pct >= UNLOCK_PCT)
                ds->current_max_diff = d + 1;
            else
                break;
        } else {
            break;
        }
    }
}

void difficulty_update(DifficultyState *ds, const Pool *pool, const Settings *s, Progress *prog)
{
    recount(ds, pool, s, prog);
    for (int d = 1; d < NUM_DIFFICULTIES; d++) {
        int active = ds->total_at_diff[d];
        int mastered = ds->mastered_at_diff[d];
        if (active > 0 && mastered >= MIN_MASTERED_FOR_UNLOCK) {
            int pct = (mastered * 100) / active;
            if (pct >= UNLOCK_PCT && d >= ds->current_max_diff)
                ds->current_max_diff = d + 1;
        }
    }
    if (ds->current_max_diff > NUM_DIFFICULTIES)
        ds->current_max_diff = NUM_DIFFICULTIES;
}

int difficulty_pick_level(const DifficultyState *ds, const Pool *pool, Progress *prog)
{
    int max = ds->current_max_diff;
    if (max < 1) max = 1;
    if (max > NUM_DIFFICULTIES) max = NUM_DIFFICULTIES;

    for (int d = max; d >= 1; d--) {
        int unmastered = 0;
        for (int i = 0; i < pool->count; i++) {
            Entry *e = pool->entries[i];
            if (e->difficulty != d) continue;
            if (prog) {
                char *base = utils_base_command(e->command);
                int streak = progress_streak(prog, base);
                free(base);
                if (streak < 5) unmastered++;
            } else {
                unmastered++;
            }
            if (unmastered > 0) return d;
        }
    }
    return max;
}
