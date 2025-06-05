#include "rsa.h"

// Montgomery multiplication for efficient modular arithmetic
static rsa_int_t montgomery_multiply(rsa_int_t a, rsa_int_t b, rsa_int_t n, rsa_int_t n_prime, int r_bits) {
#pragma HLS INLINE
    rsa_int_double_t t = (rsa_int_double_t)a * b;
    rsa_int_t t_low = t(RSA_BITS-1, 0);
    rsa_int_t m = t_low * n_prime;
    m = m(RSA_BITS-1, 0);  // Keep only lower bits
    
    rsa_int_double_t u = t + (rsa_int_double_t)m * n;
    rsa_int_t u_high = u(RSA_BITS*2-1, RSA_BITS);
    
    if (u_high >= n) {
        u_high -= n;
    }
    
    return u_high;
}

// Compute Montgomery parameter n' where n*n' ≡ -1 (mod 2^r)
static rsa_int_t compute_n_prime(rsa_int_t n) {
#pragma HLS INLINE off
    rsa_int_t n_prime = 1;
    
    // Use Newton's method to compute modular inverse
    for (int i = 0; i < 10; i++) {
#pragma HLS PIPELINE II=1
        n_prime = n_prime * (2 - n * n_prime);
    }
    
    return -n_prime;  // Return -n^(-1) mod 2^r
}

// Convert to Montgomery form: a_mont = a * 2^r mod n
static rsa_int_t to_montgomery(rsa_int_t a, rsa_int_t n) {
#pragma HLS INLINE off
    rsa_int_double_t temp = a;
    temp = temp << RSA_BITS;  // Multiply by 2^r
    
    // Modular reduction
    rsa_int_t result = 0;
    for (int i = RSA_BITS*2-1; i >= RSA_BITS; i--) {
#pragma HLS PIPELINE II=1
        result = result << 1;
        if (temp[i]) {
            result = result | 1;
        }
        if (result >= n) {
            result -= n;
        }
    }
    
    return result;
}

// Convert from Montgomery form: a = a_mont * 2^(-r) mod n
static rsa_int_t from_montgomery(rsa_int_t a_mont, rsa_int_t n, rsa_int_t n_prime) {
#pragma HLS INLINE
    return montgomery_multiply(a_mont, 1, n, n_prime, RSA_BITS);
}

// Binary method for modular exponentiation using Montgomery multiplication
static rsa_int_t mod_exp_montgomery(rsa_int_t base, rsa_int_t exp, rsa_int_t n) {
#pragma HLS INLINE off
    
    // Precompute Montgomery parameters
    rsa_int_t n_prime = compute_n_prime(n);
    
    // Convert base to Montgomery form
    rsa_int_t base_mont = to_montgomery(base, n);
    
    // Initialize result in Montgomery form (1 * 2^r mod n)
    rsa_int_t result_mont = to_montgomery(1, n);
    
    // Square-and-multiply algorithm with Montgomery arithmetic
    MOD_EXP_LOOP: for (int i = RSA_BITS-1; i >= 0; i--) {
#pragma HLS PIPELINE II=2
        // Always square
        result_mont = montgomery_multiply(result_mont, result_mont, n, n_prime, RSA_BITS);
        
        // Multiply by base if bit is set
        if (exp[i]) {
            result_mont = montgomery_multiply(result_mont, base_mont, n, n_prime, RSA_BITS);
        }
    }
    
    // Convert back from Montgomery form
    return from_montgomery(result_mont, n, n_prime);
}

// Convert byte array to arbitrary precision integer
static rsa_int_t bytes_to_rsa_int(const uint8_t *bytes) {
#pragma HLS INLINE
    rsa_int_t result = 0;
    
    BYTES_TO_INT: for (int i = 0; i < RSA_BYTES; i++) {
#pragma HLS UNROLL factor=8
        result = (result << 8) | bytes[i];
    }
    
    return result;
}

// Convert arbitrary precision integer to byte array
static void rsa_int_to_bytes(rsa_int_t val, uint8_t *bytes) {
#pragma HLS INLINE
    
    INT_TO_BYTES: for (int i = RSA_BYTES-1; i >= 0; i--) {
#pragma HLS UNROLL factor=8
        bytes[i] = val & 0xFF;
        val = val >> 8;
    }
}

// Main RSA encryption function with HLS pragmas
void rsa_encrypt(const uint8_t *plaintext, 
                 const uint8_t *n_bytes,
                 const uint8_t *e_bytes, 
                 uint8_t *ciphertext, 
                 int num_blocks) {
#pragma HLS INTERFACE m_axi port=plaintext depth=1024 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=n_bytes depth=256 offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi port=e_bytes depth=256 offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi port=ciphertext depth=1024 offset=slave bundle=gmem3
#pragma HLS INTERFACE s_axilite port=plaintext bundle=control
#pragma HLS INTERFACE s_axilite port=n_bytes bundle=control
#pragma HLS INTERFACE s_axilite port=e_bytes bundle=control
#pragma HLS INTERFACE s_axilite port=ciphertext bundle=control
#pragma HLS INTERFACE s_axilite port=num_blocks bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

    // Local buffers for key components
    uint8_t n_local[RSA_BYTES];
    uint8_t e_local[RSA_BYTES];
#pragma HLS ARRAY_PARTITION variable=n_local cyclic factor=16
#pragma HLS ARRAY_PARTITION variable=e_local cyclic factor=16
    
    // Load modulus and exponent
    LOAD_N: for (int i = 0; i < RSA_BYTES; i++) {
#pragma HLS PIPELINE II=1
        n_local[i] = n_bytes[i];
    }
    
    LOAD_E: for (int i = 0; i < RSA_BYTES; i++) {
#pragma HLS PIPELINE II=1
        e_local[i] = e_bytes[i];
    }
    
    // Convert to arbitrary precision integers
    rsa_int_t n = bytes_to_rsa_int(n_local);
    rsa_int_t e = bytes_to_rsa_int(e_local);
    
    // Process each block
    BLOCK_LOOP: for (int block = 0; block < num_blocks; block++) {
#pragma HLS PIPELINE off
        
        uint8_t plain_block[RSA_BYTES];
        uint8_t cipher_block[RSA_BYTES];
#pragma HLS ARRAY_PARTITION variable=plain_block cyclic factor=16
#pragma HLS ARRAY_PARTITION variable=cipher_block cyclic factor=16
        
        // Load plaintext block
        LOAD_PLAIN: for (int i = 0; i < RSA_BYTES; i++) {
#pragma HLS PIPELINE II=1
            plain_block[i] = plaintext[block * RSA_BYTES + i];
        }
        
        // Convert plaintext to integer
        rsa_int_t m = bytes_to_rsa_int(plain_block);
        
        // Perform RSA encryption: c = m^e mod n
        rsa_int_t c = mod_exp_montgomery(m, e, n);
        
        // Convert result back to bytes
        rsa_int_to_bytes(c, cipher_block);
        
        // Store ciphertext block
        STORE_CIPHER: for (int i = 0; i < RSA_BYTES; i++) {
#pragma HLS PIPELINE II=1
            ciphertext[block * RSA_BYTES + i] = cipher_block[i];
        }
    }
}