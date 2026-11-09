#include "server_session.h"

#include "game.h"
#include "log.h"
#include "protocol.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

static int session_deadline_after_seconds(
    struct timespec *deadline,
    unsigned int seconds
)
{
    if (clock_gettime(CLOCK_MONOTONIC, deadline) != 0) {
        return -1;
    }

    deadline->tv_sec += (time_t)seconds;
    return 0;
}

static protocol_status_t session_send_text(
    int socket_fd,
    protocol_message_type_t type,
    const char *text,
    const struct timespec *deadline
)
{
    return protocol_send_frame(
        socket_fd,
        type,
        text,
        (uint32_t)strlen(text),
        deadline
    );
}

static int session_format_system_info(char *buffer, size_t buffer_size)
{
    struct sysinfo system_info;
    struct utsname system_name;
    long logical_cpus;
    unsigned long long memory_total;
    unsigned long long memory_available;
    int written;

    if (uname(&system_name) != 0 || sysinfo(&system_info) != 0) {
        return -1;
    }

    logical_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (logical_cpus < 1L) {
        return -1;
    }

    memory_total = (unsigned long long)system_info.totalram
        * (unsigned long long)system_info.mem_unit;
    memory_available = (unsigned long long)system_info.freeram
        * (unsigned long long)system_info.mem_unit;

    written = snprintf(
        buffer,
        buffer_size,
        "os=%s\n"
        "kernel=%s\n"
        "cpu_arch=%s\n"
        "logical_cpus=%ld\n"
        "uptime_seconds=%ld\n"
        "memory_total_bytes=%llu\n"
        "memory_available_bytes=%llu\n",
        system_name.sysname,
        system_name.release,
        system_name.machine,
        logical_cpus,
        system_info.uptime,
        memory_total,
        memory_available
    );

    if (written < 0 || (size_t)written >= buffer_size) {
        return -1;
    }

    return 0;
}

