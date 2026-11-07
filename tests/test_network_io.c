#include "network_io.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t interruption_seen = 0;

static void mark_interruption(int signal_number)
{
    (void)signal_number;
    interruption_seen = 1;
}

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

int main(void)
{
    int sockets[2];
    char received[4] = {0};
    const char expected[4] = {'A', 'B', 'G', 'M'};
    struct timespec deadline;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) {
        perror("socketpair");
        return 1;
    }

    if (send(sockets[0], expected, sizeof(expected), 0) != (ssize_t)sizeof(expected)) {
        perror("send");
        close(sockets[0]);
        close(sockets[1]);
        return 1;
    }

    if (deadline_after_milliseconds(&deadline, 1000L) != 0) {
        perror("clock_gettime");
        close(sockets[0]);
        close(sockets[1]);
        return 1;
    }

    if (network_read_exact(
            sockets[1],
            received,
            sizeof(received),
            &deadline
        ) != NETWORK_IO_STATUS_OK) {
        fprintf(stderr, "expected available bytes to be read successfully\n");
        close(sockets[0]);
        close(sockets[1]);
        return 1;
    }

    close(sockets[0]);
    close(sockets[1]);

    if (memcmp(received, expected, sizeof(expected)) != 0) {
        fprintf(stderr, "read bytes did not match the sent bytes\n");
        return 1;
    }

    {
        int partial_sockets[2];
        int child_status;
        pid_t child;
        char partial_received[4] = {0};
        const struct timespec pause = {0, 100000000L};

        if (socketpair(AF_UNIX, SOCK_STREAM, 0, partial_sockets) != 0) {
            perror("socketpair partial read");
            return 1;
        }

        child = fork();
        if (child < 0) {
            perror("fork partial read");
            close(partial_sockets[0]);
            close(partial_sockets[1]);
            return 1;
        }

        if (child == 0) {
            close(partial_sockets[1]);
            if (send(partial_sockets[0], expected, 2U, 0) != 2) {
                _exit(2);
            }
            nanosleep(&pause, NULL);
            if (send(partial_sockets[0], expected + 2, 2U, 0) != 2) {
                _exit(3);
            }
            close(partial_sockets[0]);
            _exit(0);
        }

        close(partial_sockets[0]);
        if (deadline_after_milliseconds(&deadline, 1000L) != 0) {
            perror("clock_gettime partial read");
            close(partial_sockets[1]);
            waitpid(child, NULL, 0);
            return 1;
        }

        if (network_read_exact(
                partial_sockets[1],
                partial_received,
                sizeof(partial_received),
                &deadline
            ) != NETWORK_IO_STATUS_OK) {
            fprintf(stderr, "expected split bytes to be reassembled\n");
            close(partial_sockets[1]);
            waitpid(child, NULL, 0);
            return 1;
        }

        close(partial_sockets[1]);
        if (waitpid(child, &child_status, 0) != child
            || !WIFEXITED(child_status)
            || WEXITSTATUS(child_status) != 0) {
            fprintf(stderr, "partial-read child failed\n");
            return 1;
        }

        if (memcmp(partial_received, expected, sizeof(expected)) != 0) {
            fprintf(stderr, "reassembled bytes did not match\n");
            return 1;
        }
    }

    {
        int idle_sockets[2];
        char byte = '\0';
        struct timespec started;
        struct timespec finished;
        long long elapsed_milliseconds;

        if (socketpair(AF_UNIX, SOCK_STREAM, 0, idle_sockets) != 0) {
            perror("socketpair timeout");
            return 1;
        }

        if (clock_gettime(CLOCK_MONOTONIC, &started) != 0
            || deadline_after_milliseconds(&deadline, 100L) != 0) {
            perror("clock_gettime timeout");
            close(idle_sockets[0]);
            close(idle_sockets[1]);
            return 1;
        }

        if (network_read_exact(
                idle_sockets[1],
                &byte,
                1U,
                &deadline
            ) != NETWORK_IO_STATUS_TIMEOUT) {
            fprintf(stderr, "expected an idle read to time out\n");
            close(idle_sockets[0]);
            close(idle_sockets[1]);
            return 1;
        }

        if (clock_gettime(CLOCK_MONOTONIC, &finished) != 0) {
            perror("clock_gettime timeout finish");
            close(idle_sockets[0]);
            close(idle_sockets[1]);
            return 1;
        }

        close(idle_sockets[0]);
        close(idle_sockets[1]);

        elapsed_milliseconds = (long long)(finished.tv_sec - started.tv_sec) * 1000LL
            + (finished.tv_nsec - started.tv_nsec) / 1000000L;
        if (elapsed_milliseconds < 50LL) {
            fprintf(stderr, "idle timeout returned too early\n");
            return 1;
        }
    }

    {
        int eof_sockets[2];
        char eof_buffer[4] = {0};

        if (socketpair(AF_UNIX, SOCK_STREAM, 0, eof_sockets) != 0) {
            perror("socketpair eof");
            return 1;
        }

        if (send(eof_sockets[0], expected, 2U, 0) != 2) {
            perror("send eof");
            close(eof_sockets[0]);
            close(eof_sockets[1]);
            return 1;
        }
        close(eof_sockets[0]);

        if (deadline_after_milliseconds(&deadline, 1000L) != 0) {
            perror("clock_gettime eof");
            close(eof_sockets[1]);
            return 1;
        }

        if (network_read_exact(
                eof_sockets[1],
                eof_buffer,
                sizeof(eof_buffer),
                &deadline
            ) != NETWORK_IO_STATUS_EOF) {
            fprintf(stderr, "expected EOF before the requested length\n");
            close(eof_sockets[1]);
            return 1;
        }

        close(eof_sockets[1]);
    }

    {
        int slow_sockets[2];
        pid_t slow_child;
        char slow_buffer[4] = {0};
        const struct timespec slow_pause = {0, 80000000L};
        network_io_status_t slow_status;

        if (socketpair(AF_UNIX, SOCK_STREAM, 0, slow_sockets) != 0) {
            perror("socketpair slow read");
            return 1;
        }

        slow_child = fork();
        if (slow_child < 0) {
            perror("fork slow read");
            close(slow_sockets[0]);
            close(slow_sockets[1]);
            return 1;
        }

        if (slow_child == 0) {
            size_t index;

            close(slow_sockets[1]);
            for (index = 0U; index < sizeof(expected); ++index) {
                if (index > 0U) {
                    nanosleep(&slow_pause, NULL);
                }
                if (send(
                        slow_sockets[0],
                        expected + index,
                        1U,
                        MSG_NOSIGNAL
                    ) != 1) {
                    break;
                }
            }
            close(slow_sockets[0]);
            _exit(0);
        }

        close(slow_sockets[0]);
        if (deadline_after_milliseconds(&deadline, 150L) != 0) {
            perror("clock_gettime slow read");
            close(slow_sockets[1]);
            waitpid(slow_child, NULL, 0);
            return 1;
        }

        slow_status = network_read_exact(
            slow_sockets[1],
            slow_buffer,
            sizeof(slow_buffer),
            &deadline
        );
        close(slow_sockets[1]);
        waitpid(slow_child, NULL, 0);

        if (slow_status != NETWORK_IO_STATUS_TIMEOUT) {
            fprintf(stderr, "expected slow bytes not to extend the absolute deadline\n");
            return 1;
        }
    }

    {
        int interrupt_sockets[2];
        pid_t interrupt_child;
        char interrupt_byte = '\0';
        const struct timespec signal_pause = {0, 50000000L};
        const struct timespec child_hold = {0, 200000000L};
        struct sigaction action = {0};
        struct sigaction previous_action;
        network_io_status_t interrupt_status;

        action.sa_handler = mark_interruption;
        sigemptyset(&action.sa_mask);
        if (sigaction(SIGUSR1, &action, &previous_action) != 0) {
            perror("sigaction install");
            return 1;
        }

        if (socketpair(AF_UNIX, SOCK_STREAM, 0, interrupt_sockets) != 0) {
            perror("socketpair interrupted read");
            sigaction(SIGUSR1, &previous_action, NULL);
            return 1;
        }

        interrupt_child = fork();
        if (interrupt_child < 0) {
            perror("fork interrupted read");
            close(interrupt_sockets[0]);
            close(interrupt_sockets[1]);
            sigaction(SIGUSR1, &previous_action, NULL);
            return 1;
        }

        if (interrupt_child == 0) {
            close(interrupt_sockets[1]);
            nanosleep(&signal_pause, NULL);
            kill(getppid(), SIGUSR1);
            nanosleep(&child_hold, NULL);
            close(interrupt_sockets[0]);
            _exit(0);
        }

        close(interrupt_sockets[0]);
        if (deadline_after_milliseconds(&deadline, 150L) != 0) {
            perror("clock_gettime interrupted read");
            close(interrupt_sockets[1]);
            waitpid(interrupt_child, NULL, 0);
            sigaction(SIGUSR1, &previous_action, NULL);
            return 1;
        }

        interruption_seen = 0;
        interrupt_status = network_read_exact(
            interrupt_sockets[1],
            &interrupt_byte,
            1U,
            &deadline
        );
        close(interrupt_sockets[1]);
        waitpid(interrupt_child, NULL, 0);
        sigaction(SIGUSR1, &previous_action, NULL);

        if (!interruption_seen
            || interrupt_status != NETWORK_IO_STATUS_TIMEOUT) {
            fprintf(stderr, "expected EINTR to preserve the original deadline\n");
            return 1;
        }
    }

    {
        int write_sockets[2];
        char written_bytes[4] = {0};

        if (socketpair(AF_UNIX, SOCK_STREAM, 0, write_sockets) != 0) {
            perror("socketpair write");
            return 1;
        }

        if (deadline_after_milliseconds(&deadline, 1000L) != 0) {
            perror("clock_gettime write");
            close(write_sockets[0]);
            close(write_sockets[1]);
            return 1;
        }

        if (network_write_all(
                write_sockets[0],
                expected,
                sizeof(expected),
                &deadline
            ) != NETWORK_IO_STATUS_OK) {
            fprintf(stderr, "expected available bytes to be written successfully\n");
            close(write_sockets[0]);
            close(write_sockets[1]);
            return 1;
        }

        if (recv(
                write_sockets[1],
                written_bytes,
                sizeof(written_bytes),
                0
            ) != (ssize_t)sizeof(written_bytes)) {
            perror("recv written bytes");
            close(write_sockets[0]);
            close(write_sockets[1]);
            return 1;
        }

        close(write_sockets[0]);
        close(write_sockets[1]);

        if (memcmp(written_bytes, expected, sizeof(expected)) != 0) {
            fprintf(stderr, "written bytes did not match the source bytes\n");
            return 1;
        }
    }

    {
        int partial_write_sockets[2];
        int send_buffer_size = 1024;
        int partial_write_child_status;
        pid_t partial_write_child;
        char large_source[65536];
        size_t index;
        network_io_status_t write_status;

        for (index = 0U; index < sizeof(large_source); ++index) {
            large_source[index] = (char)(index % 251U);
        }

        if (socketpair(AF_UNIX, SOCK_STREAM, 0, partial_write_sockets) != 0) {
            perror("socketpair partial write");
            return 1;
        }

        if (setsockopt(
                partial_write_sockets[0],
                SOL_SOCKET,
                SO_SNDBUF,
                &send_buffer_size,
                sizeof(send_buffer_size)
            ) != 0) {
            perror("setsockopt partial write");
            close(partial_write_sockets[0]);
            close(partial_write_sockets[1]);
            return 1;
        }

        partial_write_child = fork();
        if (partial_write_child < 0) {
            perror("fork partial write");
            close(partial_write_sockets[0]);
            close(partial_write_sockets[1]);
            return 1;
        }

        if (partial_write_child == 0) {
            char chunk[1024];
            size_t total = 0U;
            const struct timespec reader_pause = {0, 100000000L};

            close(partial_write_sockets[0]);
            nanosleep(&reader_pause, NULL);

            while (total < sizeof(large_source)) {
                ssize_t received_count = recv(
                    partial_write_sockets[1],
                    chunk,
                    sizeof(chunk),
                    0
                );

                if (received_count <= 0) {
                    close(partial_write_sockets[1]);
                    _exit(2);
                }

                if (memcmp(
                        chunk,
                        large_source + total,
                        (size_t)received_count
                    ) != 0) {
                    close(partial_write_sockets[1]);
                    _exit(3);
                }

                total += (size_t)received_count;
            }

            close(partial_write_sockets[1]);
            _exit(0);
        }

        close(partial_write_sockets[1]);
        if (deadline_after_milliseconds(&deadline, 2000L) != 0) {
            perror("clock_gettime partial write");
            close(partial_write_sockets[0]);
            waitpid(partial_write_child, NULL, 0);
            return 1;
        }

        write_status = network_write_all(
            partial_write_sockets[0],
            large_source,
            sizeof(large_source),
            &deadline
        );
        close(partial_write_sockets[0]);

        if (waitpid(
                partial_write_child,
                &partial_write_child_status,
                0
            ) != partial_write_child
            || !WIFEXITED(partial_write_child_status)
            || WEXITSTATUS(partial_write_child_status) != 0) {
            fprintf(stderr, "partial-write child failed\n");
            return 1;
        }

        if (write_status != NETWORK_IO_STATUS_OK) {
            fprintf(stderr, "expected a partial write to complete before deadline\n");
            return 1;
        }
    }

    {
        int blocked_sockets[2];
        int send_buffer_size = 1024;
        char blocked_source[65536] = {0};

        if (socketpair(AF_UNIX, SOCK_STREAM, 0, blocked_sockets) != 0) {
            perror("socketpair blocked write");
            return 1;
        }

        if (setsockopt(
                blocked_sockets[0],
                SOL_SOCKET,
                SO_SNDBUF,
                &send_buffer_size,
                sizeof(send_buffer_size)
            ) != 0) {
            perror("setsockopt blocked write");
            close(blocked_sockets[0]);
            close(blocked_sockets[1]);
            return 1;
        }

        if (deadline_after_milliseconds(&deadline, 100L) != 0) {
            perror("clock_gettime blocked write");
            close(blocked_sockets[0]);
            close(blocked_sockets[1]);
            return 1;
        }

        if (network_write_all(
                blocked_sockets[0],
                blocked_source,
                sizeof(blocked_source),
                &deadline
            ) != NETWORK_IO_STATUS_TIMEOUT) {
            fprintf(stderr, "expected a blocked write to time out\n");
            close(blocked_sockets[0]);
            close(blocked_sockets[1]);
            return 1;
        }

        close(blocked_sockets[0]);
        close(blocked_sockets[1]);
    }

    {
        int closed_peer_sockets[2];

        if (socketpair(AF_UNIX, SOCK_STREAM, 0, closed_peer_sockets) != 0) {
            perror("socketpair closed peer");
            return 1;
        }
        close(closed_peer_sockets[1]);

        if (deadline_after_milliseconds(&deadline, 1000L) != 0) {
            perror("clock_gettime closed peer");
            close(closed_peer_sockets[0]);
            return 1;
        }

        if (network_write_all(
                closed_peer_sockets[0],
                expected,
                sizeof(expected),
                &deadline
            ) != NETWORK_IO_STATUS_SYSTEM_ERROR) {
            fprintf(stderr, "expected a closed peer to return a write error\n");
            close(closed_peer_sockets[0]);
            return 1;
        }

        close(closed_peer_sockets[0]);
    }

    return 0;
}
