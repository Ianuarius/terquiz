#include "str.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *strings[STR_COUNT];

static const struct {
    const char *name;
    int id;
} keymap[] = {
    {"menu_title", STR_MENU_TITLE},
    {"menu_subtitle", STR_MENU_SUBTITLE},
    {"menu_version", STR_MENU_VERSION},
    {"menu_play", STR_MENU_PLAY},
    {"menu_settings", STR_MENU_SETTINGS},
    {"menu_progress", STR_MENU_PROGRESS},
    {"menu_quit", STR_MENU_QUIT},
    {"menu_topics", STR_MENU_TOPICS},
    {"menu_nav", STR_MENU_NAV},
    {"menu_mastered", STR_MENU_MASTERED},
    {"settings_title", STR_SETTINGS_TITLE},
    {"settings_mode", STR_SETTINGS_MODE},
    {"settings_length", STR_SETTINGS_LENGTH},
    {"settings_timer", STR_SETTINGS_TIMER},
    {"settings_lives", STR_SETTINGS_LIVES},
    {"settings_practice", STR_SETTINGS_PRACTICE},
    {"settings_language", STR_SETTINGS_LANGUAGE},
    {"settings_nav", STR_SETTINGS_NAV},
    {"settings_back", STR_SETTINGS_BACK},
    {"settings_guess_cmd", STR_SETTINGS_GUESS_CMD},
    {"settings_guess_def", STR_SETTINGS_GUESS_DEF},
    {"settings_guess_mixed", STR_SETTINGS_GUESS_MIXED},
    {"settings_timer_off", STR_SETTINGS_TIMER_OFF},
    {"settings_lives_off", STR_SETTINGS_LIVES_OFF},
    {"settings_practice_off", STR_SETTINGS_PRACTICE_OFF},
    {"settings_lang_eng", STR_SETTINGS_LANG_ENG},
    {"settings_lang_fi", STR_SETTINGS_LANG_FI},
    {"settings_length_10", STR_SETTINGS_LENGTH_10},
    {"settings_length_20", STR_SETTINGS_LENGTH_20},
    {"settings_length_50", STR_SETTINGS_LENGTH_50},
    {"settings_length_endless", STR_SETTINGS_LENGTH_ENDLESS},
    {"topics_title", STR_TOPICS_TITLE},
    {"topics_categories", STR_TOPICS_CATEGORIES},
    {"topics_packs", STR_TOPICS_PACKS},
    {"topics_nav", STR_TOPICS_NAV},
    {"topics_back", STR_TOPICS_BACK},
    {"source_core", STR_SOURCE_CORE},
    {"source_hotkeys", STR_SOURCE_HOTKEYS},
    {"source_builtins", STR_SOURCE_BUILTINS},
    {"source_redir", STR_SOURCE_REDIR},
    {"source_debian", STR_SOURCE_DEBIAN},
    {"source_rhel", STR_SOURCE_RHEL},
    {"source_arch", STR_SOURCE_ARCH},
    {"source_macos", STR_SOURCE_MACOS},
    {"game_score", STR_GAME_SCORE},
    {"game_streak", STR_GAME_STREAK},
    {"game_lives", STR_GAME_LIVES},
    {"game_question_fmt", STR_GAME_QUESTION_FMT},
    {"game_question_n", STR_GAME_QUESTION_N},
    {"game_guess_cmd", STR_GAME_GUESS_CMD},
    {"game_guess_def", STR_GAME_GUESS_DEF},
    {"game_time_limit", STR_GAME_TIME_LIMIT},
    {"game_select", STR_GAME_SELECT},
    {"game_select_typed", STR_GAME_SELECT_TYPED},
    {"game_type_prompt", STR_GAME_TYPE_PROMPT},
    {"results_title", STR_RESULTS_TITLE},
    {"results_header", STR_RESULTS_HEADER},
    {"results_score", STR_RESULTS_SCORE},
    {"results_correct", STR_RESULTS_CORRECT},
    {"results_accuracy", STR_RESULTS_ACCURACY},
    {"results_best_streak", STR_RESULTS_BEST_STREAK},
    {"results_lives_lost", STR_RESULTS_LIVES_LOST},
    {"results_grade", STR_RESULTS_GRADE},
    {"results_continue", STR_RESULTS_CONTINUE},
    {"progress_title", STR_PROGRESS_TITLE},
    {"progress_header", STR_PROGRESS_HEADER},
    {"progress_mastered", STR_PROGRESS_MASTERED},
    {"progress_learning", STR_PROGRESS_LEARNING},
    {"progress_new", STR_PROGRESS_NEW},
    {"progress_nav", STR_PROGRESS_NAV},
    {"progress_empty", STR_PROGRESS_EMPTY},
    {"msg_no_commands", STR_MSG_NO_COMMANDS},
    {"msg_failed_load", STR_MSG_FAILED_LOAD},
    {"confirm_title", STR_CONFIRM_TITLE},
    {"confirm_yes", STR_CONFIRM_YES},
    {"info_title", STR_INFO_TITLE},
    {"info_any_key", STR_INFO_ANY_KEY},
    {"cat_file_operations", STR_CAT_FILE_OPERATIONS},
    {"cat_text_processing", STR_CAT_TEXT_PROCESSING},
    {"cat_networking", STR_CAT_NETWORKING},
    {"cat_process_management", STR_CAT_PROCESS_MANAGEMENT},
    {"cat_system_information", STR_CAT_SYSTEM_INFORMATION},
    {"cat_users_permissions", STR_CAT_USERS_PERMISSIONS},
    {"cat_package_management", STR_CAT_PACKAGE_MANAGEMENT},
    {"cat_development_tools", STR_CAT_DEVELOPMENT_TOOLS},
    {"cat_shell_basics", STR_CAT_SHELL_BASICS},
    {"cat_terminal_hotkeys", STR_CAT_TERMINAL_HOTKEYS},
    {"cat_redir_pipes", STR_CAT_REDIR_PIPES},
    {"difficulty_label", STR_DIFFICULTY_LABEL},
    {"difficulty_level", STR_DIFFICULTY_LEVEL},
    {"result_correct", STR_RESULT_CORRECT},
    {"result_wrong_cmd", STR_RESULT_WRONG_CMD},
    {"result_wrong_def", STR_RESULT_WRONG_DEF},
    {"result_timed_out", STR_RESULT_TIMED_OUT},
    {"result_retry_correct", STR_RESULT_RETRY_CORRECT},
    {"result_retry_wrong_cmd", STR_RESULT_RETRY_WRONG_CMD},
    {"result_retry_wrong_def", STR_RESULT_RETRY_WRONG_DEF},
    {"result_retry_timed_out", STR_RESULT_RETRY_TIMED_OUT},
    {"game_retry_tag", STR_GAME_RETRY_TAG},
    {"game_option_none", STR_GAME_OPTION_NONE},
    {"settings_timer_5", STR_SETTINGS_TIMER_5},
    {"settings_timer_10", STR_SETTINGS_TIMER_10},
    {"settings_timer_15", STR_SETTINGS_TIMER_15},
    {"settings_timer_30", STR_SETTINGS_TIMER_30},
    {"settings_lives_1", STR_SETTINGS_LIVES_1},
    {"settings_lives_3", STR_SETTINGS_LIVES_3},
    {"settings_lives_5", STR_SETTINGS_LIVES_5},
    {"grade_f", STR_GRADE_F},
    {"grade_d", STR_GRADE_D},
    {"grade_c", STR_GRADE_C},
    {"grade_b", STR_GRADE_B},
    {"grade_a", STR_GRADE_A},
};

static int lookup_key(const char *name)
{
    int n = sizeof(keymap) / sizeof(keymap[0]);
    for (int i = 0; i < n; i++)
        if (strcmp(keymap[i].name, name) == 0)
            return keymap[i].id;
    return -1;
}

static void fill_empty(void)
{
    for (int i = 0; i < STR_COUNT; i++)
        if (!strings[i])
            strings[i] = "???";
}

void str_load(const char *filename)
{
    for (int i = 0; i < STR_COUNT; i++)
        strings[i] = NULL;

    FILE *f = fopen(filename, "r");
    if (!f) {
        fill_empty();
        return;
    }

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        utils_strip_newline(line);
        if (line[0] == '#' || line[0] == '\0') continue;

        char *key = strtok(line, "\t");
        char *val = strtok(NULL, "\t");
        if (!key || !val) continue;

        int id = lookup_key(key);
        if (id >= 0)
            strings[id] = utils_strdup(val);
    }

    fclose(f);
    fill_empty();
}

const char *S(int key)
{
    if (key < 0 || key >= STR_COUNT) return "???";
    return strings[key] ? strings[key] : "???";
}
