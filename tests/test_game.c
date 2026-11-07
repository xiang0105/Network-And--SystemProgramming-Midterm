#include "game.h"

#include <stdio.h>

int main(void)
{
    game_score_t score = {0U, 0U};
    char generated[5] = {0};
    size_t generation;

    for (generation = 0U; generation < 100U; ++generation) {
        if (!game_generate_code(generated)
            || !game_code_is_valid(generated, 4U)
            || generated[4] != '\0') {
            fprintf(stderr, "expected generated codes to follow game rules\n");
            return 1;
        }
    }

    if (!game_code_is_valid("0123", 4U)) {
        fprintf(stderr, "expected a four-digit code with a leading zero to be valid\n");
        return 1;
    }

    if (game_code_is_valid("123", 3U)) {
        fprintf(stderr, "expected a three-digit code to be invalid\n");
        return 1;
    }

    if (game_code_is_valid(NULL, 4U)) {
        fprintf(stderr, "expected a null code buffer to be invalid\n");
        return 1;
    }

    if (game_code_is_valid("12a3", 4U)) {
        fprintf(stderr, "expected a code containing a non-digit to be invalid\n");
        return 1;
    }

    if (game_code_is_valid("1123", 4U)) {
        fprintf(stderr, "expected a code containing duplicate digits to be invalid\n");
        return 1;
    }

    if (!game_score_calculate("0123", 4U, "0123", 4U, &score)) {
        fprintf(stderr, "expected two valid codes to be scoreable\n");
        return 1;
    }

    if (score.exact_matches != 4U || score.misplaced_matches != 0U) {
        fprintf(stderr, "expected identical codes to score 4A0B\n");
        return 1;
    }

    if (!game_score_calculate("0123", 4U, "0132", 4U, &score)) {
        fprintf(stderr, "expected two valid codes to be scoreable\n");
        return 1;
    }

    if (score.exact_matches != 2U || score.misplaced_matches != 2U) {
        fprintf(stderr, "expected 0123 compared with 0132 to score 2A2B\n");
        return 1;
    }

    if (game_score_calculate("0123", 4U, "1123", 4U, &score)) {
        fprintf(stderr, "expected scoring to reject an invalid guess\n");
        return 1;
    }

    if (game_score_calculate("0123", 4U, "0132", 4U, NULL)) {
        fprintf(stderr, "expected scoring to reject a null output buffer\n");
        return 1;
    }

    return 0;
}
