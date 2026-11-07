#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    unsigned int exact_matches;
    unsigned int misplaced_matches;
} game_score_t;

bool game_code_is_valid(const char *code, size_t length);
bool game_generate_code(char output[5]);
bool game_score_calculate(
    const char *answer,
    size_t answer_length,
    const char *guess,
    size_t guess_length,
    game_score_t *score
);

#endif
