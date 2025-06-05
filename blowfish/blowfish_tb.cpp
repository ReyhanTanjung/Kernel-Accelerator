#include <iostream>
#include <iomanip>
#include <cstring>
#include "blowfish.h"

#define NUM_TEST_BLOCKS 4

int main() {
    // Test vectors for Blowfish (64-bit block, 128-bit key)
    uint8_t test_key[16] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
        0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87
    };
    int key_len = 16;

    uint8_t test_plaintext[NUM_TEST_BLOCKS * 8] = {
        // Block 1
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
        // Block 2
        0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
        // Block 3
        0x7c, 0xa1, 0x10, 0x45, 0x4a, 0x1a, 0x6e, 0x57,
        // Block 4
        0x01, 0x31, 0xd9, 0x61, 0x9d, 0xc1, 0x37, 0x6e
    };

    uint8_t ciphertext[NUM_TEST_BLOCKS * 8];

    std::cout << "=== Blowfish Encryption Test ===" << std::endl;

    // Print original plaintext
    std::cout << "\nOriginal Plaintext:" << std::endl;
    for (int block = 0; block < NUM_TEST_BLOCKS; block++) {
        std::cout << "Block " << block << ": ";
        for (int i = 0; i < 8; i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0')
                      << (int)test_plaintext[block * 8 + i] << " ";
        }
        std::cout << std::endl;
    }

    // Print key
    std::cout << "\nKey: ";
    for (int i = 0; i < key_len; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << (int)test_key[i] << " ";
    }
    std::cout << std::endl;

    // Perform encryption
    std::cout << "\n=== Encryption ===" << std::endl;
    blowfish_encrypt(test_plaintext, test_key, ciphertext, NUM_TEST_BLOCKS, key_len);

    // Print ciphertext
    std::cout << "Ciphertext:" << std::endl;
    for (int block = 0; block < NUM_TEST_BLOCKS; block++) {
        std::cout << "Block " << block << ": ";
        for (int i = 0; i < 8; i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0')
                      << (int)ciphertext[block * 8 + i] << " ";
        }
        std::cout << std::endl;
    }

    // Basic validation
    std::cout << "\n=== Basic Validation ===" << std::endl;
    bool different = false;
    for (int i = 0; i < NUM_TEST_BLOCKS * 8; i++) {
        if (test_plaintext[i] != ciphertext[i]) {
            different = true;
            break;
        }
    }

    if (different) {
        std::cout << "✓ Encryption completed! Ciphertext differs from plaintext." << std::endl;
        std::cout << "\n=== Performance Features ===" << std::endl;
        std::cout << "• F-function: Pipelined with S-box lookups" << std::endl;
        std::cout << "• Subkey Generation: Optimized with unrolled loops" << std::endl;
        std::cout << "• Block processing: Pipelined with II=1" << std::endl;
        std::cout << "• Memory interfaces: AXI4 with separate bundles" << std::endl;
        std::cout << "• Arrays: Partitioned for parallel access" << std::endl;
        return 0;
    } else {
        std::cout << "✗ Encryption failed! Ciphertext same as plaintext." << std::endl;
        return 1;
    }
}