#include "protocol.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

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
    struct timespec deadline;
    uint8_t wire_bytes[PROTOCOL_HEADER_SIZE + 4U] = {0U};
    const uint8_t expected[PROTOCOL_HEADER_SIZE + 4U] = {
        'A', 'B', 'G', 'M',
        PROTOCOL_VERSION,
        PROTOCOL_MSG_GUESS,
        0U, 0U,
        0U, 0U, 0U, 4U,
        '0', '1', '2', '3'
    };

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) {
        perror("socketpair send frame");
        return 1;
    }

    if (deadline_after_milliseconds(&deadline, 1000L) != 0) {
        perror("clock_gettime send frame");
        close(sockets[0]);
        close(sockets[1]);
        return 1;
    }

    if (protocol_send_frame(
            sockets[0],
            PROTOCOL_MSG_GUESS,
            "0123",
            4U,
            &deadline
        ) != PROTOCOL_STATUS_OK) {
        fprintf(stderr, "expected a valid frame to send successfully\n");
        close(sockets[0]);
        close(sockets[1]);
        return 1;
    }

    if (recv(
            sockets[1],
            wire_bytes,
            sizeof(wire_bytes),
            MSG_WAITALL
        ) != (ssize_t)sizeof(wire_bytes)) {
        perror("recv frame bytes");
        close(sockets[0]);
        close(sockets[1]);
        return 1;
    }

    close(sockets[0]);
    close(sockets[1]);

    if (memcmp(wire_bytes, expected, sizeof(expected)) != 0) {
        fprintf(stderr, "sent frame did not match the wire format\n");
        return 1;
    }

    {
        int roundtrip_sockets[2];
        protocol_frame_t frame = {0};

        if (socketpair(AF_UNIX, SOCK_STREAM, 0, roundtrip_sockets) != 0) {
            perror("socketpair receive frame");
            return 1;
        }

        if (deadline_after_milliseconds(&deadline, 1000L) != 0) {
            perror("clock_gettime receive frame");
            close(roundtrip_sockets[0]);
            close(roundtrip_sockets[1]);
            return 1;
        }

        if (protocol_send_frame(
                roundtrip_sockets[0],
                PROTOCOL_MSG_GUESS,
                "0123",
                4U,
                &deadline
            ) != PROTOCOL_STATUS_OK) {
            fprintf(stderr, "expected roundtrip frame send to succeed\n");
            close(roundtrip_sockets[0]);
            close(roundtrip_sockets[1]);
            return 1;
        }

        if (protocol_receive_frame(
                roundtrip_sockets[1],
                &frame,
                &deadline
            ) != PROTOCOL_STATUS_OK) {
            fprintf(stderr, "expected a valid frame to receive successfully\n");
            close(roundtrip_sockets[0]);
            close(roundtrip_sockets[1]);
            return 1;
        }

        close(roundtrip_sockets[0]);
        close(roundtrip_sockets[1]);

        if (frame.header.version != PROTOCOL_VERSION
            || frame.header.type != PROTOCOL_MSG_GUESS
            || frame.header.payload_length != 4U
            || memcmp(frame.payload, "0123", 4U) != 0
            || frame.payload[4] != '\0') {
            fprintf(stderr, "received frame did not match the sent frame\n");
            return 1;
        }
    }

    {
        int invalid_magic_sockets[2];
        protocol_frame_t frame = {0};
        const uint8_t invalid_header[PROTOCOL_HEADER_SIZE] = {
            'X', 'B', 'G', 'M',
            PROTOCOL_VERSION,
            PROTOCOL_MSG_START_GAME,
            0U, 0U,
            0U, 0U, 0U, 0U
        };

        if (socketpair(AF_UNIX, SOCK_STREAM, 0, invalid_magic_sockets) != 0) {
            perror("socketpair invalid magic");
            return 1;
        }

        if (send(
                invalid_magic_sockets[0],
                invalid_header,
                sizeof(invalid_header),
                0
            ) != (ssize_t)sizeof(invalid_header)) {
            perror("send invalid magic");
            close(invalid_magic_sockets[0]);
            close(invalid_magic_sockets[1]);
            return 1;
        }

        if (deadline_after_milliseconds(&deadline, 1000L) != 0) {
            perror("clock_gettime invalid magic");
            close(invalid_magic_sockets[0]);
            close(invalid_magic_sockets[1]);
            return 1;
        }

        if (protocol_receive_frame(
                invalid_magic_sockets[1],
                &frame,
                &deadline
            ) != PROTOCOL_STATUS_INVALID_MAGIC) {
            fprintf(stderr, "expected an invalid frame magic to be rejected\n");
            close(invalid_magic_sockets[0]);
            close(invalid_magic_sockets[1]);
            return 1;
        }

        close(invalid_magic_sockets[0]);
        close(invalid_magic_sockets[1]);
    }

    {
        int oversized_sockets[2];
        protocol_frame_t frame = {0};
        const uint8_t oversized_header[PROTOCOL_HEADER_SIZE] = {
            'A', 'B', 'G', 'M',
            PROTOCOL_VERSION,
            PROTOCOL_MSG_SYSTEM_INFO,
            0U, 0U,
            0U, 0U, 0x10U, 0x01U
        };

        if (socketpair(AF_UNIX, SOCK_STREAM, 0, oversized_sockets) != 0) {
            perror("socketpair oversized frame");
            return 1;
        }

        if (send(
                oversized_sockets[0],
                oversized_header,
                sizeof(oversized_header),
                0
            ) != (ssize_t)sizeof(oversized_header)) {
            perror("send oversized frame");
            close(oversized_sockets[0]);
            close(oversized_sockets[1]);
            return 1;
        }

        if (deadline_after_milliseconds(&deadline, 1000L) != 0) {
            perror("clock_gettime oversized frame");
            close(oversized_sockets[0]);
            close(oversized_sockets[1]);
            return 1;
        }

        if (protocol_receive_frame(
                oversized_sockets[1],
                &frame,
                &deadline
            ) != PROTOCOL_STATUS_PAYLOAD_TOO_LARGE) {
            fprintf(stderr, "expected an oversized frame to be rejected before payload read\n");
            close(oversized_sockets[0]);
            close(oversized_sockets[1]);
            return 1;
        }

        close(oversized_sockets[0]);
        close(oversized_sockets[1]);
    }

    {
        int missing_payload_sockets[2];
        protocol_frame_t frame = {0};
        const uint8_t header_only[PROTOCOL_HEADER_SIZE] = {
            'A', 'B', 'G', 'M',
            PROTOCOL_VERSION,
            PROTOCOL_MSG_GUESS,
            0U, 0U,
            0U, 0U, 0U, 4U
        };

        if (socketpair(AF_UNIX, SOCK_STREAM, 0, missing_payload_sockets) != 0) {
            perror("socketpair missing payload");
            return 1;
        }

        if (send(
                missing_payload_sockets[0],
                header_only,
                sizeof(header_only),
                0
            ) != (ssize_t)sizeof(header_only)) {
            perror("send header without payload");
            close(missing_payload_sockets[0]);
            close(missing_payload_sockets[1]);
            return 1;
        }

        if (deadline_after_milliseconds(&deadline, 100L) != 0) {
            perror("clock_gettime missing payload");
            close(missing_payload_sockets[0]);
            close(missing_payload_sockets[1]);
            return 1;
        }

        if (protocol_receive_frame(
                missing_payload_sockets[1],
                &frame,
                &deadline
            ) != PROTOCOL_STATUS_TIMEOUT) {
            fprintf(stderr, "expected a missing payload to time out\n");
            close(missing_payload_sockets[0]);
            close(missing_payload_sockets[1]);
            return 1;
        }

        close(missing_payload_sockets[0]);
        close(missing_payload_sockets[1]);
    }

    {
        int partial_header_sockets[2];
        protocol_frame_t frame = {0};
        const uint8_t partial_header[6] = {
            'A', 'B', 'G', 'M',
            PROTOCOL_VERSION,
            PROTOCOL_MSG_START_GAME
        };

        if (socketpair(AF_UNIX, SOCK_STREAM, 0, partial_header_sockets) != 0) {
            perror("socketpair partial header");
            return 1;
        }

        if (send(
                partial_header_sockets[0],
                partial_header,
                sizeof(partial_header),
                0
            ) != (ssize_t)sizeof(partial_header)) {
            perror("send partial header");
            close(partial_header_sockets[0]);
            close(partial_header_sockets[1]);
            return 1;
        }
        close(partial_header_sockets[0]);

        if (deadline_after_milliseconds(&deadline, 1000L) != 0) {
            perror("clock_gettime partial header");
            close(partial_header_sockets[1]);
            return 1;
        }

        if (protocol_receive_frame(
                partial_header_sockets[1],
                &frame,
                &deadline
            ) != PROTOCOL_STATUS_EOF) {
            fprintf(stderr, "expected EOF for a truncated frame header\n");
            close(partial_header_sockets[1]);
            return 1;
        }

        close(partial_header_sockets[1]);
    }

    {
        int empty_frame_sockets[2];
        protocol_frame_t frame = {0};

        if (socketpair(AF_UNIX, SOCK_STREAM, 0, empty_frame_sockets) != 0) {
            perror("socketpair empty frame");
            return 1;
        }

        if (deadline_after_milliseconds(&deadline, 1000L) != 0) {
            perror("clock_gettime empty frame");
            close(empty_frame_sockets[0]);
            close(empty_frame_sockets[1]);
            return 1;
        }

        if (protocol_send_frame(
                empty_frame_sockets[0],
                PROTOCOL_MSG_START_GAME,
                NULL,
                0U,
                &deadline
            ) != PROTOCOL_STATUS_OK
            || protocol_receive_frame(
                empty_frame_sockets[1],
                &frame,
                &deadline
            ) != PROTOCOL_STATUS_OK) {
            fprintf(stderr, "expected a zero-payload frame to roundtrip\n");
            close(empty_frame_sockets[0]);
            close(empty_frame_sockets[1]);
            return 1;
        }

        close(empty_frame_sockets[0]);
        close(empty_frame_sockets[1]);

        if (frame.header.type != PROTOCOL_MSG_START_GAME
            || frame.header.payload_length != 0U
            || frame.payload[0] != '\0') {
            fprintf(stderr, "zero-payload frame fields were incorrect\n");
            return 1;
        }
    }

    {
        int invalid_payload_sockets[2];
        char unexpected_byte;

        if (socketpair(AF_UNIX, SOCK_STREAM, 0, invalid_payload_sockets) != 0) {
            perror("socketpair invalid payload");
            return 1;
        }

        if (deadline_after_milliseconds(&deadline, 1000L) != 0) {
            perror("clock_gettime invalid payload");
            close(invalid_payload_sockets[0]);
            close(invalid_payload_sockets[1]);
            return 1;
        }

        if (protocol_send_frame(
                invalid_payload_sockets[0],
                PROTOCOL_MSG_GUESS,
                NULL,
                4U,
                &deadline
            ) != PROTOCOL_STATUS_INVALID_ARGUMENT) {
            fprintf(stderr, "expected a missing nonempty payload to be rejected\n");
            close(invalid_payload_sockets[0]);
            close(invalid_payload_sockets[1]);
            return 1;
        }

        if (recv(
                invalid_payload_sockets[1],
                &unexpected_byte,
                1U,
                MSG_DONTWAIT
            ) != -1
            || (errno != EAGAIN && errno != EWOULDBLOCK)) {
            fprintf(stderr, "invalid frame wrote bytes before validation\n");
            close(invalid_payload_sockets[0]);
            close(invalid_payload_sockets[1]);
            return 1;
        }

        close(invalid_payload_sockets[0]);
        close(invalid_payload_sockets[1]);
    }

    return 0;
}
