#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int utils_randint(int min, int max)
{
    static int seeded = 0;
    if (!seeded) {
        seeded = 1;
        srand((unsigned)(time(NULL) ^ getpid()));
    }
    if (min >= max) return min;
    return min + rand() % (max - min + 1);
}

void utils_shuffle(void *base, size_t nmemb, size_t size)
{
    if (nmemb < 2) return;
    char *arr = (char *)base;
    for (size_t i = nmemb - 1; i > 0; i--) {
        size_t j = (size_t)utils_randint(0, (int)i);
        if (i != j) {
            char tmp;
            for (size_t k = 0; k < size; k++) {
                tmp = arr[i * size + k];
                arr[i * size + k] = arr[j * size + k];
                arr[j * size + k] = tmp;
            }
        }
    }
}

void utils_strip_newline(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r'))
        s[--len] = '\0';
}

char *utils_strdup(const char *s)
{
    size_t len = strlen(s) + 1;
    char *copy = (char *)malloc(len);
    if (copy) memcpy(copy, s, len);
    return copy;
}

char *utils_base_command(const char *cmd)
{
    const char *space = strchr(cmd, ' ');
    if (!space)
        return utils_strdup(cmd);

    size_t len = (size_t)(space - cmd);
    char *base = (char *)malloc(len + 1);
    if (base) {
        memcpy(base, cmd, len);
        base[len] = '\0';
    }
    return base;
}
