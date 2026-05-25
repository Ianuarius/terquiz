#ifndef PROGRESS_H
#define PROGRESS_H

#define PROGRESS_DIR ".terquiz"
#define PROGRESS_FILE "progress"
#define MASTERY_THRESHOLD 5

typedef struct {
    char *command;
    int streak;
    int total;
    int mastered;
    int typed_correct;
} Progress;

Progress *progress_load(void);
void progress_save(Progress *p);
Progress *progress_update(Progress *p, const char *base_cmd, int correct, int typed);
void progress_free(Progress *p);
int progress_count(Progress *p);
int progress_streak(Progress *p, const char *base_cmd);
int progress_total(Progress *p, const char *base_cmd);
int progress_typed_correct(Progress *p, const char *base_cmd);

#endif
