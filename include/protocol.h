#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#define PROTOCOL_HEADER_SIZE 12U
#define PROTOCOL_MAX_PAYLOAD_SIZE 4096U
#define PROTOCOL_VERSION 1U

typedef enum {
    PROTOCOL_MSG_START_GAME = 0x01,
    PROTOCOL_MSG_GUESS = 0x02,
    PROTOCOL_MSG_GET_SYSTEM_INFO = 0x03,
    PROTOCOL_MSG_GET_STATS = 0x04,
    PROTOCOL_MSG_QUIT = 0x05,
    PROTOCOL_MSG_WELCOME = 0x81,
    PROTOCOL_MSG_GAME_STARTED = 0x82,
    PROTOCOL_MSG_GUESS_RESULT = 0x83,
    PROTOCOL_MSG_GAME_WON = 0x84,
    PROTOCOL_MSG_GAME_LOST = 0x85,
    PROTOCOL_MSG_SYSTEM_INFO = 0x86,
    PROTOCOL_MSG_STATS = 0x87,
    PROTOCOL_MSG_ERROR = 0xFF
} protocol_message_type_t;

typedef enum {
    PROTOCOL_STATUS_OK = 0,
    PROTOCOL_STATUS_INVALID_ARGUMENT,
    PROTOCOL_STATUS_INVALID_MAGIC,
    PROTOCOL_STATUS_UNSUPPORTED_VERSION,
    PROTOCOL_STATUS_INVALID_RESERVED,
    PROTOCOL_STATUS_PAYLOAD_TOO_LARGE,
    PROTOCOL_STATUS_UNKNOWN_TYPE
} protocol_status_t;

typedef struct {
    uint8_t version;
    protocol_message_type_t type;
    uint32_t payload_length;
} protocol_header_t;

protocol_status_t protocol_encode_header(
    uint8_t *buffer,
    size_t buffer_length,
    protocol_message_type_t type,
    uint32_t payload_length
);

protocol_status_t protocol_decode_header(
    const uint8_t *buffer,
    size_t buffer_length,
    protocol_header_t *header
);

#endif
