#ifndef DATA_H
#define DATA_H

#define MAX_COMMANDS 2048
#define MAX_LINE_LEN 512
#define NUM_CATEGORIES 11

#define CAT_FILE  0
#define CAT_TEXT  1
#define CAT_NET   2
#define CAT_PROC  3
#define CAT_SYS   4
#define CAT_PERM  5
#define CAT_PKG   6
#define CAT_DEV   7
#define CAT_SHELL 8
#define CAT_HOTKEYS 9
#define CAT_REDIR 10

#define SOURCE_CORE     (1 << 0)
#define SOURCE_HOTKEYS  (1 << 1)
#define SOURCE_BUILTINS (1 << 2)
#define SOURCE_REDIR    (1 << 3)
#define SOURCE_DEBIAN   (1 << 4)
#define SOURCE_RHEL     (1 << 5)
#define SOURCE_ARCH     (1 << 6)
#define SOURCE_MACOS    (1 << 7)
#define NUM_SOURCES     8
#define SOURCE_ALL      0xFF

typedef struct {
    char *id;
    char *command;
    char *description;
    char *category;
    int difficulty;
    int source;
} Entry;

typedef struct {
    Entry **entries;
    int count;
    int capacity;
} Pool;

extern const char *category_ids[NUM_CATEGORIES];
extern const char *source_ids[NUM_SOURCES];

int category_index(const char *id);
int source_index(const char *id);
Pool *pool_create(void);
void pool_add(Pool *p, Entry *e);
void pool_free(Pool *p);
void entry_free(Entry *e);
Entry *entry_copy(const Entry *e);
Pool *pool_load_master(const char *filename, int source_flag);
void pool_apply_lang(Pool *p, const char *filename);
Pool *pool_filter(const Pool *src, int cat_mask, int source_mask);

#endif
