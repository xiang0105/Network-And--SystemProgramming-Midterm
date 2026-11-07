#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static log_level_t current_level = LOG_LEVEL_INFO;

void log_set_level(log_level_t level)
{
    current_level = level;
}

int log_parse_level(const char *text, log_level_t *level)
{
    if (text == NULL || level == NULL) {
        return -1;
    }
    if (strcmp(text, "error") == 0) {
        *level = LOG_LEVEL_ERROR;
    } else if (strcmp(text, "info") == 0) {
        *level = LOG_LEVEL_INFO;
    } else if (strcmp(text, "debug") == 0) {
        *level = LOG_LEVEL_DEBUG;
    } else {
        return -1;
    }
    return 0;
}

void log_message(log_level_t level, const char *format, ...)
{
    static const char *names[] = {"ERROR", "INFO", "DEBUG"};
    va_list arguments;

    if (level > current_level || level < LOG_LEVEL_ERROR
        || level > LOG_LEVEL_DEBUG || format == NULL) {
        return;
    }

    fprintf(stderr, "[%s pid=%ld] ", names[level], (long)getpid());
    va_start(arguments, format);
    vfprintf(stderr, format, arguments);
    va_end(arguments);
    fputc('\n', stderr);
    fflush(stderr);
}
