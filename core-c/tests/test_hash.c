/**
 * @file test_hash.c
 * @brief Unit tests for cryptographic hashing (Keccak-256 & Poseidon).
 */

#include "aegis_crypto.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_keccak256_empty(void) {
    uint8_t out[32];
    aegis_status_t status = aegis_keccak256((const uint8_t *)"", 0, out);
    assert(status == AEGIS_SUCCESS);

    char hex[67];
    aegis_bytes_to_hex(out, 32, hex);
    printf("[TEST] Keccak256(\"\") = %s\n", hex);
    assert(strcmp(hex, "0xc5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470") == 0);
}

static void test_keccak256_abc(void) {
    uint8_t out[32];
    const char *msg = "abc";
    aegis_status_t status = aegis_keccak256((const uint8_t *)msg, strlen(msg), out);
    assert(status == AEGIS_SUCCESS);

    char hex[67];
    aegis_bytes_to_hex(out, 32, hex);
    printf("[TEST] Keccak256(\"abc\") = %s\n", hex);
    assert(strcmp(hex, "0x4e03657aea45a94fc7d47ba826c8d667c0d1e6e33a64a036ec44f58fa12d6c45") == 0);
}

static void test_hash_nodes_equivalence(void) {
    uint8_t left[32];
    uint8_t right[32];
    for (int i = 0; i < 32; i++) {
        left[i] = (uint8_t)(i + 1);
        right[i] = (uint8_t)(i + 33);
    }

    uint8_t fast_out[32];
    aegis_status_t s1 = aegis_hash_nodes(left, right, fast_out);
    assert(s1 == AEGIS_SUCCESS);

    uint8_t combined[64];
    memcpy(combined, left, 32);
    memcpy(combined + 32, right, 32);

    uint8_t standard_out[32];
    aegis_status_t s2 = aegis_keccak256(combined, 64, standard_out);
    assert(s2 == AEGIS_SUCCESS);

    assert(memcmp(fast_out, standard_out, 32) == 0);
    printf("[TEST] aegis_hash_nodes matches standard 64-byte Keccak-256 exactly!\n");
}

static void test_hex_roundtrip(void) {
    const char *orig = "0xdeadbeefcafebabe0123456789abcdef0123456789abcdef0123456789abcdef";
    uint8_t bytes[32];
    aegis_status_t s1 = aegis_hex_to_bytes(orig, bytes, 32);
    assert(s1 == AEGIS_SUCCESS);

    char hex_out[67];
    aegis_status_t s2 = aegis_bytes_to_hex(bytes, 32, hex_out);
    assert(s2 == AEGIS_SUCCESS);

    assert(strcmp(orig, hex_out) == 0);
    printf("[TEST] Hex conversion roundtrip verified!\n");
}

int main(void) {
    printf("=== Aegis Crypto Hash Test Suite ===\n");
    test_keccak256_empty();
    test_keccak256_abc();
    test_hash_nodes_equivalence();
    test_hex_roundtrip();
    printf("=== ALL CRYPTO HASH TESTS PASSED ===\n");
    return 0;
}
