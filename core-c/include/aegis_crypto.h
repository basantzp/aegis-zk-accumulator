/**
 * @file aegis_crypto.h
 * @brief Cryptographic hash functions and byte utilities for Aegis Accumulator.
 * @copyright (c) 2026 Aegis Project Contributors. Apache-2.0 / MIT.
 */

#ifndef AEGIS_CRYPTO_H
#define AEGIS_CRYPTO_H

#include "aegis_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Standard Ethereum Keccak-256 hash.
 * @param msg Pointer to input byte buffer.
 * @param msg_len Length of input buffer in bytes.
 * @param out Buffer of at least 32 bytes to store the resulting digest.
 * @return AEGIS_SUCCESS on success, error code otherwise.
 */
aegis_status_t aegis_keccak256(const uint8_t *msg, size_t msg_len, uint8_t out[AEGIS_WORD_SIZE]);

/**
 * @brief Ultra-fast 2-to-1 Merkle node compressor: keccak256(left || right).
 * Optimized to operate strictly in L1 cache with zero heap allocations.
 * @param left Left child 32-byte hash.
 * @param right Right child 32-byte hash.
 * @param out 32-byte parent node output buffer.
 * @return AEGIS_SUCCESS on success.
 */
aegis_status_t aegis_hash_nodes(const uint8_t left[AEGIS_WORD_SIZE],
                                const uint8_t right[AEGIS_WORD_SIZE],
                                uint8_t out[AEGIS_WORD_SIZE]);

/**
 * @brief ZK-friendly Poseidon 2-to-1 compression function over BN254 scalar field.
 * @param left 32-byte scalar input 1.
 * @param right 32-byte scalar input 2.
 * @param out 32-byte scalar output digest.
 * @return AEGIS_SUCCESS on success.
 */
aegis_status_t aegis_poseidon_hash(const uint8_t left[AEGIS_WORD_SIZE],
                                   const uint8_t right[AEGIS_WORD_SIZE],
                                   uint8_t out[AEGIS_WORD_SIZE]);

/**
 * @brief Check if a 32-byte word is all zeroes.
 */
bool aegis_bytes32_is_zero(const aegis_bytes32_t *b);

/**
 * @brief Constant-time equality check for two 32-byte words.
 */
bool aegis_bytes32_equal(const aegis_bytes32_t *a, const aegis_bytes32_t *b);

/**
 * @brief Convert hexadecimal string (with optional 0x prefix) to byte array.
 */
aegis_status_t aegis_hex_to_bytes(const char *hex_str, uint8_t *out, size_t out_len);

/**
 * @brief Convert byte array to hexadecimal string (lowercase, with 0x prefix).
 * Buffer out_hex must have size at least (len * 2 + 3).
 */
aegis_status_t aegis_bytes_to_hex(const uint8_t *bytes, size_t len, char *out_hex);

#ifdef __cplusplus
}
#endif

#endif /* AEGIS_CRYPTO_H */
