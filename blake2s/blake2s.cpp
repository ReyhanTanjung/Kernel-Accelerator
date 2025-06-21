#include "blake2s.h"

// BLAKE2s IV constants
static const uint32_t blake2s_iv[8] = {
    0x6A09E667UL, 0xBB67AE85UL, 0x3C6EF372UL, 0xA54FF53AUL,
    0x510E527FUL, 0x9B05688CUL, 0x1F83D9ABUL, 0x5BE0CD19UL
};

// BLAKE2s sigma permutation table
static const uint8_t blake2s_sigma[10][16] = {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
    { 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 },
    { 11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4 },
    { 7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8 },
    { 9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13 },
    { 2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9 },
    { 12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11 },
    { 13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10 },
    { 6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5 },
    { 10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0 }
};

// Utility functions
inline uint32_t rotr32(uint32_t w, unsigned c) {
#pragma HLS INLINE
    return (w >> c) | (w << (32 - c));
}

// BLAKE2s G function
inline void blake2s_g(uint32_t v[16], int a, int b, int c, int d, uint32_t x, uint32_t y) {
#pragma HLS INLINE
    v[a] = v[a] + v[b] + x;
    v[d] = rotr32(v[d] ^ v[a], 16);
    v[c] = v[c] + v[d];
    v[b] = rotr32(v[b] ^ v[c], 12);
    v[a] = v[a] + v[b] + y;
    v[d] = rotr32(v[d] ^ v[a], 8);
    v[c] = v[c] + v[d];
    v[b] = rotr32(v[b] ^ v[c], 7);
}

// BLAKE2s compression function
void blake2s_compress(blake2s_state *state, const uint8_t block[BLAKE2S_BLOCKBYTES]) {
#pragma HLS INLINE off
    
    uint32_t m[16];
    uint32_t v[16];
    
    // Convert block to 32-bit words (little-endian)
    for (int i = 0; i < 16; i++) {
#pragma HLS UNROLL
        m[i] = ((uint32_t)block[4*i + 0]) |
               ((uint32_t)block[4*i + 1] << 8) |
               ((uint32_t)block[4*i + 2] << 16) |
               ((uint32_t)block[4*i + 3] << 24);
    }
    
    // Initialize working variables
    for (int i = 0; i < 8; i++) {
#pragma HLS UNROLL
        v[i] = state->h[i];
        v[i + 8] = blake2s_iv[i];
    }
    
    v[12] ^= state->t[0];
    v[13] ^= state->t[1];
    v[14] ^= state->f[0];
    v[15] ^= state->f[1];
    
    // 10 rounds of mixing
    for (int round = 0; round < 10; round++) {
#pragma HLS PIPELINE
        const uint8_t *s = blake2s_sigma[round];
        
        blake2s_g(v, 0, 4, 8, 12, m[s[0]], m[s[1]]);
        blake2s_g(v, 1, 5, 9, 13, m[s[2]], m[s[3]]);
        blake2s_g(v, 2, 6, 10, 14, m[s[4]], m[s[5]]);
        blake2s_g(v, 3, 7, 11, 15, m[s[6]], m[s[7]]);
        blake2s_g(v, 0, 5, 10, 15, m[s[8]], m[s[9]]);
        blake2s_g(v, 1, 6, 11, 12, m[s[10]], m[s[11]]);
        blake2s_g(v, 2, 7, 8, 13, m[s[12]], m[s[13]]);
        blake2s_g(v, 3, 4, 9, 14, m[s[14]], m[s[15]]);
    }
    
    // Finalize
    for (int i = 0; i < 8; i++) {
#pragma HLS UNROLL
        state->h[i] ^= v[i] ^ v[i + 8];
    }
}

