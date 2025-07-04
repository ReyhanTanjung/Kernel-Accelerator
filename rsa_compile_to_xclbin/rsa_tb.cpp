#include <iostream>
#include <iomanip>
#include <cstring>
#include "rsa.h"

#define NUM_TEST_BLOCKS 2

// Helper function to print hex values
void print_hex(const char* label, const uint8_t* data, int len) {
    std::cout << label << ": ";
    for (int i = 0; i < len; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
        if ((i + 1) % 32 == 0) std::cout << std::endl << "    ";
    }
    std::cout << std::dec << std::endl;
}

int main() {
    std::cout << "=== RSA-2048 Encryption Test ===" << std::endl;
    
    // Test RSA-2048 public key (65537 is common public exponent)
    // This is a test key - DO NOT use in production!
    uint8_t test_n[RSA_BYTES] = {
        0xC5, 0x3F, 0xB3, 0x9A, 0x3D, 0x42, 0x87, 0x67, 0x23, 0x78, 0x6D, 0x87, 0xF6, 0x4B, 0x7C, 0x41,
        0x8B, 0x65, 0x29, 0x8C, 0x4A, 0x92, 0x18, 0x81, 0x25, 0x5D, 0x3F, 0x6C, 0x7D, 0x8E, 0x69, 0x5F,
        0x3E, 0x9D, 0x8C, 0xAF, 0x18, 0x5A, 0x90, 0x83, 0x77, 0xBC, 0x54, 0x98, 0x5B, 0x38, 0x5E, 0x8D,
        0x62, 0x4C, 0x2E, 0x77, 0x84, 0x97, 0x15, 0xFC, 0x25, 0x7A, 0x4B, 0x2A, 0x90, 0x38, 0x6E, 0x85,
        0xA5, 0xD8, 0x7C, 0x5F, 0x51, 0x2B, 0x66, 0xCD, 0x91, 0x59, 0xE7, 0x10, 0xE5, 0x3B, 0x1A, 0x86,
        0x28, 0x07, 0x81, 0x0F, 0x97, 0xFB, 0x7E, 0x55, 0x43, 0x38, 0x5C, 0x7F, 0x9B, 0x8C, 0x5E, 0x34,
        0x53, 0x8D, 0x09, 0x24, 0x95, 0xFA, 0x39, 0x0A, 0x11, 0xB8, 0x66, 0xF8, 0x45, 0x92, 0x4D, 0xB8,
        0x83, 0x73, 0xAE, 0x8A, 0x38, 0x9A, 0x30, 0x50, 0x78, 0xD9, 0xF4, 0x58, 0x18, 0x2C, 0xBC, 0x1B,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
    };
    
    // Public exponent e = 65537 (0x10001)
    uint8_t test_e[RSA_BYTES] = {0};
    test_e[RSA_BYTES-3] = 0x01;
    test_e[RSA_BYTES-1] = 0x01;
    
    // Test plaintext (must be less than n)
    uint8_t test_plaintext[NUM_TEST_BLOCKS * RSA_BYTES] = {0};
    
    // Block 1: Simple test message
    const char* msg1 = "Hello RSA Encryption Test!";
    int msg1_len = strlen(msg1);
    memcpy(test_plaintext, msg1, msg1_len);
    
    // Block 2: Another test message  
    const char* msg2 = "FPGA Accelerated RSA with HLS";
    int msg2_len = strlen(msg2);
    memcpy(test_plaintext + RSA_BYTES, msg2, msg2_len);
    
    uint8_t ciphertext[NUM_TEST_BLOCKS * RSA_BYTES];
    
    // Print test inputs
    std::cout << "\nModulus (n) - First 128 bytes:" << std::endl;
    print_hex("", test_n, 128);
    
    std::cout << "\nPublic Exponent (e):" << std::endl;
    std::cout << "e = 65537 (0x10001)" << std::endl;
    
    std::cout << "\nOriginal Plaintext:" << std::endl;
    for (int block = 0; block < NUM_TEST_BLOCKS; block++) {
        std::cout << "Block " << block << ": ";
        print_hex("", test_plaintext + block * RSA_BYTES, 64);
    }
    
    // Perform encryption
    std::cout << "\n=== Encryption ===" << std::endl;
    rsa_encrypt(test_plaintext, test_n, test_e, ciphertext, NUM_TEST_BLOCKS);
    
    // Print ciphertext
    std::cout << "\nCiphertext:" << std::endl;
    for (int block = 0; block < NUM_TEST_BLOCKS; block++) {
        std::cout << "Block " << block << ": ";
        print_hex("", ciphertext + block * RSA_BYTES, 64);
    }
    
    // Basic validation
    std::cout << "\n=== Basic Validation ===" << std::endl;
    bool different = false;
    for (int i = 0; i < NUM_TEST_BLOCKS * RSA_BYTES; i++) {
        if (test_plaintext[i] != ciphertext[i]) {
            different = true;
            break;
        }
    }
    
    if (different) {
        std::cout << "✓ Encryption completed! Ciphertext differs from plaintext." << std::endl;
        
        // Performance info
        std::cout << "\n=== Performance Features ===" << std::endl;
        std::cout << "• Modular Exponentiation: Montgomery multiplication with 2048-bit integers" << std::endl;
        std::cout << "• Square-and-multiply: Pipelined binary method" << std::endl;
        std::cout << "• Memory interfaces: AXI4 with separate bundles" << std::endl;
        std::cout << "• Arrays: Partitioned for parallel access" << std::endl;
        std::cout << "• Arbitrary precision: Using ap_uint<2048> for large integers" << std::endl;
        
        return 0;
    } else {
        std::cout << "✗ Encryption failed! Ciphertext same as plaintext." << std::endl;
        return 1;
    }
}