/**
 * @file aegis_precompile.h
 * @brief EVM precompile interface and calldata ABI serialization for Aegis SMT.
 * @copyright (c) 2026 Aegis Project Contributors. Apache-2.0 / MIT.
 */

#ifndef AEGIS_PRECOMPILE_H
#define AEGIS_PRECOMPILE_H

#include "aegis_types.h"
#include "aegis_smt.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Proposed EVM Precompile Address for Aegis Cryptographic Accumulator (e.g. 0x0b or 0x20) */
#define AEGIS_PRECOMPILE_ADDRESS 0x20

/** Base gas cost for precompile execution */
#define AEGIS_PRECOMPILE_BASE_GAS 450

/** Gas cost per proof level (depth) */
#define AEGIS_PRECOMPILE_GAS_PER_LEVEL 85

/**
 * @brief Precompile entry point conforming to Ethereum Client Engine specification.
 *
 * Calldata format:
 *   [0x00 .. 0x20]  Root (bytes32)
 *   [0x20 .. 0x40]  Key  (bytes32)
 *   [0x40 .. 0x60]  Value(bytes32)
 *   [0x60 .. 0x80]  Depth / Sibling count N (uint256)
 *   [0x80 .. 0x80 + N*32] Sibling hashes (bytes32[N])
 *
 * Output format:
 *   [0x00 .. 0x20]  0x01 (if valid) or 0x00 (if invalid)
 *
 * @param input Raw calldata input buffer
 * @param input_len Length of input calldata
 * @param output Output buffer of at least 32 bytes
 * @param max_output_len Capacity of output buffer
 * @param actual_output_len Pointer to write number of bytes returned (32)
 * @param gas_cost Pointer to write calculated gas cost
 * @return 0 on success (evaluation completed), non-zero on malformed input
 */
int aegis_precompile_eval(const uint8_t *input,
                          size_t input_len,
                          uint8_t *output,
                          size_t max_output_len,
                          size_t *actual_output_len,
                          uint64_t *gas_cost);

/**
 * @brief Encode proof and parameters into standard EVM ABI calldata for Solidity function call:
 * verifyProof(bytes32 root, bytes32 key, bytes32 value, bytes32[] siblings)
 */
aegis_status_t aegis_encode_solidity_calldata(const uint8_t root[AEGIS_WORD_SIZE],
                                              const uint8_t key[AEGIS_WORD_SIZE],
                                              const uint8_t value[AEGIS_WORD_SIZE],
                                              const aegis_smt_proof_t *proof,
                                              uint8_t *out_buf,
                                              size_t buf_capacity,
                                              size_t *out_len);

#ifdef __cplusplus
}
#endif

#endif /* AEGIS_PRECOMPILE_H */
