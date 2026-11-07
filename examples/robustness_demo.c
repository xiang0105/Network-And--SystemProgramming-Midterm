#include "network_io.h"
#include "protocol.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static const unsigned char get_stats_frame[PROTOCOL_HEADER_SIZE] = {
    'A', 'B', 'G', 'M',
    PROTOCOL_VERSION, PROTOCOL_MSG_GET_STATS,
    0U, 0U,
    0U, 0U, 0U, 0U
};

static int deadline_after_milliseconds(
    struct timespec *deadline,
    long milliseconds
)
{
    if (clock_gettime(CLOCK_MONOTONIC, deadline) != 0) {
        return -1;
    }
    deadline->tv_sec += milliseconds / 1000L;
    deadline->tv_nsec += (milliseconds % 1000L) * 1000000L;
    if (deadline->tv_nsec >= 1000000000L) {
        ++deadline->tv_sec;
        deadline->tv_nsec -= 1000000000L;
    }
    return 0;
}

static int fragmented_sender(int socket_fd)
{
    const struct timespec pause = {0, 100000000L};

    if (send(socket_fd, get_stats_frame, 4U, MSG_NOSIGNAL) != 4) {
        return -1;
    }
    (void)nanosleep(&pause, NULL);
    if (send(
            socket_fd,
            get_stats_frame + 4U,
            PROTOCOL_HEADER_SIZE - 4U,
            MSG_NOSIGNAL
        ) != (ssize_t)(PROTOCOL_HEADER_SIZE - 4U)) {
        return -1;
    }
    return 0;
}

static int run_fragmented_input_comparison(void)
{
    int unsafe_sockets[2];
    pid_t unsafe_sender;
    int sender_status;
    unsigned char header[PROTOCOL_HEADER_SIZE] = {0};
    ssize_t received;
    int robust_sockets[2];
    pid_t robust_sender;
    protocol_frame_t frame;
    struct timespec deadline;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, unsafe_sockets) != 0) {
        return -1;
    }
    unsafe_sender = fork();
    if (unsafe_sender < 0) {
        close(unsafe_sockets[0]);
        close(unsafe_sockets[1]);
        return -1;
    }
    if (unsafe_sender == 0) {
        int result;

        close(unsafe_sockets[1]);
        result = fragmented_sender(unsafe_sockets[0]);
        close(unsafe_sockets[0]);
        _exit(result == 0 ? 0 : 2);
    }

    close(unsafe_sockets[0]);
    received = recv(unsafe_sockets[1], header, sizeof(header), 0);
    if (waitpid(unsafe_sender, &sender_status, 0) != unsafe_sender) {
        close(unsafe_sockets[1]);
        return -1;
    }
    close(unsafe_sockets[1]);
    if (received != 4 || !WIFEXITED(sender_status)
        || WEXITSTATUS(sender_status) != 0) {
        return -1;
    }

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, robust_sockets) != 0) {
        return -1;
    }
    robust_sender = fork();
    if (robust_sender < 0) {
        close(robust_sockets[0]);
        close(robust_sockets[1]);
        return -1;
    }
    if (robust_sender == 0) {
        int result;

        close(robust_sockets[1]);
        result = fragmented_sender(robust_sockets[0]);
        close(robust_sockets[0]);
        _exit(result == 0 ? 0 : 2);
    }

    close(robust_sockets[0]);
    if (deadline_after_milliseconds(&deadline, 1000L) != 0
        || protocol_receive_frame(robust_sockets[1], &frame, &deadline)
            != PROTOCOL_STATUS_OK
        || frame.header.type != PROTOCOL_MSG_GET_STATS
        || frame.header.payload_length != 0U) {
        close(robust_sockets[1]);
        waitpid(robust_sender, NULL, 0);
        return -1;
    }
    close(robust_sockets[1]);
    if (waitpid(robust_sender, &sender_status, 0) != robust_sender
        || !WIFEXITED(sender_status) || WEXITSTATUS(sender_status) != 0) {
        return -1;
    }

    puts("case=fragmented-input unsafe=FAIL(short-read) robust=PASS(reassembled)");
    return 0;
}

