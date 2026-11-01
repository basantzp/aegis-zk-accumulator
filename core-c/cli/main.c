/**
 * @file main.c
 * @brief Aegis Accumulator CLI tool: proof generator, test-vector exporter, and precompile runner.
 * @copyright (c) 2026 Aegis Project Contributors. Apache-2.0 / MIT.
 */

#include "aegis_precompile.h"
#include "aegis_smt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *prog_name) {
    printf("Aegis ZK Cryptographic Accumulator & EVM Precompile Engine (v1.0.0)\n");
    printf("Usage: %s [command] [options]\n\n", prog_name);
    printf("Commands:\n");
    printf("  --demo                  Generate a sample 16-level tree and verify inclusion\n");
    printf("  --export-test-vector    Print JSON test vector and Solidity ABI calldata\n");
    printf("  --eval-precompile <hex> Run precompile evaluation on raw calldata hex\n");
    printf("  --version               Display version and cryptographic specifications\n");
}

static void run_demo(void) {
    printf("=== Aegis Cryptographic State Accumulator Demo ===\n");
    aegis_smt_t *tree = NULL;
    aegis_smt_create(&tree, 16);

    uint8_t key[32] = {0};
    uint8_t value[32] = {0};
    key[0] = 0xca;
    key[1] = 0xfe;
    key[31] = 0x01;
    value[0] = 0xba;
    value[1] = 0xbe;
    value[31] = 0x77;

    aegis_smt_insert(tree, key, value);

    uint8_t root[32];
    aegis_smt_get_root(tree, root);

    char hex_root[67], hex_key[67], hex_val[67];
    aegis_bytes_to_hex(root, 32, hex_root);
    aegis_bytes_to_hex(key, 32, hex_key);
    aegis_bytes_to_hex(value, 32, hex_val);

    printf("Tree Depth:  16\n");
    printf("State Root:  %s\n", hex_root);
    printf("Leaf Key:    %s\n", hex_key);
    printf("Leaf Value:  %s\n", hex_val);

    aegis_smt_proof_t proof;
    aegis_smt_generate_proof(tree, key, &proof);
    aegis_status_t s = aegis_smt_verify_proof(root, key, value, &proof);

    printf("Proof Status: %s\n", (s == AEGIS_SUCCESS) ? "VALID (Verified)" : "INVALID");
    aegis_smt_free(tree);
}

static void run_export_test_vector(void) {
    aegis_smt_t *tree = NULL;
    aegis_smt_create(&tree, 8);

    uint8_t key[32] = {0};
    key[31] = 0x42;
    uint8_t value[32] = {0};
    value[31] = 0x99;
    aegis_smt_insert(tree, key, value);

    uint8_t root[32];
    aegis_smt_get_root(tree, root);

    aegis_smt_proof_t proof;
    aegis_smt_generate_proof(tree, key, &proof);

    char hex_root[67], hex_key[67], hex_val[67];
    aegis_bytes_to_hex(root, 32, hex_root);
    aegis_bytes_to_hex(key, 32, hex_key);
    aegis_bytes_to_hex(value, 32, hex_val);

    uint8_t calldata[512];
    size_t calldata_len = 0;
    aegis_encode_solidity_calldata(root, key, value, &proof, calldata, sizeof(calldata), &calldata_len);

    char calldata_hex[1024];
    aegis_bytes_to_hex(calldata, calldata_len, calldata_hex);

    printf("{\n");
    printf("  \"depth\": 8,\n");
    printf("  \"root\": \"%s\",\n", hex_root);
    printf("  \"key\": \"%s\",\n", hex_key);
    printf("  \"value\": \"%s\",\n", hex_val);
    printf("  \"solidityCalldata\": \"%s\",\n", calldata_hex);
    printf("  \"siblings\": [\n");
    for (uint32_t i = 0; i < proof.depth; i++) {
        char sib_hex[67];
        aegis_bytes_to_hex(proof.siblings[i].data, 32, sib_hex);
        printf("    \"%s\"%s\n", sib_hex, (i == proof.depth - 1) ? "" : ",");
    }
    printf("  ]\n");
    printf("}\n");

    aegis_smt_free(tree);
}

static void run_eval_precompile(const char *hex_str) {
    if (!hex_str) {
        fprintf(stderr, "Error: missing hex calldata argument for --eval-precompile\n");
        exit(1);
    }
    const char *p = hex_str;
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        p += 2;
    }
    size_t hex_len = strlen(p);
    if (hex_len % 2 != 0 || hex_len < 256) {
        fprintf(stderr, "Error: calldata hex must be an even length and at least 256 hex chars (128 bytes)\n");
        exit(1);
    }
    size_t byte_len = hex_len / 2;
    uint8_t *calldata = (uint8_t *)malloc(byte_len);
    if (!calldata) {
        fprintf(stderr, "Error: memory allocation failed\n");
        exit(1);
    }
    aegis_status_t s = aegis_hex_to_bytes(p, calldata, byte_len);
    if (s != AEGIS_SUCCESS) {
        fprintf(stderr, "Error: invalid hex formatting in calldata\n");
        free(calldata);
        exit(1);
    }

    uint8_t out[32] = {0};
    size_t actual_out_len = 0;
    uint64_t gas_cost = 0;
    int res = aegis_precompile_eval(calldata, byte_len, out, 32, &actual_out_len, &gas_cost);

    char out_hex[67];
    aegis_bytes_to_hex(out, 32, out_hex);

    printf("=== Aegis EVM Precompile Evaluator ===\n");
    printf("Calldata Bytes: %zu\n", byte_len);
    printf("Precompile Eval: %s\n", (res == 0) ? "SUCCESS" : "ERROR");
    printf("Gas Consumed:   %llu gas\n", (unsigned long long)gas_cost);
    printf("Result Word:    %s\n", out_hex);
    printf("Verdict:        %s\n", (out[31] == 1) ? "VALID (1)" : "INVALID (0)");

    free(calldata);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 0;
    }

    if (strcmp(argv[1], "--demo") == 0) {
        run_demo();
    } else if (strcmp(argv[1], "--export-test-vector") == 0) {
        run_export_test_vector();
    } else if (strcmp(argv[1], "--eval-precompile") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: --eval-precompile requires <calldata_hex>\n");
            return 1;
        }
        run_eval_precompile(argv[2]);
    } else if (strcmp(argv[1], "--version") == 0) {
        printf("Aegis ZK Accumulator Engine v1.0.0 (C11, POSIX, Keccak-256, Poseidon-BN254)\n");
    } else {
        print_usage(argv[0]);
    }

    return 0;
}
