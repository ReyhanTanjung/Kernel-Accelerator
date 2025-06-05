#ifndef _AES_H_
#define _AES_H_

#include <stdint.h>

#define BLOCK_SIZE 16  // 128-bit block
#define NUM_ROUNDS 10  // AES-128 has 10 rounds

// AES S-box lookup table
extern const uint8_t sbox[256];

// Galois Field multiplication lookup tables for MixColumns
extern const uint8_t mul2[256];
extern const uint8_t mul3[256];

extern "C" {
    void aes_encrypt(const uint8_t *plaintext, const uint8_t *key, uint8_t *ciphertext, int num_blocks);
}

#endif