/**
 * @file precompile_shim.c
 * @brief EVM precompile execution engine conforming to Ethereum Client architecture.
 * @copyright (c) 2026 Aegis Project Contributors. Apache-2.0 / MIT.
 */

#include "aegis_precompile.h"
#include <string.h>

static uint8_t get_key_bit(const uint8_t key[AEGIS_WORD_SIZE], uint32_t bit_idx) {
    if (bit_idx >= 256)
        return 0;
    uint32_t byte_idx = bit_idx / 8;
    uint32_t bit_pos = 7 - (bit_idx % 8);
    return (key[byte_idx] >> bit_pos) & 1;
}

int aegis_precompile_eval(const uint8_t *input, size_t input_len, uint8_t *output, size_t max_output_len,
                          size_t *actual_output_len, uint64_t *gas_cost) {
    if (!output || !actual_output_len || !gas_cost)
        return -1;
    if (max_output_len < AEGIS_WORD_SIZE)
        return -1;

    /* Base gas initialization */
    *gas_cost = AEGIS_PRECOMPILE_BASE_GAS;
    *actual_output_len = AEGIS_WORD_SIZE;
    memset(output, 0, AEGIS_WORD_SIZE);

    /* Minimal input requires root(32) + key(32) + value(32) + depth(32) = 128 bytes */
    if (input_len < 128 || !input) {
        return 0; /* Returns false (0x00) for malformed calldata */
    }

    const uint8_t *root = input;
    const uint8_t *key = input + 32;
    const uint8_t *value = input + 64;

    /* Validate that upper 28 bytes of uint256 depth word are all zeroes */
    for (size_t b = 96; b < 124; b++) {
        if (input[b] != 0) {
            return 0; /* Depth exceeds uint32 range -> reject as invalid */
        }
    }

    /* Extract 32-bit depth from 256-bit big-endian uint */
    uint32_t depth = ((uint32_t)input[124] << 24) | ((uint32_t)input[125] << 16) | ((uint32_t)input[126] << 8) |
                     ((uint32_t)input[127]);

    if (depth == 0 || depth > AEGIS_MAX_TREE_DEPTH) {
        return 0;
    }

    size_t expected_total_len = 128 + (size_t)depth * 32;
    if (input_len < expected_total_len) {
        return 0;
    }

    /* Compute gas cost */
    *gas_cost = AEGIS_PRECOMPILE_BASE_GAS + (uint64_t)depth * AEGIS_PRECOMPILE_GAS_PER_LEVEL;

    /* Recompute leaf hash */
    uint8_t current[AEGIS_WORD_SIZE];
    aegis_status_t s = aegis_smt_leaf_hash(key, value, current);
    if (s != AEGIS_SUCCESS)
        return 0;

    /* Traverse siblings from leaf level up to root */
    for (int i = (int)depth - 1; i >= 0; i--) {
        const uint8_t *sib = input + 128 + i * 32;
        uint8_t bit = get_key_bit(key, (uint32_t)i);
        uint8_t next[AEGIS_WORD_SIZE];

        if (bit == 0) {
            /* Current is left, sibling is right */
            aegis_hash_nodes(current, sib, next);
        } else {
            /* Current is right, sibling is left */
            aegis_hash_nodes(sib, current, next);
        }
        memcpy(current, next, AEGIS_WORD_SIZE);
    }

    /* Compare computed root with given root */
    if (memcmp(current, root, AEGIS_WORD_SIZE) == 0) {
        output[31] = 0x01; /* Valid */
    } else {
        output[31] = 0x00; /* Invalid */
    }

    return 0;
}
