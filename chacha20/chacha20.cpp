#include "chacha20.h"

// ChaCha20 constants: "expand 32-byte k"
const uint32_t chacha20_constants[4] = {
    0x61707865, 0x3320646e, 0x79622d32, 0x6b206574
};

// Utility function: 32-bit left rotation
static inline uint32_t rotl32(uint32_t x, int n) {
#pragma HLS INLINE
    return (x << n) | (x >> (32 - n));
}

// Little-endian conversion functions
static inline uint32_t le32_to_cpu(const uint8_t *p) {
#pragma HLS INLINE
    return ((uint32_t)p[0]) | 
           ((uint32_t)p[1] << 8) | 
           ((uint32_t)p[2] << 16) | 
           ((uint32_t)p[3] << 24);
}

static inline void cpu_to_le32(uint8_t *p, uint32_t v) {
#pragma HLS INLINE
    p[0] = v & 0xff;
    p[1] = (v >> 8) & 0xff;
    p[2] = (v >> 16) & 0xff;
    p[3] = (v >> 24) & 0xff;
}

// ChaCha20 quarter round operation - PIPELINED
static void quarter_round(uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d) {
#pragma HLS INLINE
    *a += *b; *d ^= *a; *d = rotl32(*d, 16);
    *c += *d; *b ^= *c; *b = rotl32(*b, 12);
    *a += *b; *d ^= *a; *d = rotl32(*d, 8);
    *c += *d; *b ^= *c; *b = rotl32(*b, 7);
}

// ChaCha20 block function - OPTIMIZED FOR FPGA
static void chacha20_block(uint32_t state[16], uint32_t output[16]) {
#pragma HLS INLINE
    
    // Copy initial state to working state
    uint32_t x[16];
    
    for (int i = 0; i < 16; i++) {
#pragma HLS UNROLL
        x[i] = state[i];
    }
    
    // 20 rounds (10 double rounds) with pipelining
    for (int i = 0; i < 10; i++) {
#pragma HLS PIPELINE II=1
        
        // Odd rounds - column rounds
        quarter_round(&x[0], &x[4], &x[8],  &x[12]);
        quarter_round(&x[1], &x[5], &x[9],  &x[13]);
        quarter_round(&x[2], &x[6], &x[10], &x[14]);
        quarter_round(&x[3], &x[7], &x[11], &x[15]);
        
        // Even rounds - diagonal rounds
        quarter_round(&x[0], &x[5], &x[10], &x[15]);
        quarter_round(&x[1], &x[6], &x[11], &x[12]);
        quarter_round(&x[2], &x[7], &x[8],  &x[13]);
        quarter_round(&x[3], &x[4], &x[9],  &x[14]);
    }
    
    // Add initial state to final state
    for (int i = 0; i < 16; i++) {
#pragma HLS UNROLL
        output[i] = x[i] + state[i];
    }
}

// Initialize ChaCha20 state
static void chacha20_init_state(uint32_t state[16], const uint8_t *key, 
                               const uint8_t *nonce, uint32_t counter) {
#pragma HLS INLINE
    
    // Constants
    state[0] = chacha20_constants[0];
    state[1] = chacha20_constants[1];
    state[2] = chacha20_constants[2];
    state[3] = chacha20_constants[3];
    
    // Key (32 bytes = 8 words)
    for (int i = 0; i < 8; i++) {
#pragma HLS UNROLL
        state[4 + i] = le32_to_cpu(key + i * 4);
    }
    
    // Counter (32-bit)
    state[12] = counter;
    
    // Nonce (96-bit = 3 words)
    for (int i = 0; i < 3; i++) {
#pragma HLS UNROLL
        state[13 + i] = le32_to_cpu(nonce + i * 4);
    }
}

// ChaCha20 Encryption function with HLS pragmas for FPGA optimization
void chacha20_encrypt(const uint8_t *plaintext, const uint8_t *key, 
                     const uint8_t *nonce, uint32_t counter,
                     uint8_t *ciphertext, int num_blocks) {
#pragma HLS INTERFACE m_axi port=plaintext depth=128 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=key depth=32 offset=slave bundle=gmem1  
#pragma HLS INTERFACE m_axi port=nonce depth=12 offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi port=ciphertext depth=128 offset=slave bundle=gmem3
#pragma HLS INTERFACE s_axilite port=plaintext bundle=control
#pragma HLS INTERFACE s_axilite port=key bundle=control
#pragma HLS INTERFACE s_axilite port=nonce bundle=control
#pragma HLS INTERFACE s_axilite port=counter bundle=control
#pragma HLS INTERFACE s_axilite port=ciphertext bundle=control
#pragma HLS INTERFACE s_axilite port=num_blocks bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

    uint32_t state[16];
    uint32_t keystream[16];
    
    // Process each block
    for (int block = 0; block < num_blocks; block++) {
        
        // Initialize state for this block
        chacha20_init_state(state, key, nonce, counter + block);
        
        // Generate keystream block
        chacha20_block(state, keystream);
        
        // Convert keystream to bytes and XOR with plaintext
        uint8_t keystream_bytes[64];
        
        // Convert 32-bit words to bytes
        for (int i = 0; i < 16; i++) {
            cpu_to_le32(&keystream_bytes[i * 4], keystream[i]);
        }
        
        // XOR plaintext with keystream to produce ciphertext
        for (int i = 0; i < CHACHA20_BLOCK_SIZE; i++) {
            ciphertext[block * CHACHA20_BLOCK_SIZE + i] = 
                plaintext[block * CHACHA20_BLOCK_SIZE + i] ^ keystream_bytes[i];
        }
    }
}