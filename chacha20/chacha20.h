#ifndef _CHACHA20_H_
#define _CHACHA20_H_

#include <stdint.h>
#include <cstring>

#define CHACHA20_BLOCK_SIZE 64  // 512-bit block (64 bytes)
#define CHACHA20_KEY_SIZE 32    // 256-bit key (32 bytes)
#define CHACHA20_NONCE_SIZE 12  // 96-bit nonce (12 bytes)
#define CHACHA20_ROUNDS 20      // 20 rounds for ChaCha20

// ChaCha20 constants
extern const uint32_t chacha20_constants[4];

extern "C" {
    void chacha20_encrypt(
        const uint8_t *plaintext, 
        const uint8_t *key, 
        const uint8_t *nonce,
        uint32_t counter,
        uint8_t *ciphertext, 
        int num_blocks
    );
}

#endif