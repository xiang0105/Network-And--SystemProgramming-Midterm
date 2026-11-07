#include "game.h"
#include "protocol.h"

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT "9734"

static int deadline_after_seconds(struct timespec *deadline, unsigned int seconds)
{
    if (clock_gettime(CLOCK_MONOTONIC, deadline) != 0) {
        return -1;
    }
    deadline->tv_sec += (time_t)seconds;
    return 0;
}

static int connect_to_server(const char *host, const char *port)
{
    struct addrinfo hints = {0};
    struct addrinfo *addresses = NULL;
    struct addrinfo *address;
    int socket_fd = -1;

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, port, &hints, &addresses) != 0) {
        return -1;
    }
    for (address = addresses; address != NULL; address = address->ai_next) {
        socket_fd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        if (socket_fd >= 0
            && connect(socket_fd, address->ai_addr, address->ai_addrlen) == 0) {
            break;
        }
        if (socket_fd >= 0) {
            close(socket_fd);
        }
        socket_fd = -1;
    }
    freeaddrinfo(addresses);
    return socket_fd;
}

static int exchange(
    int socket_fd,
    protocol_message_type_t type,
    const void *payload,
    uint32_t length,
    protocol_frame_t *response
)
{
    struct timespec deadline;

    if (deadline_after_seconds(&deadline, 120U) != 0
        || protocol_send_frame(socket_fd, type, payload, length, &deadline)
            != PROTOCOL_STATUS_OK
        || protocol_receive_frame(socket_fd, response, &deadline)
            != PROTOCOL_STATUS_OK) {
        return -1;
    }
    return 0;
}

static int play_game(int socket_fd)
{
    protocol_frame_t response;
    char input[128];

    if (exchange(socket_fd, PROTOCOL_MSG_START_GAME, NULL, 0U, &response) != 0
        || response.header.type != PROTOCOL_MSG_GAME_STARTED) {
        return -1;
    }
    printf("%s", response.payload);

    for (;;) {
        size_t length;
        printf("Guess four distinct digits: ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            return -1;
        }
        input[strcspn(input, "\r\n")] = '\0';
        length = strlen(input);
        if (!game_code_is_valid(input, length)) {
            puts("Invalid guess: enter exactly four distinct digits.");
            continue;
        }
        if (exchange(socket_fd, PROTOCOL_MSG_GUESS, input, 4U, &response) != 0) {
            return -1;
        }
        printf("%s", response.payload);
        if (response.header.type == PROTOCOL_MSG_GAME_WON
            || response.header.type == PROTOCOL_MSG_GAME_LOST) {
            return 0;
        }
        if (response.header.type != PROTOCOL_MSG_GUESS_RESULT
            && response.header.type != PROTOCOL_MSG_ERROR) {
            return -1;
        }
    }
}

int main(int argc, char **argv)
{
    const char *host = argc > 1 ? argv[1] : DEFAULT_HOST;
    const char *port = argc > 2 ? argv[2] : DEFAULT_PORT;
    int socket_fd;
    protocol_frame_t response;
    struct timespec deadline;
    char input[32];

    if (argc > 3) {
        fprintf(stderr, "usage: %s [HOST] [PORT]\n", argv[0]);
        return 2;
    }
    socket_fd = connect_to_server(host, port);
    if (socket_fd < 0) {
        fprintf(stderr, "failed to connect to %s:%s\n", host, port);
        return 1;
    }
    if (deadline_after_seconds(&deadline, 120U) != 0
        || protocol_receive_frame(socket_fd, &response, &deadline)
            != PROTOCOL_STATUS_OK
        || response.header.type != PROTOCOL_MSG_WELCOME) {
        fprintf(stderr, "invalid server welcome\n");
        close(socket_fd);
        return 1;
    }
    printf("%s\n", response.payload);

    for (;;) {
        puts("\n1. Start game\n2. Server system information\n3. Connection stats\n4. Quit");
        printf("Choice: ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        if (input[0] == '1') {
            if (play_game(socket_fd) != 0) {
                fprintf(stderr, "game communication failed\n");
                close(socket_fd);
                return 1;
            }
        } else if (input[0] == '2' || input[0] == '3') {
            protocol_message_type_t type = input[0] == '2'
                ? PROTOCOL_MSG_GET_SYSTEM_INFO : PROTOCOL_MSG_GET_STATS;
            if (exchange(socket_fd, type, NULL, 0U, &response) != 0) {
                fprintf(stderr, "request failed\n");
                close(socket_fd);
                return 1;
            }
            printf("%s", response.payload);
        } else if (input[0] == '4') {
            if (deadline_after_seconds(&deadline, 120U) == 0) {
                (void)protocol_send_frame(socket_fd, PROTOCOL_MSG_QUIT,
                    NULL, 0U, &deadline);
            }
            break;
        } else {
            puts("Invalid choice.");
        }
    }

    close(socket_fd);
    return 0;
}
