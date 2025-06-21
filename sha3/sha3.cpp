#include "sha3.h"

// Simplified Keccak round constants (first 8 rounds for faster synthesis)
static const uint64_t keccak_round_constants[8] = {
    0x0000000000000001ULL, 0x0000000000008082ULL, 
    0x800000000000808AULL, 0x8000000080008000ULL,
    0x000000000000808BULL, 0x0000000080000001ULL, 
    0x8000000080008081ULL, 0x8000000000008009ULL
};

// Simplified rotation offsets
static const int rho_offsets[25] = {
    0,  1,  3,  6,  10, 15, 21, 28, 36, 45, 55, 2,  14,
    27, 41, 56, 8,  25, 43, 62, 18, 39, 61, 20, 44
};

// Rotate left function
uint64_t rotl64(uint64_t x, int n) {
#pragma HLS INLINE
    if (n == 0) return x;
    return (x << n) | (x >> (64 - n));
}

// Simplified Keccak-f permutation (8 rounds instead of 24 for faster synthesis)
void keccak_f_simple(keccak_state_t state) {
#pragma HLS INLINE off
    
    uint64_t C[5], D[5];
    
    // Reduced rounds for faster synthesis and testing
    ROUNDS_LOOP: for (int round = 0; round < 8; round++) {
#pragma HLS PIPELINE off
        
        // Theta step
        THETA_C_LOOP: for (int x = 0; x < 5; x++) {
#pragma HLS UNROLL
            C[x] = state[x] ^ state[x + 5] ^ state[x + 10] ^ state[x + 15] ^ state[x + 20];
        }
        
        THETA_D_LOOP: for (int x = 0; x < 5; x++) {
#pragma HLS UNROLL
            D[x] = C[(x + 4) % 5] ^ rotl64(C[(x + 1) % 5], 1);
        }
        
        THETA_APPLY_LOOP: for (int y = 0; y < 5; y++) {
#pragma HLS UNROLL
            for (int x = 0; x < 5; x++) {
#pragma HLS UNROLL
                state[y * 5 + x] ^= D[x];
            }
        }
        
        // Simplified Rho and Pi steps
        uint64_t temp = state[1];
        RHO_PI_LOOP: for (int i = 0; i < 24; i++) {
#pragma HLS UNROLL factor=4
            int j = ((i + 1) * (i + 2) / 2) % 25;
            uint64_t temp2 = state[j];
            state[j] = rotl64(temp, rho_offsets[i]);
            temp = temp2;
        }
        
        // Chi step
        CHI_LOOP: for (int y = 0; y < 5; y++) {
#pragma HLS UNROLL
            uint64_t row[5];
            for (int x = 0; x < 5; x++) {
#pragma HLS UNROLL
                row[x] = state[y * 5 + x];
            }
            for (int x = 0; x < 5; x++) {
#pragma HLS UNROLL
                state[y * 5 + x] = row[x] ^ ((~row[(x + 1) % 5]) & row[(x + 2) % 5]);
            }
        }
        
        // Iota step
        state[0] ^= keccak_round_constants[round];
    }
}

// Absorb one block into the state
void absorb_block(keccak_state_t state, const uint8_t *block) {
#pragma HLS INLINE off
    
    // Convert bytes to 64-bit words and XOR with state
    ABSORB_LOOP: for (int i = 0; i < 17; i++) { // 136/8 = 17 words
#pragma HLS PIPELINE II=1
        uint64_t word = 0;
        
        // Little-endian byte order
        BYTE_LOOP: for (int j = 0; j < 8; j++) {
#pragma HLS UNROLL
            word |= ((uint64_t)block[i * 8 + j]) << (j * 8);
        }
        
        state[i] ^= word;
    }
    
    // Apply simplified Keccak-f permutation
    keccak_f_simple(state);
}

// Main SHA-3 256 function (simplified version)
void sha3_256(
    const uint8_t *message,
    uint32_t message_len,
    uint8_t *hash,
    uint32_t num_blocks
) {
#pragma HLS INTERFACE m_axi port=message depth=1024 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=hash depth=32 offset=slave bundle=gmem1
#pragma HLS INTERFACE s_axilite port=message bundle=control
#pragma HLS INTERFACE s_axilite port=message_len bundle=control
#pragma HLS INTERFACE s_axilite port=hash bundle=control
#pragma HLS INTERFACE s_axilite port=num_blocks bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

    // Initialize state to all zeros
    keccak_state_t state;
    INIT_LOOP: for (int i = 0; i < KECCAK_STATE_SIZE; i++) {
#pragma HLS UNROLL
        state[i] = 0;
    }
    
    // Process message in blocks
    uint32_t processed = 0;
    uint8_t block[SHA3_256_RATE];
    
    BLOCK_LOOP: for (uint32_t block_idx = 0; block_idx < num_blocks; block_idx++) {
#pragma HLS PIPELINE off
        
        // Copy block from input
        COPY_BLOCK_LOOP: for (int i = 0; i < SHA3_256_RATE; i++) {
#pragma HLS PIPELINE II=1
            if (processed + i < message_len) {
                block[i] = message[processed + i];
            } else {
                block[i] = 0;
            }
        }
        
        // Add simple padding for the last block
        if (block_idx == num_blocks - 1) {
            // Simplified padding: just add 0x80 at message end
            if (message_len % SHA3_256_RATE < SHA3_256_RATE) {
                block[message_len % SHA3_256_RATE] = 0x80;
            }
        }
        
        // Absorb the block
        absorb_block(state, block);
        processed += SHA3_256_RATE;
    }
    
    // Extract hash (squeeze phase) - simplified
    EXTRACT_LOOP: for (int i = 0; i < SHA3_256_HASH_SIZE; i++) {
#pragma HLS PIPELINE II=1
        int word_idx = i / 8;
        int byte_idx = i % 8;
        if (word_idx < KECCAK_STATE_SIZE) {
            hash[i] = (uint8_t)(state[word_idx] >> (byte_idx * 8));
        } else {
            hash[i] = 0;
        }
    }
}