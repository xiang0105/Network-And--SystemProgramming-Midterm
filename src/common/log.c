#include "log.h"

#include <errno.h>
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
    char message[1024];
    int prefix_length;
    int body_length;
    size_t message_length;
    ssize_t written;
    va_list arguments;

    if (level > current_level || level < LOG_LEVEL_ERROR
        || level > LOG_LEVEL_DEBUG || format == NULL) {
        return;
    }

    prefix_length = snprintf(
        message,
        sizeof(message),
        "[%s pid=%ld] ",
        names[level],
        (long)getpid()
    );
    if (prefix_length < 0 || (size_t)prefix_length >= sizeof(message) - 1U) {
        return;
    }

    va_start(arguments, format);
    body_length = vsnprintf(
        message + prefix_length,
        sizeof(message) - (size_t)prefix_length - 1U,
        format,
        arguments
    );
    va_end(arguments);
    if (body_length < 0) {
        return;
    }

    if ((size_t)body_length
        >= sizeof(message) - (size_t)prefix_length - 1U) {
        message_length = sizeof(message) - 2U;
    } else {
        message_length = (size_t)prefix_length + (size_t)body_length;
    }
    message[message_length++] = '\n';

    do {
        written = write(STDERR_FILENO, message, message_length);
    } while (written < 0 && errno == EINTR);

    if (written < 0 || (size_t)written != message_length) {
        return;
    }
}
