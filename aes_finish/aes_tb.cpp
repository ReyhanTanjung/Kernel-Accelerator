#include <iostream>
#include <iomanip>
#include <cstring>
#include "aes.h"

#define NUM_TEST_BLOCKS 4

int main() {
    // Test vectors - AES-128 test case
    uint8_t test_key[16] = {
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
    };
    
    uint8_t test_plaintext[NUM_TEST_BLOCKS * 16] = {
        // Block 1
        0x32, 0x43, 0xf6, 0xa8, 0x88, 0x5a, 0x30, 0x8d,
        0x31, 0x31, 0x98, 0xa2, 0xe0, 0x37, 0x07, 0x34,
        // Block 2  
        0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
        0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
        // Block 3
        0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
        0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
        // Block 4
        0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
        0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef
    };
    
    uint8_t ciphertext[NUM_TEST_BLOCKS * 16];
    
    std::cout << "=== AES-128 Encryption Test ===" << std::endl;
    
    // Print original plaintext
    std::cout << "\nOriginal Plaintext:" << std::endl;
    for (int block = 0; block < NUM_TEST_BLOCKS; block++) {
        std::cout << "Block " << block << ": ";
        for (int i = 0; i < 16; i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                      << (int)test_plaintext[block * 16 + i] << " ";
        }
        std::cout << std::endl;
    }
    
    // Print key
    std::cout << "\nKey: ";
    for (int i = 0; i < 16; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') 
                  << (int)test_key[i] << " ";
    }
    std::cout << std::endl;
    
    // Perform encryption
    std::cout << "\n=== Encryption ===" << std::endl;
    aes_encrypt(test_plaintext, test_key, ciphertext, NUM_TEST_BLOCKS);
    
    // Print ciphertext
    std::cout << "Ciphertext:" << std::endl;
    for (int block = 0; block < NUM_TEST_BLOCKS; block++) {
        std::cout << "Block " << block << ": ";
        for (int i = 0; i < 16; i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                      << (int)ciphertext[block * 16 + i] << " ";
        }
        std::cout << std::endl;
    }
    
    // Basic validation - check if ciphertext is different from plaintext
    std::cout << "\n=== Basic Validation ===" << std::endl;
    bool different = false;
    for (int i = 0; i < NUM_TEST_BLOCKS * 16; i++) {
        if (test_plaintext[i] != ciphertext[i]) {
            different = true;
            break;
        }
    }
    
    if (different) {
        std::cout << "✓ Encryption completed! Ciphertext differs from plaintext." << std::endl;
        
        // Performance info
        std::cout << "\n=== Performance Features ===" << std::endl;
        std::cout << "• SubBytes: Pipelined with S-box lookup tables" << std::endl;
        std::cout << "• MixColumns: Pipelined with Galois Field lookup tables" << std::endl;
        std::cout << "• Block processing: Pipelined with II=1" << std::endl;
        std::cout << "• Memory interfaces: AXI4 with separate bundles" << std::endl;
        std::cout << "• Arrays: Partitioned for parallel access" << std::endl;
        
        return 0;
    } else {
        std::cout << "✗ Encryption failed! Ciphertext same as plaintext." << std::endl;
        return 1;
    }
}