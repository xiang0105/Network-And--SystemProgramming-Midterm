#include "protocol.h"
#include "server_session.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static int fixed_answer(char output[5], void *context)
{
    (void)context;
    memcpy(output, "0123", 5U);
    return 0;
}

static int deadline_after_one_second(struct timespec *deadline)
{
    if (clock_gettime(CLOCK_MONOTONIC, deadline) != 0) {
        return -1;
    }
    ++deadline->tv_sec;
    return 0;
}

int main(void)
{
    int sockets[2];
    int child_status;
    pid_t child;
    protocol_frame_t frame = {0};
    struct timespec deadline;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) {
        perror("socketpair session");
        return 1;
    }

    child = fork();
    if (child < 0) {
        perror("fork session");
        close(sockets[0]);
        close(sockets[1]);
        return 1;
    }

    if (child == 0) {
        int result;

        close(sockets[0]);
        result = server_session_run(
            sockets[1], 1U, "[test-client]:1", fixed_answer, NULL
        );
        close(sockets[1]);
        _exit(result == 0 ? 0 : 2);
    }

    close(sockets[1]);
    if (deadline_after_one_second(&deadline) != 0
        || protocol_receive_frame(sockets[0], &frame, &deadline)
            != PROTOCOL_STATUS_OK
        || frame.header.type != PROTOCOL_MSG_WELCOME) {
        fprintf(stderr, "expected server session to send WELCOME\n");
        close(sockets[0]);
        waitpid(child, NULL, 0);
        return 1;
    }

    if (deadline_after_one_second(&deadline) != 0
        || protocol_send_frame(
            sockets[0],
            PROTOCOL_MSG_GET_SYSTEM_INFO,
            NULL,
            0U,
            &deadline
        ) != PROTOCOL_STATUS_OK
        || protocol_receive_frame(sockets[0], &frame, &deadline)
            != PROTOCOL_STATUS_OK
        || frame.header.type != PROTOCOL_MSG_SYSTEM_INFO
        || strstr((const char *)frame.payload, "kernel=") == NULL
        || strstr((const char *)frame.payload, "cpu_arch=") == NULL
        || strstr((const char *)frame.payload, "uptime_seconds=") == NULL
        || strstr((const char *)frame.payload, "memory_total_bytes=") == NULL) {
        fprintf(stderr, "expected safe server system information\n");
        close(sockets[0]);
        waitpid(child, NULL, 0);
        return 1;
    }

    if (deadline_after_one_second(&deadline) != 0
        || protocol_send_frame(
            sockets[0],
            PROTOCOL_MSG_GET_STATS,
            NULL,
            0U,
            &deadline
        ) != PROTOCOL_STATUS_OK
        || protocol_receive_frame(sockets[0], &frame, &deadline)
            != PROTOCOL_STATUS_OK
        || frame.header.type != PROTOCOL_MSG_STATS
        || strstr((const char *)frame.payload, "completed=0") == NULL
        || strstr((const char *)frame.payload, "wins=0") == NULL
        || strstr((const char *)frame.payload, "losses=0") == NULL) {
        fprintf(stderr, "expected initial connection stats to be zero\n");
        close(sockets[0]);
        waitpid(child, NULL, 0);
        return 1;
    }

    if (deadline_after_one_second(&deadline) != 0
        || protocol_send_frame(
            sockets[0],
            PROTOCOL_MSG_START_GAME,
            NULL,
            0U,
            &deadline
        ) != PROTOCOL_STATUS_OK
        || protocol_receive_frame(sockets[0], &frame, &deadline)
            != PROTOCOL_STATUS_OK
        || frame.header.type != PROTOCOL_MSG_GAME_STARTED) {
        fprintf(stderr, "expected a game to start\n");
        close(sockets[0]);
        waitpid(child, NULL, 0);
        return 1;
    }

    if (deadline_after_one_second(&deadline) != 0
        || protocol_send_frame(
            sockets[0],
            PROTOCOL_MSG_GUESS,
            "1123",
            4U,
            &deadline
        ) != PROTOCOL_STATUS_OK
        || protocol_receive_frame(sockets[0], &frame, &deadline)
            != PROTOCOL_STATUS_OK
        || frame.header.type != PROTOCOL_MSG_ERROR) {
        fprintf(stderr, "expected a duplicate-digit guess to be rejected\n");
        close(sockets[0]);
        waitpid(child, NULL, 0);
        return 1;
    }

    {
        static const char wrong_guesses[15][5] = {
            "1023", "0132", "0213", "0231", "0312",
            "0321", "1032", "1203", "1230", "1302",
            "1320", "2013", "2031", "2103", "2130"
        };
        size_t guess_index;

        for (guess_index = 0U; guess_index < 15U; ++guess_index) {
            protocol_message_type_t expected_type = guess_index == 14U
                ? PROTOCOL_MSG_GAME_LOST
                : PROTOCOL_MSG_GUESS_RESULT;

            if (deadline_after_one_second(&deadline) != 0
                || protocol_send_frame(
                    sockets[0],
                    PROTOCOL_MSG_GUESS,
                    wrong_guesses[guess_index],
                    4U,
                    &deadline
                ) != PROTOCOL_STATUS_OK
                || protocol_receive_frame(sockets[0], &frame, &deadline)
                    != PROTOCOL_STATUS_OK
                || frame.header.type != expected_type) {
                fprintf(stderr, "unexpected response for valid guess %zu\n", guess_index + 1U);
                close(sockets[0]);
                waitpid(child, NULL, 0);
                return 1;
            }
        }

        if (strstr((const char *)frame.payload, "answer=0123") == NULL) {
            fprintf(stderr, "expected a loss to reveal the answer\n");
            close(sockets[0]);
            waitpid(child, NULL, 0);
            return 1;
        }
    }

    if (deadline_after_one_second(&deadline) != 0
        || protocol_send_frame(
            sockets[0],
            PROTOCOL_MSG_GET_STATS,
            NULL,
            0U,
            &deadline
        ) != PROTOCOL_STATUS_OK
        || protocol_receive_frame(sockets[0], &frame, &deadline)
            != PROTOCOL_STATUS_OK
        || frame.header.type != PROTOCOL_MSG_STATS
        || strstr((const char *)frame.payload, "completed=1") == NULL
        || strstr((const char *)frame.payload, "wins=0") == NULL
        || strstr((const char *)frame.payload, "losses=1") == NULL
        || strstr((const char *)frame.payload, "best_attempts=0") == NULL) {
        fprintf(stderr, "expected a loss to update connection stats\n");
        close(sockets[0]);
        waitpid(child, NULL, 0);
        return 1;
    }

    if (deadline_after_one_second(&deadline) != 0
        || protocol_send_frame(
            sockets[0],
            PROTOCOL_MSG_START_GAME,
            NULL,
            0U,
            &deadline
        ) != PROTOCOL_STATUS_OK
        || protocol_receive_frame(sockets[0], &frame, &deadline)
            != PROTOCOL_STATUS_OK
        || frame.header.type != PROTOCOL_MSG_GAME_STARTED) {
        fprintf(stderr, "expected START_GAME to begin a game\n");
        close(sockets[0]);
        waitpid(child, NULL, 0);
        return 1;
    }

    if (deadline_after_one_second(&deadline) != 0
        || protocol_send_frame(
            sockets[0],
            PROTOCOL_MSG_GUESS,
            "0123",
            4U,
            &deadline
        ) != PROTOCOL_STATUS_OK
        || protocol_receive_frame(sockets[0], &frame, &deadline)
            != PROTOCOL_STATUS_OK
        || frame.header.type != PROTOCOL_MSG_GAME_WON
        || strstr((const char *)frame.payload, "attempts=1") == NULL) {
        fprintf(stderr, "expected the fixed answer to win in one guess\n");
        close(sockets[0]);
        waitpid(child, NULL, 0);
        return 1;
    }

    if (deadline_after_one_second(&deadline) != 0
        || protocol_send_frame(
            sockets[0],
            PROTOCOL_MSG_GET_STATS,
            NULL,
            0U,
            &deadline
        ) != PROTOCOL_STATUS_OK
        || protocol_receive_frame(sockets[0], &frame, &deadline)
            != PROTOCOL_STATUS_OK
        || frame.header.type != PROTOCOL_MSG_STATS
        || strstr((const char *)frame.payload, "completed=2") == NULL
        || strstr((const char *)frame.payload, "wins=1") == NULL
        || strstr((const char *)frame.payload, "losses=1") == NULL
        || strstr((const char *)frame.payload, "best_attempts=1") == NULL) {
        fprintf(stderr, "expected a win to update connection stats\n");
        close(sockets[0]);
        waitpid(child, NULL, 0);
        return 1;
    }

    if (deadline_after_one_second(&deadline) != 0
        || protocol_send_frame(
            sockets[0],
            PROTOCOL_MSG_QUIT,
            NULL,
            0U,
            &deadline
        ) != PROTOCOL_STATUS_OK) {
        fprintf(stderr, "expected QUIT to send successfully\n");
        close(sockets[0]);
        waitpid(child, NULL, 0);
        return 1;
    }

    close(sockets[0]);
    if (waitpid(child, &child_status, 0) != child
        || !WIFEXITED(child_status)
        || WEXITSTATUS(child_status) != 0) {
        fprintf(stderr, "server session child failed\n");
        return 1;
    }

    {
        int abuse_sockets[2];
        int abuse_child_status;
        pid_t abuse_child;
        size_t invalid_index;

        if (socketpair(AF_UNIX, SOCK_STREAM, 0, abuse_sockets) != 0) {
            perror("socketpair abuse session");
            return 1;
        }

        abuse_child = fork();
        if (abuse_child < 0) {
            perror("fork abuse session");
            close(abuse_sockets[0]);
            close(abuse_sockets[1]);
            return 1;
        }

        if (abuse_child == 0) {
            int result;

            close(abuse_sockets[0]);
            result = server_session_run(
                abuse_sockets[1],
                1U,
                "[test-client]:2",
                fixed_answer,
                NULL
            );
            close(abuse_sockets[1]);
            _exit(result == 0 ? 0 : 2);
        }

        close(abuse_sockets[1]);
        if (deadline_after_one_second(&deadline) != 0
            || protocol_receive_frame(abuse_sockets[0], &frame, &deadline)
                != PROTOCOL_STATUS_OK
            || frame.header.type != PROTOCOL_MSG_WELCOME) {
            fprintf(stderr, "expected abuse session WELCOME\n");
            close(abuse_sockets[0]);
            waitpid(abuse_child, NULL, 0);
            return 1;
        }

        for (invalid_index = 0U; invalid_index < 5U; ++invalid_index) {
            if (deadline_after_one_second(&deadline) != 0
                || protocol_send_frame(
                    abuse_sockets[0],
                    PROTOCOL_MSG_GUESS,
                    "1023",
                    4U,
                    &deadline
                ) != PROTOCOL_STATUS_OK
                || protocol_receive_frame(abuse_sockets[0], &frame, &deadline)
                    != PROTOCOL_STATUS_OK
                || frame.header.type != PROTOCOL_MSG_ERROR) {
                fprintf(stderr, "expected ERROR for invalid request %zu\n", invalid_index + 1U);
                close(abuse_sockets[0]);
                waitpid(abuse_child, NULL, 0);
                return 1;
            }
        }

        if (deadline_after_one_second(&deadline) != 0
            || protocol_receive_frame(abuse_sockets[0], &frame, &deadline)
                != PROTOCOL_STATUS_EOF) {
            fprintf(stderr, "expected connection close after five invalid requests\n");
            close(abuse_sockets[0]);
            waitpid(abuse_child, NULL, 0);
            return 1;
        }

        close(abuse_sockets[0]);
        if (waitpid(abuse_child, &abuse_child_status, 0) != abuse_child
            || !WIFEXITED(abuse_child_status)
            || WEXITSTATUS(abuse_child_status) != 0) {
            fprintf(stderr, "abuse session child failed\n");
            return 1;
        }
    }

    {
        int timeout_sockets[2];
        int timeout_child_status;
        pid_t timeout_child;

        if (socketpair(AF_UNIX, SOCK_STREAM, 0, timeout_sockets) != 0) {
            perror("socketpair timeout session");
            return 1;
        }

        timeout_child = fork();
        if (timeout_child < 0) {
            perror("fork timeout session");
            close(timeout_sockets[0]);
            close(timeout_sockets[1]);
            return 1;
        }

        if (timeout_child == 0) {
            int result;

            close(timeout_sockets[0]);
            result = server_session_run(
                timeout_sockets[1],
                1U,
                "[test-client]:3",
                fixed_answer,
                NULL
            );
            close(timeout_sockets[1]);
            _exit(result == 0 ? 0 : 2);
        }

        close(timeout_sockets[1]);
        if (deadline_after_one_second(&deadline) != 0
            || protocol_receive_frame(timeout_sockets[0], &frame, &deadline)
                != PROTOCOL_STATUS_OK
            || frame.header.type != PROTOCOL_MSG_WELCOME) {
            fprintf(stderr, "expected timeout session WELCOME\n");
            close(timeout_sockets[0]);
            waitpid(timeout_child, NULL, 0);
            return 1;
        }

        if (clock_gettime(CLOCK_MONOTONIC, &deadline) != 0) {
            perror("clock_gettime timeout response");
            close(timeout_sockets[0]);
            waitpid(timeout_child, NULL, 0);
            return 1;
        }
        deadline.tv_sec += 2;

        if (protocol_receive_frame(timeout_sockets[0], &frame, &deadline)
                != PROTOCOL_STATUS_OK
            || frame.header.type != PROTOCOL_MSG_ERROR
            || strstr((const char *)frame.payload, "timeout") == NULL
            || protocol_receive_frame(timeout_sockets[0], &frame, &deadline)
                != PROTOCOL_STATUS_EOF) {
            fprintf(stderr, "expected timeout ERROR followed by close\n");
            close(timeout_sockets[0]);
            waitpid(timeout_child, NULL, 0);
            return 1;
        }

        close(timeout_sockets[0]);
        if (waitpid(timeout_child, &timeout_child_status, 0) != timeout_child
            || !WIFEXITED(timeout_child_status)
            || WEXITSTATUS(timeout_child_status) != 0) {
            fprintf(stderr, "timeout session child failed\n");
            return 1;
        }
    }

    return 0;
}
