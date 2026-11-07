#ifndef NETWORK_IO_H
#define NETWORK_IO_H

#include <stddef.h>
#include <time.h>

typedef enum {
    NETWORK_IO_STATUS_OK = 0,
    NETWORK_IO_STATUS_EOF,
    NETWORK_IO_STATUS_TIMEOUT,
    NETWORK_IO_STATUS_INVALID_ARGUMENT,
    NETWORK_IO_STATUS_SYSTEM_ERROR
} network_io_status_t;

network_io_status_t network_read_exact(
    int socket_fd,
    void *buffer,
    size_t length,
    const struct timespec *deadline
);

network_io_status_t network_write_all(
    int socket_fd,
    const void *buffer,
    size_t length,
    const struct timespec *deadline
);

#endif
