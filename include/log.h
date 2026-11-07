#ifndef GAME_LOG_H
#define GAME_LOG_H

typedef enum {
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_DEBUG = 2
} log_level_t;

void log_set_level(log_level_t level);
int log_parse_level(const char *text, log_level_t *level);
void log_message(log_level_t level, const char *format, ...);

#define LOG_ERROR(...) log_message(LOG_LEVEL_ERROR, __VA_ARGS__)
#define LOG_INFO(...) log_message(LOG_LEVEL_INFO, __VA_ARGS__)

#ifdef ENABLE_DEBUG_LOG
#define LOG_DEBUG(...) log_message(LOG_LEVEL_DEBUG, __VA_ARGS__)
#else
#define LOG_DEBUG(...) ((void)0)
#endif

#endif
