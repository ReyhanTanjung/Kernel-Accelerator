#ifndef _BLOWFISH_H_
#define _BLOWFISH_H_

#include <stdint.h>

#define BLOCK_SIZE 8  // 64-bit block
#define NUM_ROUNDS 16 // Blowfish has 16 rounds
#define NUM_SBOXES 4  // Four S-boxes
#define SBOX_SIZE 256 // Each S-box has 256 entries
#define P_ARRAY_SIZE 18 // 18 P-arrays for subkeys

// S-box and P-array declarations
extern const uint32_t sbox[NUM_SBOXES][SBOX_SIZE];
extern const uint32_t p_array[P_ARRAY_SIZE];

extern "C" {
    void blowfish_encrypt(const uint8_t *plaintext, const uint8_t *key, uint8_t *ciphertext, int num_blocks, int key_len);
}

#endif