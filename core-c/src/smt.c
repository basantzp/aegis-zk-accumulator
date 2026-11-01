/**
 * @file smt.c
 * @brief Sparse Merkle Tree (SMT) with Patricia Radix pruning and proof generation.
 * @copyright (c) 2026 Aegis Project Contributors. Apache-2.0 / MIT.
 */

#include "aegis_smt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Internal tree node */
typedef struct smt_node {
    uint8_t hash[AEGIS_WORD_SIZE];
    struct smt_node *left;
    struct smt_node *right;
    bool is_leaf;
    uint8_t key[AEGIS_WORD_SIZE];
    uint8_t value[AEGIS_WORD_SIZE];
} smt_node_t;

/* Concrete SMT structure */
struct aegis_smt {
    uint32_t depth;
    smt_node_t *root;
    size_t leaf_count;
    uint8_t zero_hashes[AEGIS_MAX_TREE_DEPTH + 1][AEGIS_WORD_SIZE];
};

static uint8_t get_key_bit(const uint8_t key[AEGIS_WORD_SIZE], uint32_t bit_idx) {
    if (bit_idx >= 256)
        return 0;
    uint32_t byte_idx = bit_idx / 8;
    uint32_t bit_pos = 7 - (bit_idx % 8);
    return (key[byte_idx] >> bit_pos) & 1;
}

static void precompute_zero_hashes(aegis_smt_t *tree) {
    memset(tree->zero_hashes[0], 0, AEGIS_WORD_SIZE);
    for (uint32_t d = 1; d <= tree->depth; d++) {
        aegis_hash_nodes(tree->zero_hashes[d - 1], tree->zero_hashes[d - 1], tree->zero_hashes[d]);
    }
}

aegis_status_t aegis_smt_create(aegis_smt_t **out_tree, uint32_t depth) {
    if (!out_tree)
        return AEGIS_ERR_NULL_PTR;
    if (depth == 0 || depth > AEGIS_MAX_TREE_DEPTH)
        return AEGIS_ERR_INVALID_DEPTH;

    aegis_smt_t *tree = (aegis_smt_t *)calloc(1, sizeof(aegis_smt_t));
    if (!tree)
        return AEGIS_ERR_OUT_OF_MEMORY;

    tree->depth = depth;
    tree->leaf_count = 0;
    tree->root = NULL;
    precompute_zero_hashes(tree);

    *out_tree = tree;
    return AEGIS_SUCCESS;
}

static void free_node(smt_node_t *node) {
    if (!node)
        return;
    free_node(node->left);
    free_node(node->right);
    free(node);
}

void aegis_smt_free(aegis_smt_t *tree) {
    if (!tree)
        return;
    free_node(tree->root);
    free(tree);
}

aegis_status_t aegis_smt_leaf_hash(const uint8_t key[AEGIS_WORD_SIZE], const uint8_t value[AEGIS_WORD_SIZE],
                                   uint8_t leaf_hash[AEGIS_WORD_SIZE]) {
    if (!key || !value || !leaf_hash)
        return AEGIS_ERR_NULL_PTR;

    /* Check if value is zero -> empty leaf has precomputed zero hash */
    aegis_bytes32_t val_word;
    memcpy(val_word.data, value, AEGIS_WORD_SIZE);
    if (aegis_bytes32_is_zero(&val_word)) {
        memset(leaf_hash, 0, AEGIS_WORD_SIZE);
        return AEGIS_SUCCESS;
    }

    /* leaf = keccak256(key || value) */
    uint8_t buffer[64];
    memcpy(buffer, key, AEGIS_WORD_SIZE);
    memcpy(buffer + AEGIS_WORD_SIZE, value, AEGIS_WORD_SIZE);
    return aegis_keccak256(buffer, 64, leaf_hash);
}

static const uint8_t *get_node_hash(const aegis_smt_t *tree, const smt_node_t *node, uint32_t level) {
    if (node) {
        return node->hash;
    }
    uint32_t height = tree->depth - level;
    return tree->zero_hashes[height];
}

