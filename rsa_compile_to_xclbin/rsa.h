#ifndef _RSA_H_
#define _RSA_H_

#define AP_INT_MAX_W 4096
#include <ap_int.h>
#include <stdint.h>

// RSA bit widths - using 2048-bit RSA for security
#define RSA_BITS 2048
#define RSA_BYTES (RSA_BITS/8)

// Use arbitrary precision integers for large numbers
typedef ap_uint<RSA_BITS> rsa_int_t;
typedef ap_uint<RSA_BITS*2> rsa_int_double_t;

// Structure to hold RSA public key
struct RSAPublicKey {
    rsa_int_t n;  // modulus
    rsa_int_t e;  // public exponent
};

// RSA encryption function
extern "C" {
    void rsa_encrypt(const uint8_t *plaintext, 
                     const uint8_t *n_bytes,
                     const uint8_t *e_bytes, 
                     uint8_t *ciphertext, 
                     int num_blocks);
}

#endif