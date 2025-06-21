#ifndef _BLAKE2S_H_
#define _BLAKE2S_H_

#include <stdint.h>

// BLAKE2s constants
#define BLAKE2S_BLOCKBYTES 64
#define BLAKE2S_OUTBYTES 32
#define BLAKE2S_KEYBYTES 32
#define BLAKE2S_SALTBYTES 8
#define BLAKE2S_PERSONALBYTES 8

// BLAKE2s state structure
typedef struct {
    uint32_t h[8];      // hash state
    uint32_t t[2];      // total number of bytes
    uint32_t f[2];      // finalization flags
    uint8_t buf[BLAKE2S_BLOCKBYTES];  // input buffer
    uint32_t buflen;    // buffer length
    uint8_t outlen;     // output length
    uint8_t keylen;     // key length
} blake2s_state;

// BLAKE2s parameter block
typedef struct {
    uint8_t digest_length;                   // 0
    uint8_t key_length;                      // 1
    uint8_t fanout;                          // 2
    uint8_t depth;                           // 3
    uint32_t leaf_length;                    // 4
    uint32_t node_offset;                    // 8
    uint32_t xof_length;                     // 12
    uint8_t node_depth;                      // 16
    uint8_t inner_length;                    // 17
    uint8_t reserved[14];                    // 18
    uint8_t salt[BLAKE2S_SALTBYTES];         // 32
    uint8_t personal[BLAKE2S_PERSONALBYTES]; // 40
} blake2s_param;

extern "C" {
    void blake2s_hash(
        const uint8_t *input,
        uint32_t input_len,
        uint8_t *output,
        uint32_t output_len,
        const uint8_t *key,
        uint32_t key_len
    );
}

#endif