static smt_node_t *insert_recursive(aegis_smt_t *tree, smt_node_t *node, uint32_t level,
                                    const uint8_t key[AEGIS_WORD_SIZE], const uint8_t value[AEGIS_WORD_SIZE],
                                    bool *is_new) {
    if (level == tree->depth) {
        if (!node) {
            node = (smt_node_t *)calloc(1, sizeof(smt_node_t));
            if (!node)
                return NULL;
            *is_new = true;
        }
        node->is_leaf = true;
        memcpy(node->key, key, AEGIS_WORD_SIZE);
        memcpy(node->value, value, AEGIS_WORD_SIZE);
        aegis_smt_leaf_hash(key, value, node->hash);
        return node;
    }

    if (!node) {
        node = (smt_node_t *)calloc(1, sizeof(smt_node_t));
        if (!node)
            return NULL;
    }

    uint8_t bit = get_key_bit(key, level);
    if (bit == 0) {
        node->left = insert_recursive(tree, node->left, level + 1, key, value, is_new);
    } else {
        node->right = insert_recursive(tree, node->right, level + 1, key, value, is_new);
    }

    const uint8_t *left_h = get_node_hash(tree, node->left, level + 1);
    const uint8_t *right_h = get_node_hash(tree, node->right, level + 1);
    aegis_hash_nodes(left_h, right_h, node->hash);

    return node;
}

aegis_status_t aegis_smt_insert(aegis_smt_t *tree, const uint8_t key[AEGIS_WORD_SIZE],
                                const uint8_t value[AEGIS_WORD_SIZE]) {
    if (!tree || !key || !value)
        return AEGIS_ERR_NULL_PTR;

    bool is_new = false;
    smt_node_t *new_root = insert_recursive(tree, tree->root, 0, key, value, &is_new);
    if (!new_root)
        return AEGIS_ERR_OUT_OF_MEMORY;

    tree->root = new_root;
    if (is_new) {
        tree->leaf_count++;
    }
    return AEGIS_SUCCESS;
}

aegis_status_t aegis_smt_get(const aegis_smt_t *tree, const uint8_t key[AEGIS_WORD_SIZE],
                             uint8_t out_value[AEGIS_WORD_SIZE]) {
    if (!tree || !key || !out_value)
        return AEGIS_ERR_NULL_PTR;

    smt_node_t *curr = tree->root;
    for (uint32_t level = 0; level < tree->depth; level++) {
        if (!curr) {
            memset(out_value, 0, AEGIS_WORD_SIZE);
            return AEGIS_SUCCESS;
        }
        uint8_t bit = get_key_bit(key, level);
        curr = (bit == 0) ? curr->left : curr->right;
    }

    if (curr && curr->is_leaf) {
        memcpy(out_value, curr->value, AEGIS_WORD_SIZE);
    } else {
        memset(out_value, 0, AEGIS_WORD_SIZE);
    }
    return AEGIS_SUCCESS;
}

aegis_status_t aegis_smt_get_root(const aegis_smt_t *tree, uint8_t out_root[AEGIS_WORD_SIZE]) {
    if (!tree || !out_root)
        return AEGIS_ERR_NULL_PTR;

    const uint8_t *root_h = get_node_hash(tree, tree->root, 0);
    memcpy(out_root, root_h, AEGIS_WORD_SIZE);
    return AEGIS_SUCCESS;
}

aegis_status_t aegis_smt_generate_proof(const aegis_smt_t *tree, const uint8_t key[AEGIS_WORD_SIZE],
                                        aegis_smt_proof_t *out_proof) {
    if (!tree || !key || !out_proof)
        return AEGIS_ERR_NULL_PTR;
    if (tree->depth == 0 || tree->depth > AEGIS_MAX_TREE_DEPTH)
        return AEGIS_ERR_INVALID_DEPTH;

    out_proof->depth = tree->depth;
    smt_node_t *curr = tree->root;

    for (uint32_t level = 0; level < tree->depth && level < AEGIS_MAX_TREE_DEPTH; level++) {
        uint8_t bit = get_key_bit(key, level);
        out_proof->directions[level] = bit;

        if (bit == 0) {
            /* Target goes left; sibling is right child */
            const smt_node_t *sibling_node = curr ? curr->right : NULL;
            const uint8_t *sib_hash = get_node_hash(tree, sibling_node, level + 1);
            memcpy(out_proof->siblings[level].data, sib_hash, AEGIS_WORD_SIZE);
            curr = curr ? curr->left : NULL;
        } else {
            /* Target goes right; sibling is left child */
            const smt_node_t *sibling_node = curr ? curr->left : NULL;
            const uint8_t *sib_hash = get_node_hash(tree, sibling_node, level + 1);
            memcpy(out_proof->siblings[level].data, sib_hash, AEGIS_WORD_SIZE);
            curr = curr ? curr->right : NULL;
        }
    }

    return AEGIS_SUCCESS;
}

