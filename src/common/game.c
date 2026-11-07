#include "game.h"

bool game_code_is_valid(const char *code, size_t length)
{
    size_t index;
    size_t previous;

    if (code == NULL || length != 4U) {
        return false;
    }

    for (index = 0U; index < length; ++index) {
        if (code[index] < '0' || code[index] > '9') {
            return false;
        }

        for (previous = 0U; previous < index; ++previous) {
            if (code[index] == code[previous]) {
                return false;
            }
        }
    }

    return true;
}

bool game_score_calculate(
    const char *answer,
    size_t answer_length,
    const char *guess,
    size_t guess_length,
    game_score_t *score
)
{
    size_t index;
    size_t other;

    if (score == NULL
        || !game_code_is_valid(answer, answer_length)
        || !game_code_is_valid(guess, guess_length)) {
        return false;
    }

    score->exact_matches = 0U;
    score->misplaced_matches = 0U;

    for (index = 0U; index < 4U; ++index) {
        if (answer[index] == guess[index]) {
            ++score->exact_matches;
            continue;
        }

        for (other = 0U; other < 4U; ++other) {
            if (answer[index] == guess[other]) {
                ++score->misplaced_matches;
                break;
            }
        }
    }

    return true;
}
