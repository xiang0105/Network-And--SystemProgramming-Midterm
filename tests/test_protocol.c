#include "protocol.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    uint8_t encoded[PROTOCOL_HEADER_SIZE] = {0U};
    protocol_header_t decoded = {0U, PROTOCOL_MSG_START_GAME, 0U};
    const uint8_t expected[PROTOCOL_HEADER_SIZE] = {
        'A', 'B', 'G', 'M',
        PROTOCOL_VERSION,
        PROTOCOL_MSG_START_GAME,
        0U, 0U,
        0U, 0U, 0U, 0U
    };

    if (protocol_encode_header(
            encoded,
            sizeof(encoded),
            PROTOCOL_MSG_START_GAME,
            0U
        ) != PROTOCOL_STATUS_OK) {
        fprintf(stderr, "expected a valid header to encode successfully\n");
        return 1;
    }

    if (memcmp(encoded, expected, sizeof(expected)) != 0) {
        fprintf(stderr, "encoded header did not match the wire format\n");
        return 1;
    }

    if (protocol_encode_header(
            encoded,
            sizeof(encoded),
            PROTOCOL_MSG_SYSTEM_INFO,
            258U
        ) != PROTOCOL_STATUS_OK) {
        fprintf(stderr, "expected a header with a payload to encode successfully\n");
        return 1;
    }

    if (encoded[8] != 0U || encoded[9] != 0U
        || encoded[10] != 1U || encoded[11] != 2U) {
        fprintf(stderr, "expected payload length to use network byte order\n");
        return 1;
    }

    if (protocol_encode_header(
            encoded,
            sizeof(encoded),
            PROTOCOL_MSG_SYSTEM_INFO,
            PROTOCOL_MAX_PAYLOAD_SIZE + 1U
        ) != PROTOCOL_STATUS_PAYLOAD_TOO_LARGE) {
        fprintf(stderr, "expected an oversized payload to be rejected\n");
        return 1;
    }

    if (protocol_encode_header(
            NULL,
            PROTOCOL_HEADER_SIZE,
            PROTOCOL_MSG_START_GAME,
            0U
        ) != PROTOCOL_STATUS_INVALID_ARGUMENT) {
        fprintf(stderr, "expected a null output buffer to be rejected\n");
        return 1;
    }

    if (protocol_encode_header(
            encoded,
            PROTOCOL_HEADER_SIZE - 1U,
            PROTOCOL_MSG_START_GAME,
            0U
        ) != PROTOCOL_STATUS_INVALID_ARGUMENT) {
        fprintf(stderr, "expected a short output buffer to be rejected\n");
        return 1;
    }

    if (protocol_encode_header(
            encoded,
            sizeof(encoded),
            (protocol_message_type_t)0x40,
            0U
        ) != PROTOCOL_STATUS_UNKNOWN_TYPE) {
        fprintf(stderr, "expected an unknown message type to be rejected\n");
        return 1;
    }

    if (protocol_encode_header(
            encoded,
            sizeof(encoded),
            PROTOCOL_MSG_GUESS,
            4U
        ) != PROTOCOL_STATUS_OK) {
        fprintf(stderr, "expected the decoder fixture to encode successfully\n");
        return 1;
    }

    if (protocol_decode_header(
            encoded,
            sizeof(encoded),
            &decoded
        ) != PROTOCOL_STATUS_OK) {
        fprintf(stderr, "expected a valid header to decode successfully\n");
        return 1;
    }

    if (decoded.version != PROTOCOL_VERSION
        || decoded.type != PROTOCOL_MSG_GUESS
        || decoded.payload_length != 4U) {
        fprintf(stderr, "decoded header fields did not match the wire bytes\n");
        return 1;
    }

    encoded[0] = 'X';
    if (protocol_decode_header(
            encoded,
            sizeof(encoded),
            &decoded
        ) != PROTOCOL_STATUS_INVALID_MAGIC) {
        fprintf(stderr, "expected an invalid magic value to be rejected\n");
        return 1;
    }
    encoded[0] = 'A';

    encoded[4] = PROTOCOL_VERSION + 1U;
    if (protocol_decode_header(
            encoded,
            sizeof(encoded),
            &decoded
        ) != PROTOCOL_STATUS_UNSUPPORTED_VERSION) {
        fprintf(stderr, "expected an unsupported version to be rejected\n");
        return 1;
    }
    encoded[4] = PROTOCOL_VERSION;

    encoded[7] = 1U;
    if (protocol_decode_header(
            encoded,
            sizeof(encoded),
            &decoded
        ) != PROTOCOL_STATUS_INVALID_RESERVED) {
        fprintf(stderr, "expected nonzero reserved bytes to be rejected\n");
        return 1;
    }
    encoded[7] = 0U;

    encoded[5] = 0x40U;
    if (protocol_decode_header(
            encoded,
            sizeof(encoded),
            &decoded
        ) != PROTOCOL_STATUS_UNKNOWN_TYPE) {
        fprintf(stderr, "expected an unknown decoded type to be rejected\n");
        return 1;
    }
    encoded[5] = PROTOCOL_MSG_GUESS;

    encoded[8] = 0U;
    encoded[9] = 0U;
    encoded[10] = 0x10U;
    encoded[11] = 0x01U;
    if (protocol_decode_header(
            encoded,
            sizeof(encoded),
            &decoded
        ) != PROTOCOL_STATUS_PAYLOAD_TOO_LARGE) {
        fprintf(stderr, "expected an oversized decoded payload to be rejected\n");
        return 1;
    }

    if (protocol_decode_header(
            NULL,
            PROTOCOL_HEADER_SIZE,
            &decoded
        ) != PROTOCOL_STATUS_INVALID_ARGUMENT) {
        fprintf(stderr, "expected a null input buffer to be rejected\n");
        return 1;
    }

    if (protocol_encode_header(
            encoded,
            sizeof(encoded),
            PROTOCOL_MSG_GUESS,
            4U
        ) != PROTOCOL_STATUS_OK) {
        fprintf(stderr, "expected the argument fixture to encode successfully\n");
        return 1;
    }

    if (protocol_decode_header(
            encoded,
            sizeof(encoded),
            NULL
        ) != PROTOCOL_STATUS_INVALID_ARGUMENT) {
        fprintf(stderr, "expected a null decoded header to be rejected\n");
        return 1;
    }

    if (protocol_decode_header(
            encoded,
            PROTOCOL_HEADER_SIZE - 1U,
            &decoded
        ) != PROTOCOL_STATUS_INVALID_ARGUMENT) {
        fprintf(stderr, "expected a short input buffer to be rejected\n");
        return 1;
    }

    return 0;
}
