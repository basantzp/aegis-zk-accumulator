/**
 * @file test_smt.c
 * @brief Unit tests for Sparse Merkle Tree (SMT), proof generation, and precompile evaluation.
 */

#include "aegis_precompile.h"
#include "aegis_smt.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_empty_tree_root(void) {
    aegis_smt_t *tree = NULL;
    aegis_status_t s = aegis_smt_create(&tree, 32);
    assert(s == AEGIS_SUCCESS);

    uint8_t root[32];
    s = aegis_smt_get_root(tree, root);
    assert(s == AEGIS_SUCCESS);

    uint8_t expected_zero_root[32];
    s = aegis_smt_get_zero_hash(32, expected_zero_root);
    assert(s == AEGIS_SUCCESS);

    assert(memcmp(root, expected_zero_root, 32) == 0);
    printf("[TEST] Empty tree root correctly equals level-32 zero hash.\n");

    aegis_smt_free(tree);
}

static void test_insert_and_proof(void) {
    aegis_smt_t *tree = NULL;
    aegis_status_t s = aegis_smt_create(&tree, 16);
    assert(s == AEGIS_SUCCESS);

    uint8_t key[32] = {0};
    key[0] = 0xaa;
    key[1] = 0xbb;
    key[31] = 0x01;

    uint8_t value[32] = {0};
    value[0] = 0xde;
    value[1] = 0xad;
    value[31] = 0xff;

    /* Insert */
    s = aegis_smt_insert(tree, key, value);
    assert(s == AEGIS_SUCCESS);
    assert(aegis_smt_leaf_count(tree) == 1);

    /* Query */
    uint8_t retrieved[32];
    s = aegis_smt_get(tree, key, retrieved);
    assert(s == AEGIS_SUCCESS);
    assert(memcmp(value, retrieved, 32) == 0);

    /* Get Root */
    uint8_t root[32];
    s = aegis_smt_get_root(tree, root);
    assert(s == AEGIS_SUCCESS);

    /* Generate Proof */
    aegis_smt_proof_t proof;
    s = aegis_smt_generate_proof(tree, key, &proof);
    assert(s == AEGIS_SUCCESS);
    assert(proof.depth == 16);

    /* Verify Proof (Valid) */
    s = aegis_smt_verify_proof(root, key, value, &proof);
    assert(s == AEGIS_SUCCESS);
    printf("[TEST] Valid SMT inclusion proof verified successfully!\n");

    /* Tampered Value (Must Fail) */
    uint8_t bad_value[32];
    memcpy(bad_value, value, 32);
    bad_value[31] ^= 0x01;
    s = aegis_smt_verify_proof(root, key, bad_value, &proof);
    assert(s == AEGIS_ERR_VERIFY_FAILED);
    printf("[TEST] Tampered value correctly rejected!\n");

    /* Tampered Sibling (Must Fail) */
    aegis_smt_proof_t bad_proof = proof;
    bad_proof.siblings[0].data[0] ^= 0xff;
    s = aegis_smt_verify_proof(root, key, value, &bad_proof);
    assert(s == AEGIS_ERR_VERIFY_FAILED);
    printf("[TEST] Tampered sibling correctly rejected!\n");

    aegis_smt_free(tree);
}

static void test_multiple_leaves(void) {
    aegis_smt_t *tree = NULL;
    aegis_status_t s = aegis_smt_create(&tree, 32);
    assert(s == AEGIS_SUCCESS);

    const int NUM_LEAVES = 50;
    uint8_t keys[50][32];
    uint8_t values[50][32];

    for (int i = 0; i < NUM_LEAVES; i++) {
        memset(keys[i], 0, 32);
        memset(values[i], 0, 32);
        keys[i][3] = (uint8_t)(i + 1);
        values[i][31] = (uint8_t)(i * 7 + 3);

        s = aegis_smt_insert(tree, keys[i], values[i]);
        assert(s == AEGIS_SUCCESS);
    }

    assert(aegis_smt_leaf_count(tree) == (size_t)NUM_LEAVES);

    uint8_t root[32];
    s = aegis_smt_get_root(tree, root);
    assert(s == AEGIS_SUCCESS);

    /* Verify each leaf */
    for (int i = 0; i < NUM_LEAVES; i++) {
        aegis_smt_proof_t proof;
        s = aegis_smt_generate_proof(tree, keys[i], &proof);
        assert(s == AEGIS_SUCCESS);

        s = aegis_smt_verify_proof(root, keys[i], values[i], &proof);
        assert(s == AEGIS_SUCCESS);
    }
    printf("[TEST] %d distinct leaves inserted and verified in 32-level tree!\n", NUM_LEAVES);

    aegis_smt_free(tree);
}

