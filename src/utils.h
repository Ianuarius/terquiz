#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

int utils_randint(int min, int max);
void utils_shuffle(void *base, size_t nmemb, size_t size);
void utils_strip_newline(char *s);
char *utils_strdup(const char *s);
char *utils_base_command(const char *cmd);

#endif
