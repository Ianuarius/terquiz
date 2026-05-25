#include "progress.h"
#include "utils.h"
#include "data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

static char *progress_path(void)
{
    const char *home = getenv("HOME");
    if (!home) return NULL;

    size_t len = strlen(home) + strlen(PROGRESS_DIR) + strlen(PROGRESS_FILE) + 3;
    char *path = (char *)malloc(len);
    if (!path) return NULL;
    snprintf(path, len, "%s/%s/%s", home, PROGRESS_DIR, PROGRESS_FILE);
    return path;
}

static void ensure_dir(void)
{
    const char *home = getenv("HOME");
    if (!home) return;
    size_t len = strlen(home) + strlen(PROGRESS_DIR) + 2;
    char *dir = (char *)malloc(len);
    if (!dir) return;
    snprintf(dir, len, "%s/%s", home, PROGRESS_DIR);
    mkdir(dir, 0700);
    free(dir);
}

Progress *progress_load(void)
{
    Progress *p = (Progress *)calloc(1, sizeof(Progress));
    if (!p) return NULL;

    p->command = NULL;
    p->streak = 0;
    p->total = 0;

    char *path = progress_path();
    if (!path) return p;

    FILE *f = fopen(path, "r");
    free(path);
    if (!f) return p;

    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), f)) {
        utils_strip_newline(line);
        if (line[0] == '#' || line[0] == '\0') continue;

        char *cmd = strtok(line, ":");
        char *streak_str = strtok(NULL, ":");
        char *total_str = strtok(NULL, ":");
        char *mastered_str = strtok(NULL, ":");
        char *typed_str = strtok(NULL, ":");

        if (cmd && streak_str) {
            Progress *entry = (Progress *)calloc(1, sizeof(Progress));
            if (!entry) continue;
            entry->command = utils_strdup(cmd);
            entry->streak = atoi(streak_str);
            entry->total = total_str ? atoi(total_str) : 0;
            entry->mastered = mastered_str ? atoi(mastered_str) : 0;
            entry->typed_correct = typed_str ? atoi(typed_str) : 0;

            Progress *newp = (Progress *)realloc(p,
                sizeof(Progress) * (progress_count(p) + 2));
            if (!newp) { free(entry->command); free(entry); continue; }
            p = newp;
            int idx = progress_count(p);
            p[idx].command = entry->command;
            p[idx].streak = entry->streak;
            p[idx].total = entry->total;
            p[idx].mastered = entry->mastered;
            p[idx].typed_correct = entry->typed_correct;
            p[idx + 1].command = NULL;
            p[idx + 1].streak = 0;
            p[idx + 1].total = 0;
            p[idx + 1].mastered = 0;
            p[idx + 1].typed_correct = 0;
            free(entry);
        }
    }

    fclose(f);
    return p;
}

void progress_save(Progress *p)
{
    ensure_dir();
    char *path = progress_path();
    if (!path) return;

    FILE *f = fopen(path, "w");
    free(path);
    if (!f) return;

    fprintf(f, "# command:streak:total:mastered:typed_correct\n");
    for (int i = 0; p[i].command; i++) {
        fprintf(f, "%s:%d:%d:%d:%d\n",
            p[i].command, p[i].streak, p[i].total, p[i].mastered, p[i].typed_correct);
    }

    fclose(f);
}

Progress *progress_update(Progress *p, const char *base_cmd, int correct, int typed)
{
    int i;
    for (i = 0; p[i].command; i++) {
        if (strcmp(p[i].command, base_cmd) == 0) {
            p[i].total++;
            if (correct) {
                p[i].streak++;
                if (typed)
                    p[i].typed_correct++;
                if (p[i].streak >= MASTERY_THRESHOLD)
                    p[i].mastered = 1;
            } else {
                p[i].streak = 0;
            }
            progress_save(p);
            return p;
        }
    }

    size_t new_count = (size_t)i + 2;
    Progress *newp = (Progress *)realloc(p, sizeof(Progress) * new_count);
    if (!newp) return p;
    newp[i].command = utils_strdup(base_cmd);
    newp[i].total = 1;
    newp[i].streak = correct ? 1 : 0;
    newp[i].mastered = 0;
    newp[i].typed_correct = (correct && typed) ? 1 : 0;
    newp[i + 1].command = NULL;
    newp[i + 1].streak = 0;
    newp[i + 1].total = 0;
    newp[i + 1].mastered = 0;
    newp[i + 1].typed_correct = 0;
    progress_save(newp);
    return newp;
}

void progress_free(Progress *p)
{
    if (!p) return;
    for (int i = 0; p[i].command; i++)
        free(p[i].command);
    free(p);
}

int progress_count(Progress *p)
{
    int i = 0;
    while (p[i].command) i++;
    return i;
}

int progress_streak(Progress *p, const char *base_cmd)
{
    for (int i = 0; p[i].command; i++)
        if (strcmp(p[i].command, base_cmd) == 0)
            return p[i].streak;
    return 0;
}

int progress_total(Progress *p, const char *base_cmd)
{
    for (int i = 0; p[i].command; i++)
        if (strcmp(p[i].command, base_cmd) == 0)
            return p[i].total;
    return 0;
}

int progress_typed_correct(Progress *p, const char *base_cmd)
{
    for (int i = 0; p[i].command; i++)
        if (strcmp(p[i].command, base_cmd) == 0)
            return p[i].typed_correct;
    return 0;
}
