#include "sha256.h"

// SHA-256 Constants
const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

// Initial hash values
const uint32_t H0[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

// Convert big-endian bytes to 32-bit word
static uint32_t bytes_to_word(const uint8_t *bytes) {
#pragma HLS INLINE
    return ((uint32_t)bytes[0] << 24) | 
           ((uint32_t)bytes[1] << 16) | 
           ((uint32_t)bytes[2] << 8) | 
           ((uint32_t)bytes[3]);
}

// Convert 32-bit word to big-endian bytes
static void word_to_bytes(uint32_t word, uint8_t *bytes) {
#pragma HLS INLINE
    bytes[0] = (word >> 24) & 0xFF;
    bytes[1] = (word >> 16) & 0xFF;
    bytes[2] = (word >> 8) & 0xFF;
    bytes[3] = word & 0xFF;
}

// Message schedule expansion - PIPELINED
static void message_schedule(const uint32_t M[16], uint32_t W[64]) {
#pragma HLS INLINE
    
    // Copy first 16 words
    COPY_LOOP: for (int t = 0; t < 16; t++) {
#pragma HLS UNROLL
        W[t] = M[t];
    }
    
    // Expand to 64 words with pipelining
    EXPAND_LOOP: for (int t = 16; t < 64; t++) {
#pragma HLS PIPELINE II=1
#pragma HLS DEPENDENCE variable=W inter false
        W[t] = sigma1(W[t-2]) + W[t-7] + sigma0(W[t-15]) + W[t-16];
    }
}

// SHA-256 compression function - PIPELINED
static void sha256_compress(uint32_t state[8], const uint32_t W[64]) {
#pragma HLS INLINE
    
    uint32_t a, b, c, d, e, f, g, h;
    uint32_t T1, T2;
    
    // Initialize working variables
    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];
    f = state[5];
    g = state[6];
    h = state[7];
    
    // Main compression loop - PIPELINED for maximum throughput
    COMPRESS_LOOP: for (int t = 0; t < 64; t++) {
#pragma HLS PIPELINE II=1
        T1 = h + SIGMA1(e) + CH(e, f, g) + K[t] + W[t];
        T2 = SIGMA0(a) + MAJ(a, b, c);
        
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;
    }
    
    // Update state
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

// Process single block - PIPELINED
static void process_block(uint32_t state[8], const uint8_t block[64]) {
#pragma HLS INLINE
    
    uint32_t M[16];
#pragma HLS ARRAY_PARTITION variable=M complete
    
    uint32_t W[64];
#pragma HLS ARRAY_PARTITION variable=W cyclic factor=8
    
    // Convert block to words
    PARSE_LOOP: for (int i = 0; i < 16; i++) {
#pragma HLS UNROLL
        M[i] = bytes_to_word(&block[i * 4]);
    }
    
    // Expand message schedule
    message_schedule(M, W);
    
    // Compress
    sha256_compress(state, W);
}

// Main SHA-256 hash function with HLS pragmas
void sha256_hash(const uint8_t *input, uint8_t *output, int num_blocks) {
#pragma HLS INTERFACE m_axi port=input depth=4096 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=output depth=32 offset=slave bundle=gmem1
#pragma HLS INTERFACE s_axilite port=input bundle=control
#pragma HLS INTERFACE s_axilite port=output bundle=control
#pragma HLS INTERFACE s_axilite port=num_blocks bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

    uint32_t state[8];
#pragma HLS ARRAY_PARTITION variable=state complete
    
    // Initialize state with initial hash values
    INIT_LOOP: for (int i = 0; i < 8; i++) {
#pragma HLS UNROLL
        state[i] = H0[i];
    }
    
    // Process each block with pipelining
    BLOCK_LOOP: for (int block = 0; block < num_blocks; block++) {
#pragma HLS PIPELINE II=64
        
        uint8_t block_data[64];
#pragma HLS ARRAY_PARTITION variable=block_data cyclic factor=8
        
        // Load block data
        LOAD_BLOCK: for (int i = 0; i < 64; i++) {
#pragma HLS PIPELINE II=1
            block_data[i] = input[block * 64 + i];
        }
        
        // Process the block
        process_block(state, block_data);
    }
    
    // Convert final state to output bytes
    OUTPUT_LOOP: for (int i = 0; i < 8; i++) {
#pragma HLS UNROLL
        word_to_bytes(state[i], &output[i * 4]);
    }
}