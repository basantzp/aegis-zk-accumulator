/**
 * @file aegis_smt.h
 * @brief Sparse Merkle Tree (SMT) and Accumulator interface.
 * @copyright (c) 2026 Aegis Project Contributors. Apache-2.0 / MIT.
 */

#ifndef AEGIS_SMT_H
#define AEGIS_SMT_H

#include "aegis_types.h"
#include "aegis_crypto.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque handle to Sparse Merkle Tree */
typedef struct aegis_smt aegis_smt_t;

/**
 * @brief Initialize a new Sparse Merkle Tree with the given depth.
 * @param out_tree Pointer to handle that will receive the new tree.
 * @param depth Tree depth (1 to 256; standard rollup tree is 32 or 256).
 * @return AEGIS_SUCCESS on success.
 */
aegis_status_t aegis_smt_create(aegis_smt_t **out_tree, uint32_t depth);

/**
 * @brief Destroy a Sparse Merkle Tree and release all allocated memory.
 */
void aegis_smt_free(aegis_smt_t *tree);

/**
 * @brief Compute the canonical leaf hash from key and value:
 * leaf = keccak256(key || value)
 */
aegis_status_t aegis_smt_leaf_hash(const uint8_t key[AEGIS_WORD_SIZE],
                                   const uint8_t value[AEGIS_WORD_SIZE],
                                   uint8_t leaf_hash[AEGIS_WORD_SIZE]);

/**
 * @brief Insert or update a key-value pair in the tree.
 * Updates all parent nodes up to the root in logarithmic time.
 */
aegis_status_t aegis_smt_insert(aegis_smt_t *tree,
                                const uint8_t key[AEGIS_WORD_SIZE],
                                const uint8_t value[AEGIS_WORD_SIZE]);

/**
 * @brief Retrieve value for a given key. If key does not exist, returns all zeroes.
 */
aegis_status_t aegis_smt_get(const aegis_smt_t *tree,
                             const uint8_t key[AEGIS_WORD_SIZE],
                             uint8_t out_value[AEGIS_WORD_SIZE]);

/**
 * @brief Retrieve current 32-byte Merkle root.
 */
aegis_status_t aegis_smt_get_root(const aegis_smt_t *tree,
                                  uint8_t out_root[AEGIS_WORD_SIZE]);

/**
 * @brief Generate a cryptographic Merkle proof for a given key.
 * Can be used for both membership (value != 0) and non-membership (value == 0).
 */
aegis_status_t aegis_smt_generate_proof(const aegis_smt_t *tree,
                                        const uint8_t key[AEGIS_WORD_SIZE],
                                        aegis_smt_proof_t *out_proof);

/**
 * @brief Verify a Merkle proof against a known root in pure C (O(depth) time, zero allocation).
 */
aegis_status_t aegis_smt_verify_proof(const uint8_t root[AEGIS_WORD_SIZE],
                                      const uint8_t key[AEGIS_WORD_SIZE],
                                      const uint8_t value[AEGIS_WORD_SIZE],
                                      const aegis_smt_proof_t *proof);

/**
 * @brief Recompute root from a leaf hash and proof path.
 */
aegis_status_t aegis_smt_calculate_root_from_proof(const uint8_t leaf_hash[AEGIS_WORD_SIZE],
                                                   const aegis_smt_proof_t *proof,
                                                   uint8_t out_computed_root[AEGIS_WORD_SIZE]);

/**
 * @brief Get precomputed zero-hash for a given tree level.
 */
aegis_status_t aegis_smt_get_zero_hash(uint32_t level, uint8_t out_zero[AEGIS_WORD_SIZE]);

/**
 * @brief Returns total number of active non-zero leaves in tree.
 */
size_t aegis_smt_leaf_count(const aegis_smt_t *tree);

#ifdef __cplusplus
}
#endif

#endif /* AEGIS_SMT_H */
