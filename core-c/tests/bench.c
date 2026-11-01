/**
 * @file bench.c
 * @brief High-resolution microbenchmark suite for Aegis ZK Accumulator C Engine.
 * @copyright (c) 2026 Aegis Project Contributors. Apache-2.0 / MIT.
 */

#define _POSIX_C_SOURCE 199309L

#include "aegis_smt.h"
#include "aegis_precompile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double get_elapsed_seconds(struct timespec start, struct timespec end) {
    return (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) / 1e9;
}

static void bench_keccak_hash(void) {
    const int ITERATIONS = 500000;
    uint8_t left[32] = {0x01};
    uint8_t right[32] = {0x02};
    uint8_t out[32];

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < ITERATIONS; i++) {
        left[0] = (uint8_t)i;
        aegis_hash_nodes(left, right, out);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = get_elapsed_seconds(start, end);
    double ops_sec = (double)ITERATIONS / elapsed;

    printf("[BENCHMARK] Keccak-256 (64-byte 2-to-1): %d ops in %.4f s -> %.2f ops/sec (%.2f ns/op)\n",
           ITERATIONS, elapsed, ops_sec, (elapsed / ITERATIONS) * 1e9);
}

static void bench_poseidon_hash(void) {
    const int ITERATIONS = 1000000;
    uint8_t left[32] = {0x05};
    uint8_t right[32] = {0x07};
    uint8_t out[32];

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < ITERATIONS; i++) {
        left[0] = (uint8_t)i;
        aegis_poseidon_hash(left, right, out);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = get_elapsed_seconds(start, end);
    double ops_sec = (double)ITERATIONS / elapsed;

    printf("[BENCHMARK] Poseidon BN254:            %d ops in %.4f s -> %.2f ops/sec (%.2f ns/op)\n",
           ITERATIONS, elapsed, ops_sec, (elapsed / ITERATIONS) * 1e9);
}

static void bench_smt_lifecycle(void) {
    const int TREE_DEPTH = 32;
    const int NUM_LEAVES = 10000;

    aegis_smt_t *tree = NULL;
    aegis_smt_create(&tree, TREE_DEPTH);

    uint8_t key[32] = {0};
    uint8_t value[32] = {0};

    /* Benchmark Insertions */
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < NUM_LEAVES; i++) {
        key[2] = (uint8_t)(i >> 8);
        key[3] = (uint8_t)i;
        value[31] = (uint8_t)(i + 1);
        aegis_smt_insert(tree, key, value);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double insert_elapsed = get_elapsed_seconds(start, end);
    printf("[BENCHMARK] SMT-%d Insert:             %d leaves in %.4f s -> %.2f inserts/sec\n",
           TREE_DEPTH, NUM_LEAVES, insert_elapsed, (double)NUM_LEAVES / insert_elapsed);

    uint8_t root[32];
    aegis_smt_get_root(tree, root);

    /* Benchmark Proof Generation */
    const int PROOF_SAMPLES = 5000;
    aegis_smt_proof_t proof;

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < PROOF_SAMPLES; i++) {
        key[2] = (uint8_t)(i >> 8);
        key[3] = (uint8_t)i;
        aegis_smt_generate_proof(tree, key, &proof);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    double proof_gen_elapsed = get_elapsed_seconds(start, end);
    printf("[BENCHMARK] SMT-%d Proof Generation:   %d proofs in %.4f s -> %.2f proofs/sec\n",
           TREE_DEPTH, PROOF_SAMPLES, proof_gen_elapsed, (double)PROOF_SAMPLES / proof_gen_elapsed);

    /* Benchmark In-Memory Proof Verification */
    const int VERIFY_SAMPLES = 200000;
    key[2] = 0; key[3] = 42;
    value[31] = 43;
    aegis_smt_generate_proof(tree, key, &proof);

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < VERIFY_SAMPLES; i++) {
        aegis_smt_verify_proof(root, key, value, &proof);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    double verify_elapsed = get_elapsed_seconds(start, end);
    printf("[BENCHMARK] SMT-%d Proof Verification: %d verifications in %.4f s -> %.2f verif/sec (%.2f ns/verif)\n",
           TREE_DEPTH, VERIFY_SAMPLES, verify_elapsed, (double)VERIFY_SAMPLES / verify_elapsed, (verify_elapsed / VERIFY_SAMPLES) * 1e9);

    aegis_smt_free(tree);
}

int main(void) {
    printf("==================================================================\n");
    printf("         AEGIS ZK ACCUMULATOR C ENGINE PERFORMANCE SUITE          \n");
    printf("==================================================================\n");
    bench_keccak_hash();
    bench_poseidon_hash();
    bench_smt_lifecycle();
    printf("==================================================================\n");
    return 0;
}
