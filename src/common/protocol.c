#include "protocol.h"

#include "network_io.h"

#include <string.h>

static protocol_status_t protocol_status_from_network_io(
    network_io_status_t status
)
{
    switch (status) {
    case NETWORK_IO_STATUS_OK:
        return PROTOCOL_STATUS_OK;
    case NETWORK_IO_STATUS_EOF:
        return PROTOCOL_STATUS_EOF;
    case NETWORK_IO_STATUS_TIMEOUT:
        return PROTOCOL_STATUS_TIMEOUT;
    case NETWORK_IO_STATUS_INVALID_ARGUMENT:
        return PROTOCOL_STATUS_INVALID_ARGUMENT;
    case NETWORK_IO_STATUS_SYSTEM_ERROR:
    default:
        return PROTOCOL_STATUS_IO_ERROR;
    }
}

static int protocol_message_type_is_known(protocol_message_type_t type)
{
    switch (type) {
    case PROTOCOL_MSG_START_GAME:
    case PROTOCOL_MSG_GUESS:
    case PROTOCOL_MSG_GET_SYSTEM_INFO:
    case PROTOCOL_MSG_GET_STATS:
    case PROTOCOL_MSG_QUIT:
    case PROTOCOL_MSG_WELCOME:
    case PROTOCOL_MSG_GAME_STARTED:
    case PROTOCOL_MSG_GUESS_RESULT:
    case PROTOCOL_MSG_GAME_WON:
    case PROTOCOL_MSG_GAME_LOST:
    case PROTOCOL_MSG_SYSTEM_INFO:
    case PROTOCOL_MSG_STATS:
    case PROTOCOL_MSG_ERROR:
        return 1;
    default:
        return 0;
    }
}

protocol_status_t protocol_encode_header(
    uint8_t *buffer,
    size_t buffer_length,
    protocol_message_type_t type,
    uint32_t payload_length
)
{
    if (buffer == NULL || buffer_length < PROTOCOL_HEADER_SIZE) {
        return PROTOCOL_STATUS_INVALID_ARGUMENT;
    }

    if (payload_length > PROTOCOL_MAX_PAYLOAD_SIZE) {
        return PROTOCOL_STATUS_PAYLOAD_TOO_LARGE;
    }

    if (!protocol_message_type_is_known(type)) {
        return PROTOCOL_STATUS_UNKNOWN_TYPE;
    }

    buffer[0] = 'A';
    buffer[1] = 'B';
    buffer[2] = 'G';
    buffer[3] = 'M';
    buffer[4] = PROTOCOL_VERSION;
    buffer[5] = (uint8_t)type;
    buffer[6] = 0U;
    buffer[7] = 0U;
    buffer[8] = (uint8_t)(payload_length >> 24U);
    buffer[9] = (uint8_t)(payload_length >> 16U);
    buffer[10] = (uint8_t)(payload_length >> 8U);
    buffer[11] = (uint8_t)payload_length;

    return PROTOCOL_STATUS_OK;
}

protocol_status_t protocol_decode_header(
    const uint8_t *buffer,
    size_t buffer_length,
    protocol_header_t *header
)
{
    uint32_t payload_length;

    if (buffer == NULL || header == NULL
        || buffer_length < PROTOCOL_HEADER_SIZE) {
        return PROTOCOL_STATUS_INVALID_ARGUMENT;
    }

    if (buffer[0] != 'A' || buffer[1] != 'B'
        || buffer[2] != 'G' || buffer[3] != 'M') {
        return PROTOCOL_STATUS_INVALID_MAGIC;
    }

    if (buffer[4] != PROTOCOL_VERSION) {
        return PROTOCOL_STATUS_UNSUPPORTED_VERSION;
    }

    if (buffer[6] != 0U || buffer[7] != 0U) {
        return PROTOCOL_STATUS_INVALID_RESERVED;
    }

    if (!protocol_message_type_is_known(
            (protocol_message_type_t)buffer[5]
        )) {
        return PROTOCOL_STATUS_UNKNOWN_TYPE;
    }

    payload_length = ((uint32_t)buffer[8] << 24U)
        | ((uint32_t)buffer[9] << 16U)
        | ((uint32_t)buffer[10] << 8U)
        | (uint32_t)buffer[11];

    if (payload_length > PROTOCOL_MAX_PAYLOAD_SIZE) {
        return PROTOCOL_STATUS_PAYLOAD_TOO_LARGE;
    }

    header->version = buffer[4];
    header->type = (protocol_message_type_t)buffer[5];
    header->payload_length = payload_length;

    return PROTOCOL_STATUS_OK;
}

protocol_status_t protocol_send_frame(
    int socket_fd,
    protocol_message_type_t type,
    const void *payload,
    uint32_t payload_length,
    const struct timespec *deadline
)
{
    uint8_t header_bytes[PROTOCOL_HEADER_SIZE];
    protocol_status_t protocol_status;
    network_io_status_t io_status;

    if (payload == NULL && payload_length > 0U) {
        return PROTOCOL_STATUS_INVALID_ARGUMENT;
    }

    protocol_status = protocol_encode_header(
        header_bytes,
        sizeof(header_bytes),
        type,
        payload_length
    );
    if (protocol_status != PROTOCOL_STATUS_OK) {
        return protocol_status;
    }

    io_status = network_write_all(
        socket_fd,
        header_bytes,
        sizeof(header_bytes),
        deadline
    );
    if (io_status != NETWORK_IO_STATUS_OK) {
        return protocol_status_from_network_io(io_status);
    }

    if (payload_length == 0U) {
        return PROTOCOL_STATUS_OK;
    }

    io_status = network_write_all(
        socket_fd,
        payload,
        payload_length,
        deadline
    );
    return protocol_status_from_network_io(io_status);
}

protocol_status_t protocol_receive_frame(
    int socket_fd,
    protocol_frame_t *frame,
    const struct timespec *deadline
)
{
    uint8_t header_bytes[PROTOCOL_HEADER_SIZE];
    protocol_status_t protocol_status;
    network_io_status_t io_status;

    if (frame == NULL) {
        return PROTOCOL_STATUS_INVALID_ARGUMENT;
    }

    memset(frame, 0, sizeof(*frame));

    io_status = network_read_exact(
        socket_fd,
        header_bytes,
        sizeof(header_bytes),
        deadline
    );
    if (io_status != NETWORK_IO_STATUS_OK) {
        return protocol_status_from_network_io(io_status);
    }

    protocol_status = protocol_decode_header(
        header_bytes,
        sizeof(header_bytes),
        &frame->header
    );
    if (protocol_status != PROTOCOL_STATUS_OK) {
        return protocol_status;
    }

    if (frame->header.payload_length > 0U) {
        io_status = network_read_exact(
            socket_fd,
            frame->payload,
            frame->header.payload_length,
            deadline
        );
        if (io_status != NETWORK_IO_STATUS_OK) {
            return protocol_status_from_network_io(io_status);
        }
    }

    frame->payload[frame->header.payload_length] = '\0';
    return PROTOCOL_STATUS_OK;
}