static void test_precompile_evaluator(void) {
    aegis_smt_t *tree = NULL;
    aegis_status_t s = aegis_smt_create(&tree, 8);
    assert(s == AEGIS_SUCCESS);

    uint8_t key[32] = {0};
    key[31] = 0x42;
    uint8_t value[32] = {0};
    value[31] = 0x99;
    s = aegis_smt_insert(tree, key, value);
    assert(s == AEGIS_SUCCESS);

    uint8_t root[32];
    s = aegis_smt_get_root(tree, root);
    assert(s == AEGIS_SUCCESS);

    aegis_smt_proof_t proof;
    s = aegis_smt_generate_proof(tree, key, &proof);
    assert(s == AEGIS_SUCCESS);

    /* Prepare raw precompile calldata: root(32) + key(32) + value(32) + depth(32) + siblings(8*32) */
    size_t calldata_len = 128 + 8 * 32;
    uint8_t *calldata = (uint8_t *)calloc(1, calldata_len);
    assert(calldata != NULL);

    memcpy(calldata, root, 32);
    memcpy(calldata + 32, key, 32);
    memcpy(calldata + 64, value, 32);
    calldata[127] = 8; /* depth = 8 */
    for (int i = 0; i < 8; i++) {
        memcpy(calldata + 128 + i * 32, proof.siblings[i].data, 32);
    }

    uint8_t out[32];
    size_t actual_out_len = 0;
    uint64_t gas_cost = 0;
    int res = aegis_precompile_eval(calldata, calldata_len, out, 32, &actual_out_len, &gas_cost);
    assert(res == 0);
    assert(actual_out_len == 32);
    assert(out[31] == 0x01); /* Valid */
    assert(gas_cost == AEGIS_PRECOMPILE_BASE_GAS + 8 * AEGIS_PRECOMPILE_GAS_PER_LEVEL);
    printf("[TEST] Precompile evaluation succeeded! Gas calculated: %lu gas.\n", (unsigned long)gas_cost);

    /* Tamper calldata root -> should evaluate to 0x00 */
    calldata[0] ^= 0x01;
    res = aegis_precompile_eval(calldata, calldata_len, out, 32, &actual_out_len, &gas_cost);
    assert(res == 0);
    assert(out[31] == 0x00); /* Invalid */
    printf("[TEST] Precompile correctly rejected tampered input!\n");

    free(calldata);
    aegis_smt_free(tree);
}

static void test_solidity_calldata_encoder(void) {
    uint8_t root[32] = {1};
    uint8_t key[32] = {2};
    uint8_t value[32] = {3};

    aegis_smt_proof_t proof;
    proof.depth = 4;
    for (int i = 0; i < 4; i++) {
        memset(proof.siblings[i].data, (uint8_t)(i + 10), 32);
    }

    uint8_t buffer[512];
    size_t out_len = 0;
    aegis_status_t s = aegis_encode_solidity_calldata(root, key, value, &proof, buffer, sizeof(buffer), &out_len);
    assert(s == AEGIS_SUCCESS);
    assert(out_len == 4 + 32 * 5 + 32 * 4); /* 4 + 160 + 128 = 292 bytes */

    /* Verify function selector: 0x532810b1 */
    assert(buffer[0] == 0x53 && buffer[1] == 0x28 && buffer[2] == 0x10 && buffer[3] == 0xb1);
    printf("[TEST] Solidity ABI calldata encoder output verified (%zu bytes)!\n", out_len);
}

static void test_depth_256_full_tree(void) {
    aegis_smt_t *tree = NULL;
    aegis_status_t s = aegis_smt_create(&tree, 256);
    assert(s == AEGIS_SUCCESS);

    uint8_t key[32] = {0};
    key[0] = 0x80;  /* MSB set */
    key[31] = 0x01; /* LSB set */

    uint8_t value[32] = {0};
    value[0] = 0x12;
    value[31] = 0x34;

    s = aegis_smt_insert(tree, key, value);
    assert(s == AEGIS_SUCCESS);
    assert(aegis_smt_leaf_count(tree) == 1);

    uint8_t root[32];
    s = aegis_smt_get_root(tree, root);
    assert(s == AEGIS_SUCCESS);

    aegis_smt_proof_t proof;
    s = aegis_smt_generate_proof(tree, key, &proof);
    assert(s == AEGIS_SUCCESS);
    assert(proof.depth == 256);

    s = aegis_smt_verify_proof(root, key, value, &proof);
    assert(s == AEGIS_SUCCESS);

    /* Tampering leaf value must fail verification */
    uint8_t bad_value[32];
    memcpy(bad_value, value, 32);
    bad_value[31] ^= 0x01;
    s = aegis_smt_verify_proof(root, key, bad_value, &proof);
    assert(s == AEGIS_ERR_VERIFY_FAILED);

    printf("[TEST] Full 256-level depth SMT insertion, proof generation, and verification verified!\n");
    aegis_smt_free(tree);
}

static void test_precompile_depth_overflow_attack(void) {
    /* Test attack where upper 28 bytes of uint256 depth word are non-zero (truncation bypass attempt) */
    size_t calldata_len = 128 + 8 * 32;
    uint8_t *calldata = (uint8_t *)calloc(1, calldata_len);
    assert(calldata != NULL);

    /* Lower 4 bytes say depth = 8 */
    calldata[127] = 8;
    /* Upper bytes have non-zero value: 1 << 64 */
    calldata[100] = 0x01;

    uint8_t out[32];
    size_t actual_out_len = 0;
    uint64_t gas_cost = 0;
    int res = aegis_precompile_eval(calldata, calldata_len, out, 32, &actual_out_len, &gas_cost);
    assert(res == 0);
    assert(out[31] == 0x00); /* Must reject with 0 */

    /* Also test depth > 256 (e.g. depth = 257) */
    memset(calldata + 96, 0, 32);
    calldata[126] = 0x01;
    calldata[127] = 0x01; /* 257 */
    res = aegis_precompile_eval(calldata, calldata_len, out, 32, &actual_out_len, &gas_cost);
    assert(res == 0);
    assert(out[31] == 0x00); /* Must reject with 0 */

    free(calldata);
    printf("[TEST] Precompile uint256 depth truncation attack and depth bounds correctly rejected!\n");
}

int main(void) {
    printf("=== Aegis SMT & Precompile Test Suite ===\n");
    test_empty_tree_root();
    test_insert_and_proof();
    test_multiple_leaves();
    test_precompile_evaluator();
    test_solidity_calldata_encoder();
    test_depth_256_full_tree();
    test_precompile_depth_overflow_attack();
    printf("=== ALL SMT TESTS PASSED ===\n");
    return 0;
}