aegis_status_t aegis_smt_calculate_root_from_proof(const uint8_t leaf_hash[AEGIS_WORD_SIZE],
                                                   const aegis_smt_proof_t *proof,
                                                   uint8_t out_computed_root[AEGIS_WORD_SIZE]) {
    if (!leaf_hash || !proof || !out_computed_root)
        return AEGIS_ERR_NULL_PTR;
    if (proof->depth == 0 || proof->depth > AEGIS_MAX_TREE_DEPTH)
        return AEGIS_ERR_INVALID_DEPTH;

    uint8_t current[AEGIS_WORD_SIZE];
    memcpy(current, leaf_hash, AEGIS_WORD_SIZE);

    /* Traverse from leaves up to root */
    for (int i = (int)proof->depth - 1; i >= 0; i--) {
        uint8_t next[AEGIS_WORD_SIZE];
        if (proof->directions[i] == 0) {
            /* Current is left child, sibling is right child */
            aegis_hash_nodes(current, proof->siblings[i].data, next);
        } else {
            /* Current is right child, sibling is left child */
            aegis_hash_nodes(proof->siblings[i].data, current, next);
        }
        memcpy(current, next, AEGIS_WORD_SIZE);
    }

    memcpy(out_computed_root, current, AEGIS_WORD_SIZE);
    return AEGIS_SUCCESS;
}

aegis_status_t aegis_smt_verify_proof(const uint8_t root[AEGIS_WORD_SIZE], const uint8_t key[AEGIS_WORD_SIZE],
                                      const uint8_t value[AEGIS_WORD_SIZE], const aegis_smt_proof_t *proof) {
    if (!root || !key || !value || !proof)
        return AEGIS_ERR_NULL_PTR;
    if (proof->depth == 0 || proof->depth > AEGIS_MAX_TREE_DEPTH)
        return AEGIS_ERR_INVALID_DEPTH;

    uint8_t leaf_h[AEGIS_WORD_SIZE];
    aegis_status_t s = aegis_smt_leaf_hash(key, value, leaf_h);
    if (s != AEGIS_SUCCESS)
        return s;

    /* Verify directions match key bits */
    for (uint32_t i = 0; i < proof->depth && i < AEGIS_MAX_TREE_DEPTH; i++) {
        uint8_t bit = get_key_bit(key, i);
        if (proof->directions[i] != bit) {
            return AEGIS_ERR_VERIFY_FAILED;
        }
    }

    uint8_t computed_root[AEGIS_WORD_SIZE];
    s = aegis_smt_calculate_root_from_proof(leaf_h, proof, computed_root);
    if (s != AEGIS_SUCCESS)
        return s;

    if (memcmp(root, computed_root, AEGIS_WORD_SIZE) != 0) {
        return AEGIS_ERR_VERIFY_FAILED;
    }

    return AEGIS_SUCCESS;
}

aegis_status_t aegis_smt_get_zero_hash(uint32_t level, uint8_t out_zero[AEGIS_WORD_SIZE]) {
    if (!out_zero)
        return AEGIS_ERR_NULL_PTR;
    if (level > AEGIS_MAX_TREE_DEPTH)
        return AEGIS_ERR_OUT_OF_BOUNDS;

    /* Static cache for zero hashes up to MAX_DEPTH */
    static bool initialized = false;
    static uint8_t zeros[AEGIS_MAX_TREE_DEPTH + 1][AEGIS_WORD_SIZE];

    if (!initialized) {
        memset(zeros[0], 0, AEGIS_WORD_SIZE);
        for (uint32_t d = 1; d <= AEGIS_MAX_TREE_DEPTH; d++) {
            aegis_hash_nodes(zeros[d - 1], zeros[d - 1], zeros[d]);
        }
        initialized = true;
    }

    memcpy(out_zero, zeros[level], AEGIS_WORD_SIZE);
    return AEGIS_SUCCESS;
}

size_t aegis_smt_leaf_count(const aegis_smt_t *tree) {
    return tree ? tree->leaf_count : 0;
}
