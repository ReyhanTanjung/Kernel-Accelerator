#ifndef _SHA3_H_
#define _SHA3_H_

#include <stdint.h>

#define KECCAK_ROUNDS 24
#define KECCAK_STATE_SIZE 25
#define SHA3_256_RATE 136  // 1088 bits / 8 = 136 bytes
#define SHA3_256_CAPACITY 64  // 512 bits / 8 = 64 bytes
#define SHA3_256_HASH_SIZE 32  // 256 bits / 8 = 32 bytes
#define MAX_MESSAGE_SIZE 1024  // Maximum input message size in bytes

typedef uint64_t keccak_state_t[KECCAK_STATE_SIZE];

extern "C" {
    void sha3_256(
        const uint8_t *message,    // Input message
        uint32_t message_len,      // Message length in bytes
        uint8_t *hash,             // Output hash (32 bytes)
        uint32_t num_blocks        // Number of message blocks to process
    );
}

#endif // _SHA3_H_