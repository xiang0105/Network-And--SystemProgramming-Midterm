#ifndef SERVER_SESSION_H
#define SERVER_SESSION_H

typedef int (*server_answer_generator_t)(char output[5], void *context);

int server_session_run(
    int socket_fd,
    unsigned int idle_timeout_seconds,
    server_answer_generator_t answer_generator,
    void *answer_generator_context
);

#endif
