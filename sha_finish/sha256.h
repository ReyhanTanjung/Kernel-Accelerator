#ifndef _SHA256_H_
#define _SHA256_H_

#include <stdint.h>

#define SHA256_BLOCK_SIZE 64  // 512 bits
#define SHA256_DIGEST_SIZE 32 // 256 bits
#define SHA256_ROUNDS 64

// SHA-256 Constants (first 32 bits of fractional parts of cube roots of first 64 primes)
extern const uint32_t K[64];

// Initial hash values (first 32 bits of fractional parts of square roots of first 8 primes)
extern const uint32_t H0[8];

// Right rotate function
#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

// SHA-256 functions
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define SIGMA0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define SIGMA1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define sigma0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define sigma1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

extern "C" {
    void sha256_hash(const uint8_t *input, uint8_t *output, int num_blocks);
}

#endif