/**
 * @file aegis_types.h
 * @brief Core data types, error constants, and primitives for Aegis ZK Accumulator.
 * @copyright (c) 2026 Aegis Project Contributors. Apache-2.0 / MIT.
 */

#ifndef AEGIS_TYPES_H
#define AEGIS_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum supported Merkle Tree depth (standard 256 for EVM 256-bit address space) */
#define AEGIS_MAX_TREE_DEPTH 256

/** Standard 32-byte word size (EVM word size) */
#define AEGIS_WORD_SIZE 32

/** Status and error codes */
typedef enum {
    AEGIS_SUCCESS = 0,
    AEGIS_ERR_NULL_PTR = -1,
    AEGIS_ERR_OUT_OF_BOUNDS = -2,
    AEGIS_ERR_VERIFY_FAILED = -3,
    AEGIS_ERR_OUT_OF_MEMORY = -4,
    AEGIS_ERR_BUFFER_OVERFLOW = -5,
    AEGIS_ERR_INVALID_DEPTH = -6,
    AEGIS_ERR_KEY_NOT_FOUND = -7,
    AEGIS_ERR_INVALID_FORMAT = -8
} aegis_status_t;

/** 32-byte array representing cryptographic hash or EVM uint256 / bytes32 */
typedef struct {
    uint8_t data[AEGIS_WORD_SIZE];
} aegis_bytes32_t;

/** SMT Single-Path Inclusion / Non-Inclusion Proof */
typedef struct {
    uint32_t depth;                                 /** Number of sibling nodes */
    aegis_bytes32_t siblings[AEGIS_MAX_TREE_DEPTH]; /** Sibling hashes */
    uint8_t directions[AEGIS_MAX_TREE_DEPTH];       /** 0 = target is left (sibling right), 1 = target is right */
} aegis_smt_proof_t;

/** SMT Batch Proof for multiple leaves */
typedef struct {
    uint32_t count;
    aegis_bytes32_t *keys;
    aegis_bytes32_t *values;
    aegis_smt_proof_t *proofs;
} aegis_batch_proof_t;

/** EVM Precompile Call Execution Result */
typedef struct {
    uint8_t valid;         /** 1 if proof is cryptographically sound, 0 otherwise */
    uint64_t gas_consumed; /** Total calculated gas consumption */
} aegis_precompile_result_t;

#ifdef __cplusplus
}
#endif

#endif /* AEGIS_TYPES_H */
