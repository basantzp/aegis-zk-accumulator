/**
 * @file accumulator.c
 * @brief State accumulator batch operations, historical root commitments, and ABI encoding.
 * @copyright (c) 2026 Aegis Project Contributors. Apache-2.0 / MIT.
 */

#include "aegis_precompile.h"
#include "aegis_smt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Selector for verifyProof(bytes32,bytes32,bytes32,bytes32[]) -> 0x532810b1 */
static const uint8_t VERIFY_PROOF_SELECTOR[4] = {0x53, 0x28, 0x10, 0xb1};

aegis_status_t aegis_encode_solidity_calldata(const uint8_t root[AEGIS_WORD_SIZE], const uint8_t key[AEGIS_WORD_SIZE],
                                              const uint8_t value[AEGIS_WORD_SIZE], const aegis_smt_proof_t *proof,
                                              uint8_t *out_buf, size_t buf_capacity, size_t *out_len) {
    if (!root || !key || !value || !proof || !out_buf || !out_len) {
        return AEGIS_ERR_NULL_PTR;
    }

    size_t required_len = 4 + 32 * 5 + 32 * proof->depth;
    if (buf_capacity < required_len) {
        return AEGIS_ERR_BUFFER_OVERFLOW;
    }

    size_t offset = 0;

    /* 1. Function Selector (4 bytes) */
    memcpy(out_buf + offset, VERIFY_PROOF_SELECTOR, 4);
    offset += 4;

    /* 2. bytes32 root (32 bytes) */
    memcpy(out_buf + offset, root, AEGIS_WORD_SIZE);
    offset += 32;

    /* 3. bytes32 key (32 bytes) */
    memcpy(out_buf + offset, key, AEGIS_WORD_SIZE);
    offset += 32;

    /* 4. bytes32 value (32 bytes) */
    memcpy(out_buf + offset, value, AEGIS_WORD_SIZE);
    offset += 32;

    /* 5. offset to siblings array in calldata: 4 * 32 = 128 bytes (0x80) */
    memset(out_buf + offset, 0, 31);
    out_buf[offset + 31] = 0x80;
    offset += 32;

    /* 6. siblings array length (32 bytes) */
    memset(out_buf + offset, 0, 28);
    out_buf[offset + 28] = (uint8_t)((proof->depth >> 24) & 0xFF);
    out_buf[offset + 29] = (uint8_t)((proof->depth >> 16) & 0xFF);
    out_buf[offset + 30] = (uint8_t)((proof->depth >> 8) & 0xFF);
    out_buf[offset + 31] = (uint8_t)(proof->depth & 0xFF);
    offset += 32;

    /* 7. siblings elements */
    for (uint32_t i = 0; i < proof->depth; i++) {
        memcpy(out_buf + offset, proof->siblings[i].data, AEGIS_WORD_SIZE);
        offset += 32;
    }

    *out_len = offset;
    return AEGIS_SUCCESS;
}