// Initialize BLAKE2s state
void blake2s_init(blake2s_state *state, uint8_t outlen, const uint8_t *key, uint8_t keylen) {
#pragma HLS INLINE
    
    blake2s_param param;
    
    // Initialize parameter block
    param.digest_length = outlen;
    param.key_length = keylen;
    param.fanout = 1;
    param.depth = 1;
    param.leaf_length = 0;
    param.node_offset = 0;
    param.xof_length = 0;
    param.node_depth = 0;
    param.inner_length = 0;
    
    // Clear reserved area
    for (int i = 0; i < 14; i++) {
#pragma HLS UNROLL
        param.reserved[i] = 0;
    }
    
    // Clear salt and personal
    for (int i = 0; i < BLAKE2S_SALTBYTES; i++) {
#pragma HLS UNROLL
        param.salt[i] = 0;
    }
    for (int i = 0; i < BLAKE2S_PERSONALBYTES; i++) {
#pragma HLS UNROLL
        param.personal[i] = 0;
    }
    
    // Initialize state
    for (int i = 0; i < 8; i++) {
#pragma HLS UNROLL
        state->h[i] = blake2s_iv[i];
    }
    
    // XOR with parameter block
    uint32_t *p = (uint32_t *)&param;
    for (int i = 0; i < 8; i++) {
#pragma HLS UNROLL
        state->h[i] ^= p[i];
    }
    
    state->t[0] = 0;
    state->t[1] = 0;
    state->f[0] = 0;
    state->f[1] = 0;
    state->buflen = 0;
    state->outlen = outlen;
    state->keylen = keylen;
    
    // Clear buffer
    for (int i = 0; i < BLAKE2S_BLOCKBYTES; i++) {
#pragma HLS UNROLL
        state->buf[i] = 0;
    }
    
    // Handle key
    if (keylen > 0) {
        for (int i = 0; i < keylen; i++) {
#pragma HLS UNROLL
            state->buf[i] = key[i];
        }
        state->buflen = BLAKE2S_BLOCKBYTES;
    }
}

// Update BLAKE2s state with input data
void blake2s_update(blake2s_state *state, const uint8_t *input, uint32_t inlen) {
    
    uint32_t left = state->buflen;
    uint32_t fill = BLAKE2S_BLOCKBYTES - left;
    
    if (inlen > fill) {
        // Fill current block
        for (uint32_t i = 0; i < fill; i++) {
#pragma HLS PIPELINE
            state->buf[left + i] = input[i];
        }
        
        state->buflen = 0;
        state->t[0] += BLAKE2S_BLOCKBYTES;
        if (state->t[0] < BLAKE2S_BLOCKBYTES) {
            state->t[1]++;
        }
        
        blake2s_compress(state, state->buf);
        
        input += fill;
        inlen -= fill;
        
        // Process full blocks
        while (inlen > BLAKE2S_BLOCKBYTES) {
#pragma HLS PIPELINE
            state->t[0] += BLAKE2S_BLOCKBYTES;
            if (state->t[0] < BLAKE2S_BLOCKBYTES) {
                state->t[1]++;
            }
            blake2s_compress(state, input);
            input += BLAKE2S_BLOCKBYTES;
            inlen -= BLAKE2S_BLOCKBYTES;
        }
    }
    
    // Store remaining bytes
    for (uint32_t i = 0; i < inlen; i++) {
#pragma HLS PIPELINE
        state->buf[state->buflen + i] = input[i];
    }
    state->buflen += inlen;
}

// Finalize BLAKE2s and produce output
void blake2s_final(blake2s_state *state, uint8_t *output) {
    
    // Pad last block with zeros
    for (uint32_t i = state->buflen; i < BLAKE2S_BLOCKBYTES; i++) {
#pragma HLS PIPELINE
        state->buf[i] = 0;
    }
    
    state->t[0] += state->buflen;
    if (state->t[0] < state->buflen) {
        state->t[1]++;
    }
    
    state->f[0] = 0xFFFFFFFF;
    blake2s_compress(state, state->buf);
    
    // Convert hash to bytes (little-endian)
    for (int i = 0; i < state->outlen; i++) {
#pragma HLS PIPELINE
        output[i] = (state->h[i / 4] >> (8 * (i % 4))) & 0xFF;
    }
}

// Main BLAKE2s hash function
void blake2s_hash(
    const uint8_t *input,
    uint32_t input_len,
    uint8_t *output,
    uint32_t output_len,
    const uint8_t *key,
    uint32_t key_len
) {
#pragma HLS INTERFACE m_axi port=input depth=1024 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=output depth=32 offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi port=key depth=32 offset=slave bundle=gmem2
#pragma HLS INTERFACE s_axilite port=input bundle=control
#pragma HLS INTERFACE s_axilite port=input_len bundle=control
#pragma HLS INTERFACE s_axilite port=output bundle=control
#pragma HLS INTERFACE s_axilite port=output_len bundle=control
#pragma HLS INTERFACE s_axilite port=key bundle=control
#pragma HLS INTERFACE s_axilite port=key_len bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

    blake2s_state state;
    
    // Clamp output length
    if (output_len > BLAKE2S_OUTBYTES) {
        output_len = BLAKE2S_OUTBYTES;
    }
    
    // Clamp key length
    if (key_len > BLAKE2S_KEYBYTES) {
        key_len = BLAKE2S_KEYBYTES;
    }
    
    // Initialize
    blake2s_init(&state, output_len, key, key_len);
    
    // Update with input data
    blake2s_update(&state, input, input_len);
    
    // Finalize
    blake2s_final(&state, output);
}