int server_session_run(
    int socket_fd,
    unsigned int idle_timeout_seconds,
    const char *peer_label,
    server_answer_generator_t answer_generator,
    void *answer_generator_context
)
{
    struct timespec deadline;
    unsigned int completed_games = 0U;
    unsigned int wins = 0U;
    unsigned int losses = 0U;
    unsigned int best_attempts = 0U;
    bool game_active = false;
    char answer[5] = {0};
    unsigned int attempts = 0U;
    unsigned int consecutive_errors = 0U;
    const char *peer = peer_label != NULL ? peer_label : "[unknown]:0";

    if (socket_fd < 0 || idle_timeout_seconds == 0U
        || answer_generator == NULL) {
        return -1;
    }

    LOG_INFO("session_started peer=%s", peer);

    if (session_deadline_after_seconds(
            &deadline,
            idle_timeout_seconds
        ) != 0
        || session_send_text(
            socket_fd,
            PROTOCOL_MSG_WELCOME,
            "Bulls and Cows server ready",
            &deadline
        ) != PROTOCOL_STATUS_OK) {
        return -1;
    }

    for (;;) {
        protocol_frame_t request;
        protocol_status_t status;

        if (session_deadline_after_seconds(
                &deadline,
                idle_timeout_seconds
            ) != 0) {
            return -1;
        }

        status = protocol_receive_frame(socket_fd, &request, &deadline);
        if (status == PROTOCOL_STATUS_EOF) {
            LOG_INFO("session_ended peer=%s reason=peer_closed", peer);
            return 0;
        }
        if (status == PROTOCOL_STATUS_TIMEOUT) {
            if (session_deadline_after_seconds(
                    &deadline,
                    idle_timeout_seconds
                ) == 0) {
                (void)session_send_text(
                    socket_fd,
                    PROTOCOL_MSG_ERROR,
                    "connection timeout",
                    &deadline
                );
            }
            LOG_INFO("session_ended peer=%s reason=timeout", peer);
            return 0;
        }
        if (status != PROTOCOL_STATUS_OK) {
            LOG_ERROR(
                "session_ended peer=%s reason=protocol_error status=%d",
                peer,
                (int)status
            );
            return 0;
        }

        if (request.header.type == PROTOCOL_MSG_QUIT
            && request.header.payload_length == 0U) {
            LOG_INFO("peer=%s action=QUIT", peer);
            return 0;
        }

        if (request.header.type == PROTOCOL_MSG_START_GAME
            && request.header.payload_length == 0U
            && !game_active) {
            if (answer_generator(answer, answer_generator_context) != 0
                || !game_code_is_valid(answer, 4U)) {
                return -1;
            }

            game_active = true;
            attempts = 0U;
            consecutive_errors = 0U;
            if (session_deadline_after_seconds(
                    &deadline,
                    idle_timeout_seconds
                ) != 0
                || session_send_text(
                    socket_fd,
                    PROTOCOL_MSG_GAME_STARTED,
                    "attempts_remaining=15\n",
                    &deadline
                ) != PROTOCOL_STATUS_OK) {
                return -1;
            }
            LOG_INFO("peer=%s action=START_GAME", peer);
            continue;
        }

        if (request.header.type == PROTOCOL_MSG_GUESS
            && request.header.payload_length == 4U
            && game_active) {
            game_score_t score;
            char result[256];
            int written;

            if (!game_code_is_valid((const char *)request.payload, 4U)) {
                ++consecutive_errors;
                LOG_INFO(
                    "peer=%s action=GUESS result=invalid consecutive_errors=%u",
                    peer,
                    consecutive_errors
                );
                if (session_deadline_after_seconds(
                        &deadline,
                        idle_timeout_seconds
                    ) != 0
                    || session_send_text(
                        socket_fd,
                        PROTOCOL_MSG_ERROR,
                        "invalid guess",
                        &deadline
                    ) != PROTOCOL_STATUS_OK) {
                    return -1;
                }
                if (consecutive_errors >= 5U) {
                    LOG_INFO(
                        "session_ended peer=%s reason=too_many_errors",
                        peer
                    );
                    return 0;
                }
                continue;
            }

            consecutive_errors = 0U;

            if (!game_score_calculate(
                    answer,
                    4U,
                    (const char *)request.payload,
                    4U,
                    &score
                )) {
                return -1;
            }

            ++attempts;
            if (score.exact_matches == 4U) {
                game_active = false;
                ++completed_games;
                ++wins;
                if (best_attempts == 0U || attempts < best_attempts) {
                    best_attempts = attempts;
                }

                written = snprintf(
                    result,
                    sizeof(result),
                    "a=4\nb=0\nattempts=%u\n",
                    attempts
                );
                if (written < 0 || (size_t)written >= sizeof(result)
                    || session_deadline_after_seconds(
                        &deadline,
                        idle_timeout_seconds
                    ) != 0
                    || session_send_text(
                        socket_fd,
                        PROTOCOL_MSG_GAME_WON,
                        result,
                        &deadline
                    ) != PROTOCOL_STATUS_OK) {
                    return -1;
                }
                LOG_INFO(
                    "peer=%s action=GUESS guess=%.4s a=4 b=0 attempt=%u outcome=win",
                    peer,
                    (const char *)request.payload,
                    attempts
                );
                continue;
            }

            if (attempts == 15U) {
                game_active = false;
                ++completed_games;
                ++losses;

                written = snprintf(
                    result,
                    sizeof(result),
                    "a=%u\nb=%u\nattempts=15\nanswer=%s\n",
                    score.exact_matches,
                    score.misplaced_matches,
                    answer
                );
                if (written < 0 || (size_t)written >= sizeof(result)
                    || session_deadline_after_seconds(
                        &deadline,
                        idle_timeout_seconds
                    ) != 0
                    || session_send_text(
                        socket_fd,
                        PROTOCOL_MSG_GAME_LOST,
                        result,
                        &deadline
                    ) != PROTOCOL_STATUS_OK) {
                    return -1;
                }
                LOG_INFO(
                    "peer=%s action=GUESS guess=%.4s a=%u b=%u attempt=15 outcome=loss",
                    peer,
                    (const char *)request.payload,
                    score.exact_matches,
                    score.misplaced_matches
                );
                continue;
            }

            written = snprintf(
                result,
                sizeof(result),
                "a=%u\nb=%u\nattempts=%u\nattempts_remaining=%u\n",
                score.exact_matches,
                score.misplaced_matches,
                attempts,
                15U - attempts
            );
            if (written < 0 || (size_t)written >= sizeof(result)
                || session_deadline_after_seconds(
                    &deadline,
                    idle_timeout_seconds
                ) != 0
                || session_send_text(
                    socket_fd,
                    PROTOCOL_MSG_GUESS_RESULT,
                    result,
                    &deadline
                ) != PROTOCOL_STATUS_OK) {
                return -1;
            }
            LOG_INFO(
                "peer=%s action=GUESS guess=%.4s a=%u b=%u attempt=%u outcome=continue",
                peer,
                (const char *)request.payload,
                score.exact_matches,
                score.misplaced_matches,
                attempts
            );
            continue;
        }

        if (request.header.type == PROTOCOL_MSG_GET_SYSTEM_INFO
            && request.header.payload_length == 0U) {
            char information[1024];

            consecutive_errors = 0U;

            if (session_format_system_info(
                    information,
                    sizeof(information)
                ) != 0
                || session_deadline_after_seconds(
                    &deadline,
                    idle_timeout_seconds
                ) != 0
                || session_send_text(
                    socket_fd,
                    PROTOCOL_MSG_SYSTEM_INFO,
                    information,
                    &deadline
                ) != PROTOCOL_STATUS_OK) {
                return -1;
            }
            LOG_INFO("peer=%s action=GET_SYSTEM_INFO", peer);
            continue;
        }

        if (request.header.type == PROTOCOL_MSG_GET_STATS
            && request.header.payload_length == 0U) {
            char statistics[256];
            int written = snprintf(
                statistics,
                sizeof(statistics),
                "completed=%u\n"
                "wins=%u\n"
                "losses=%u\n"
                "best_attempts=%u\n",
                completed_games,
                wins,
                losses,
                best_attempts
            );

            consecutive_errors = 0U;

            if (written < 0 || (size_t)written >= sizeof(statistics)
                || session_deadline_after_seconds(
                    &deadline,
                    idle_timeout_seconds
                ) != 0
                || session_send_text(
                    socket_fd,
                    PROTOCOL_MSG_STATS,
                    statistics,
                    &deadline
                ) != PROTOCOL_STATUS_OK) {
                return -1;
            }
            LOG_INFO(
                "peer=%s action=GET_STATS completed=%u wins=%u losses=%u best_attempts=%u",
                peer,
                completed_games,
                wins,
                losses,
                best_attempts
            );
            continue;
        }

        ++consecutive_errors;
        LOG_INFO(
            "peer=%s action=UNSUPPORTED type=0x%02x consecutive_errors=%u",
            peer,
            (unsigned int)request.header.type,
            consecutive_errors
        );
        if (session_deadline_after_seconds(
                &deadline,
                idle_timeout_seconds
            ) != 0
            || session_send_text(
                socket_fd,
                PROTOCOL_MSG_ERROR,
                "unsupported request",
                &deadline
            ) != PROTOCOL_STATUS_OK) {
            return -1;
        }
        if (consecutive_errors >= 5U) {
            LOG_INFO(
                "session_ended peer=%s reason=too_many_errors",
                peer
            );
            return 0;
        }
    }
}
