#include "data.h"
#include "utils.h"
#include "settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

const char *category_ids[NUM_CATEGORIES] = {
    "file", "text", "net", "proc", "sys", "perm", "pkg", "dev",
    "shell", "hotkeys", "redir"
};

const char *source_ids[NUM_SOURCES] = {
    "core", "hotkeys", "builtins", "redir",
    "debian", "rhel", "arch", "macos"
};

int category_index(const char *id)
{
    for (int i = 0; i < NUM_CATEGORIES; i++)
        if (strcmp(category_ids[i], id) == 0)
            return i;
    return -1;
}

int source_index(const char *id)
{
    for (int i = 0; i < NUM_SOURCES; i++)
        if (strcmp(source_ids[i], id) == 0)
            return i;
    return -1;
}

Pool *pool_create(void)
{
    Pool *p = (Pool *)malloc(sizeof(Pool));
    if (!p) return NULL;
    p->count = 0;
    p->capacity = 256;
    p->entries = (Entry **)malloc(sizeof(Entry *) * (size_t)p->capacity);
    if (!p->entries) { free(p); return NULL; }
    return p;
}

void pool_add(Pool *p, Entry *e)
{
    if (p->count >= p->capacity) {
        p->capacity *= 2;
        Entry **tmp = (Entry **)realloc(p->entries,
            sizeof(Entry *) * (size_t)p->capacity);
        if (!tmp) return;
        p->entries = tmp;
    }
    p->entries[p->count++] = e;
}

void entry_free(Entry *e)
{
    if (!e) return;
    free(e->id);
    free(e->command);
    free(e->description);
    free(e->category);
    free(e);
}

Entry *entry_copy(const Entry *e)
{
    Entry *c = (Entry *)malloc(sizeof(Entry));
    if (!c) return NULL;
    c->id = utils_strdup(e->id);
    c->command = utils_strdup(e->command);
    c->description = utils_strdup(e->description);
    c->category = utils_strdup(e->category);
    c->difficulty = e->difficulty;
    c->source = e->source;
    return c;
}

void pool_free(Pool *p)
{
    if (!p) return;
    for (int i = 0; i < p->count; i++)
        entry_free(p->entries[i]);
    free(p->entries);
    free(p);
}

Pool *pool_load_master(const char *filename, int source_flag)
{
    FILE *f = fopen(filename, "r");
    if (!f) return NULL;

    Pool *p = pool_create();
    if (!p) { fclose(f); return NULL; }

    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), f)) {
        utils_strip_newline(line);
        if (line[0] == '#' || line[0] == '\0') continue;

        Entry *e = (Entry *)calloc(1, sizeof(Entry));
        if (!e) continue;

        char *tok = strtok(line, "\t");
        if (!tok) { free(e); continue; }
        e->id = utils_strdup(tok);

        tok = strtok(NULL, "\t");
        if (!tok) { entry_free(e); continue; }
        e->command = utils_strdup(tok);

        tok = strtok(NULL, "\t");
        if (!tok) { entry_free(e); continue; }
        e->category = utils_strdup(tok);

        tok = strtok(NULL, "\t");
        if (!tok) { entry_free(e); continue; }
        e->difficulty = atoi(tok);

        e->description = NULL;
        e->source = source_flag;
        pool_add(p, e);
    }

    fclose(f);
    return p;
}

void pool_apply_lang(Pool *p, const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f) return;

    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), f)) {
        utils_strip_newline(line);
        if (line[0] == '#' || line[0] == '\0') continue;

        char *id = strtok(line, "\t");
        char *desc = strtok(NULL, "\t");
        if (!id || !desc) continue;

        for (int i = 0; i < p->count; i++) {
            if (strcmp(p->entries[i]->id, id) == 0) {
                free(p->entries[i]->description);
                p->entries[i]->description = utils_strdup(desc);
                break;
            }
        }
    }

    fclose(f);
}

Pool *pool_filter(const Pool *src, int cat_mask, int source_mask)
{
    Pool *p = pool_create();
    if (!p) return NULL;

    for (int i = 0; i < src->count; i++) {
        Entry *e = src->entries[i];
        int idx = category_index(e->category);
        if (idx < 0) continue;
        if (!(cat_mask & (1 << idx))) continue;
        if (!(source_mask & e->source)) continue;

        Entry *copy = entry_copy(e);
        if (copy) pool_add(p, copy);
    }

    return p;
}
