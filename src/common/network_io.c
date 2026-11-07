#include "network_io.h"

#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <sys/socket.h>

static network_io_status_t network_remaining_timeout_ms(
    const struct timespec *deadline,
    int *timeout_ms
)
{
    struct timespec now;
    long long remaining_seconds;
    long remaining_nanoseconds;
    long long remaining_milliseconds;

    while (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        if (errno != EINTR) {
            return NETWORK_IO_STATUS_SYSTEM_ERROR;
        }
    }

    remaining_seconds = (long long)deadline->tv_sec - (long long)now.tv_sec;
    remaining_nanoseconds = deadline->tv_nsec - now.tv_nsec;

    if (remaining_nanoseconds < 0L) {
        --remaining_seconds;
        remaining_nanoseconds += 1000000000L;
    }

    if (remaining_seconds < 0LL
        || (remaining_seconds == 0LL && remaining_nanoseconds == 0L)) {
        *timeout_ms = 0;
        return NETWORK_IO_STATUS_OK;
    }

    if (remaining_seconds >= (long long)INT_MAX / 1000LL) {
        *timeout_ms = INT_MAX;
        return NETWORK_IO_STATUS_OK;
    }

    remaining_milliseconds = remaining_seconds * 1000LL
        + (remaining_nanoseconds + 999999L) / 1000000L;

    if (remaining_milliseconds > INT_MAX) {
        *timeout_ms = INT_MAX;
    } else {
        *timeout_ms = (int)remaining_milliseconds;
    }

    return NETWORK_IO_STATUS_OK;
}

static network_io_status_t network_wait_for_event(
    int socket_fd,
    short events,
    const struct timespec *deadline
)
{
    struct pollfd descriptor = {socket_fd, events, 0};

    for (;;) {
        int poll_result;
        int timeout_ms;
        network_io_status_t status;

        status = network_remaining_timeout_ms(deadline, &timeout_ms);
        if (status != NETWORK_IO_STATUS_OK) {
            return status;
        }

        if (timeout_ms == 0) {
            return NETWORK_IO_STATUS_TIMEOUT;
        }

        poll_result = poll(&descriptor, 1, timeout_ms);
        if (poll_result > 0) {
            return NETWORK_IO_STATUS_OK;
        }

        if (poll_result == 0) {
            return NETWORK_IO_STATUS_TIMEOUT;
        }

        if (errno != EINTR) {
            return NETWORK_IO_STATUS_SYSTEM_ERROR;
        }
    }
}

network_io_status_t network_read_exact(
    int socket_fd,
    void *buffer,
    size_t length,
    const struct timespec *deadline
)
{
    size_t total_received = 0U;

    if (socket_fd < 0 || deadline == NULL
        || deadline->tv_nsec < 0L || deadline->tv_nsec >= 1000000000L
        || (buffer == NULL && length > 0U)) {
        return NETWORK_IO_STATUS_INVALID_ARGUMENT;
    }

    if (length == 0U) {
        return NETWORK_IO_STATUS_OK;
    }

    while (total_received < length) {
        network_io_status_t status;
        ssize_t received;

        status = network_wait_for_event(socket_fd, POLLIN, deadline);
        if (status != NETWORK_IO_STATUS_OK) {
            return status;
        }

        received = recv(
            socket_fd,
            (char *)buffer + total_received,
            length - total_received,
            MSG_DONTWAIT
        );

        if (received > 0) {
            total_received += (size_t)received;
            continue;
        }

        if (received == 0) {
            return NETWORK_IO_STATUS_EOF;
        }

        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
            continue;
        }

        return NETWORK_IO_STATUS_SYSTEM_ERROR;
    }

    return NETWORK_IO_STATUS_OK;
}

network_io_status_t network_write_all(
    int socket_fd,
    const void *buffer,
    size_t length,
    const struct timespec *deadline
)
{
    size_t total_sent = 0U;

    if (socket_fd < 0 || deadline == NULL
        || deadline->tv_nsec < 0L || deadline->tv_nsec >= 1000000000L
        || (buffer == NULL && length > 0U)) {
        return NETWORK_IO_STATUS_INVALID_ARGUMENT;
    }

    if (length == 0U) {
        return NETWORK_IO_STATUS_OK;
    }

    while (total_sent < length) {
        network_io_status_t status;
        ssize_t sent;

        status = network_wait_for_event(socket_fd, POLLOUT, deadline);
        if (status != NETWORK_IO_STATUS_OK) {
            return status;
        }

        sent = send(
            socket_fd,
            (const char *)buffer + total_sent,
            length - total_sent,
            MSG_DONTWAIT | MSG_NOSIGNAL
        );

        if (sent > 0) {
            total_sent += (size_t)sent;
            continue;
        }

        if (sent == 0) {
            return NETWORK_IO_STATUS_SYSTEM_ERROR;
        }

        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
            continue;
        }

        return NETWORK_IO_STATUS_SYSTEM_ERROR;
    }

    return NETWORK_IO_STATUS_OK;
}