static int run_incomplete_input_comparison(void)
{
    int unsafe_sockets[2];
    pid_t blocked_reader;
    pid_t wait_result;
    int reader_status;
    const struct timespec observation = {0, 200000000L};
    int robust_sockets[2];
    protocol_frame_t frame;
    struct timespec deadline;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, unsafe_sockets) != 0) {
        return -1;
    }
    blocked_reader = fork();
    if (blocked_reader < 0) {
        close(unsafe_sockets[0]);
        close(unsafe_sockets[1]);
        return -1;
    }
    if (blocked_reader == 0) {
        unsigned char header[PROTOCOL_HEADER_SIZE];
        size_t total_received = 0U;

        close(unsafe_sockets[0]);
        while (total_received < sizeof(header)) {
            ssize_t received = recv(
                unsafe_sockets[1],
                header + total_received,
                sizeof(header) - total_received,
                0
            );

            if (received <= 0) {
                close(unsafe_sockets[1]);
                _exit(2);
            }
            total_received += (size_t)received;
        }
        close(unsafe_sockets[1]);
        _exit(0);
    }

    close(unsafe_sockets[1]);
    if (send(unsafe_sockets[0], "AB", 2U, MSG_NOSIGNAL) != 2) {
        close(unsafe_sockets[0]);
        kill(blocked_reader, SIGKILL);
        waitpid(blocked_reader, NULL, 0);
        return -1;
    }
    (void)nanosleep(&observation, NULL);
    wait_result = waitpid(blocked_reader, &reader_status, WNOHANG);
    if (wait_result != 0) {
        close(unsafe_sockets[0]);
        return -1;
    }
    if (kill(blocked_reader, SIGKILL) != 0
        || waitpid(blocked_reader, &reader_status, 0) != blocked_reader) {
        close(unsafe_sockets[0]);
        return -1;
    }
    close(unsafe_sockets[0]);

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, robust_sockets) != 0) {
        return -1;
    }
    if (send(robust_sockets[0], "AB", 2U, MSG_NOSIGNAL) != 2
        || deadline_after_milliseconds(&deadline, 150L) != 0
        || protocol_receive_frame(robust_sockets[1], &frame, &deadline)
            != PROTOCOL_STATUS_TIMEOUT) {
        close(robust_sockets[0]);
        close(robust_sockets[1]);
        return -1;
    }
    close(robust_sockets[0]);
    close(robust_sockets[1]);

    puts("case=incomplete-input unsafe=HANG(blocked) robust=PASS(timeout)");
    return 0;
}

static int run_closed_peer_comparison(void)
{
    pid_t unsafe_writer = fork();
    int writer_status;
    int robust_sockets[2];
    struct timespec deadline;

    if (unsafe_writer < 0) {
        return -1;
    }
    if (unsafe_writer == 0) {
        int sockets[2];

        if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) {
            _exit(2);
        }
        (void)signal(SIGPIPE, SIG_DFL);
        close(sockets[1]);
        (void)send(sockets[0], "ABGM", 4U, 0);
        close(sockets[0]);
        _exit(3);
    }

    if (waitpid(unsafe_writer, &writer_status, 0) != unsafe_writer
        || !WIFSIGNALED(writer_status)
        || WTERMSIG(writer_status) != SIGPIPE) {
        return -1;
    }

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, robust_sockets) != 0) {
        return -1;
    }
    close(robust_sockets[1]);
    if (deadline_after_milliseconds(&deadline, 1000L) != 0
        || network_write_all(
            robust_sockets[0],
            "ABGM",
            4U,
            &deadline
        ) != NETWORK_IO_STATUS_SYSTEM_ERROR) {
        close(robust_sockets[0]);
        return -1;
    }
    close(robust_sockets[0]);

    puts("case=closed-peer unsafe=CRASH(SIGPIPE) robust=PASS(error-returned)");
    return 0;
}

int main(void)
{
    if (run_fragmented_input_comparison() != 0
        || run_incomplete_input_comparison() != 0
        || run_closed_peer_comparison() != 0) {
        fprintf(stderr, "robustness comparison failed\n");
        return 1;
    }

    puts("robustness-comparison: PASS");
    return 0;
}
