#include "game.h"
#include "log.h"
#include "protocol.h"
#include "server_session.h"

#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT "9734"
#define DEFAULT_TIMEOUT 120U
#define MAX_ACTIVE_CLIENTS 32U

static volatile sig_atomic_t child_exit_pending = 0;

static void mark_child_exit(int signal_number)
{
    (void)signal_number;
    child_exit_pending = 1;
}

static int generate_answer(char output[5], void *context)
{
    (void)context;
    return game_generate_code(output) ? 0 : -1;
}

static void remove_active_pid(pid_t *pids, size_t *count, pid_t pid)
{
    size_t index;

    for (index = 0U; index < *count; ++index) {
        if (pids[index] == pid) {
            pids[index] = pids[*count - 1U];
            --(*count);
            return;
        }
    }
}

static void reap_children(pid_t *pids, size_t *count)
{
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        remove_active_pid(pids, count, pid);
    }
    child_exit_pending = 0;
}

static int create_listener(const char *host, const char *port)
{
    struct addrinfo hints = {0};
    struct addrinfo *addresses = NULL;
    struct addrinfo *address;
    int listener = -1;
    int enabled = 1;

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(host, port, &hints, &addresses) != 0) {
        return -1;
    }

    for (address = addresses; address != NULL; address = address->ai_next) {
        listener = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        if (listener < 0) {
            continue;
        }
        (void)setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));
        if (bind(listener, address->ai_addr, address->ai_addrlen) == 0
            && listen(listener, 32) == 0) {
            break;
        }
        close(listener);
        listener = -1;
    }

    freeaddrinfo(addresses);
    return listener;
}

static unsigned int parse_timeout(const char *text)
{
    char *end = NULL;
    unsigned long value;

    errno = 0;
    value = strtoul(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || value == 0UL || value > 3600UL) {
        return 0U;
    }
    return (unsigned int)value;
}

static int format_peer_label(
    const struct sockaddr *address,
    socklen_t address_length,
    char *label,
    size_t label_size
)
{
    char host[128];
    char service[32];
    int written;

    if (address == NULL || label == NULL || label_size == 0U) {
        return -1;
    }

    if (getnameinfo(
            address,
            address_length,
            host,
            sizeof(host),
            service,
            sizeof(service),
            NI_NUMERICHOST | NI_NUMERICSERV
        ) != 0) {
        return -1;
    }

    written = snprintf(label, label_size, "[%s]:%s", host, service);
    if (written < 0 || (size_t)written >= label_size) {
        return -1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    const char *host = DEFAULT_HOST;
    const char *port = DEFAULT_PORT;
    unsigned int timeout = DEFAULT_TIMEOUT;
    log_level_t log_level = LOG_LEVEL_INFO;
    pid_t active_pids[MAX_ACTIVE_CLIENTS];
    size_t active_count = 0U;
    struct sigaction action = {0};
    int listener;
    int index;

    for (index = 1; index < argc; ++index) {
        if (strcmp(argv[index], "--host") == 0 && index + 1 < argc) {
            host = argv[++index];
        } else if (strcmp(argv[index], "--port") == 0 && index + 1 < argc) {
            port = argv[++index];
        } else if (strcmp(argv[index], "--idle-timeout") == 0 && index + 1 < argc) {
            timeout = parse_timeout(argv[++index]);
            if (timeout == 0U) {
                fprintf(stderr, "invalid idle timeout\n");
                return 2;
            }
        } else if (strcmp(argv[index], "--log-level") == 0 && index + 1 < argc) {
            if (log_parse_level(argv[++index], &log_level) != 0) {
                fprintf(stderr, "invalid log level\n");
                return 2;
            }
        } else {
            fprintf(stderr, "usage: %s [--host ADDRESS] [--port PORT] [--idle-timeout SECONDS] [--log-level error|info|debug]\n", argv[0]);
            return 2;
        }
    }

    log_set_level(log_level);

    action.sa_handler = mark_child_exit;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGCHLD, &action, NULL) != 0) {
        perror("sigaction");
        return 1;
    }

    listener = create_listener(host, port);
    if (listener < 0) {
        fprintf(stderr, "failed to listen on %s:%s\n", host, port);
        return 1;
    }
    printf("server listening on %s:%s\n", host, port);
    fflush(stdout);
    LOG_INFO("listening on %s:%s with timeout=%u", host, port, timeout);

    for (;;) {
        struct sockaddr_storage peer_address;
        socklen_t peer_address_length = sizeof(peer_address);
        char peer_label[192] = "[unknown]:0";
        int client_fd;
        pid_t child;
        int admitted;

        if (child_exit_pending) {
            reap_children(active_pids, &active_count);
        }

        client_fd = accept(
            listener,
            (struct sockaddr *)&peer_address,
            &peer_address_length
        );
        if (client_fd < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("accept");
            continue;
        }

        if (format_peer_label(
                (const struct sockaddr *)&peer_address,
                peer_address_length,
                peer_label,
                sizeof(peer_label)
            ) != 0) {
            LOG_ERROR("could not format connected client address");
        }

        LOG_DEBUG(
            "accepted connection peer=%s active_sessions=%zu",
            peer_label,
            active_count
        );

        admitted = active_count < MAX_ACTIVE_CLIENTS;
        child = fork();
        if (child < 0) {
            LOG_ERROR("fork failed peer=%s: %s", peer_label, strerror(errno));
            close(client_fd);
            continue;
        }

        if (child == 0) {
            int result = 0;
            close(listener);
            if (admitted) {
                result = server_session_run(
                    client_fd,
                    timeout,
                    peer_label,
                    generate_answer,
                    NULL
                );
            } else {
                struct timespec deadline;
                if (clock_gettime(CLOCK_MONOTONIC, &deadline) == 0) {
                    ++deadline.tv_sec;
                    (void)protocol_send_frame(client_fd, PROTOCOL_MSG_ERROR,
                        "server busy", 11U, &deadline);
                }
            }
            LOG_INFO(
                "client_disconnected peer=%s result=%s",
                peer_label,
                result == 0 ? "normal" : "error"
            );
            close(client_fd);
            _exit(result == 0 ? 0 : 1);
        }

        close(client_fd);
        if (admitted) {
            active_pids[active_count++] = child;
            LOG_INFO(
                "client_connected peer=%s child=%ld active_sessions=%zu",
                peer_label,
                (long)child,
                active_count
            );
        } else {
            LOG_INFO(
                "client_rejected peer=%s child=%ld reason=server_busy",
                peer_label,
                (long)child
            );
        }
    }
}
