#include <iostream>
#include <iomanip>
#include <cstring>
#include "chacha20.h"

#define NUM_TEST_BLOCKS 2

int main() {
    // Test vectors for ChaCha20
    uint8_t test_key[32] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    
    uint8_t test_nonce[12] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4a,
        0x00, 0x00, 0x00, 0x00
    };
    
    uint32_t test_counter = 1;
    
    uint8_t test_plaintext[NUM_TEST_BLOCKS * 64];
    uint8_t ciphertext[NUM_TEST_BLOCKS * 64];
    uint8_t decrypted[NUM_TEST_BLOCKS * 64];
    
    // Initialize test plaintext with simple pattern
    for (int i = 0; i < NUM_TEST_BLOCKS * 64; i++) {
        test_plaintext[i] = (uint8_t)(i % 256);
    }
    
    std::cout << "=== ChaCha20 Simple Test ===" << std::endl;
    
    // Print key
    std::cout << "Key: ";
    for (int i = 0; i < 32; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') 
                  << (int)test_key[i] << " ";
    }
    std::cout << std::endl;
    
    // Print nonce
    std::cout << "Nonce: ";
    for (int i = 0; i < 12; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') 
                  << (int)test_nonce[i] << " ";
    }
    std::cout << std::endl;
    
    std::cout << "Counter: " << std::dec << test_counter << std::endl;
    
    // Print first 16 bytes of plaintext
    std::cout << "Plaintext (first 16 bytes): ";
    for (int i = 0; i < 16; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') 
                  << (int)test_plaintext[i] << " ";
    }
    std::cout << std::endl;
    
    // Perform encryption
    std::cout << "\n=== Encryption ===" << std::endl;
    chacha20_encrypt(test_plaintext, test_key, test_nonce, test_counter, 
                    ciphertext, NUM_TEST_BLOCKS);
    
    // Print first 16 bytes of ciphertext
    std::cout << "Ciphertext (first 16 bytes): ";
    for (int i = 0; i < 16; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') 
                  << (int)ciphertext[i] << " ";
    }
    std::cout << std::endl;
    
    // Perform decryption (ChaCha20 is symmetric)
    std::cout << "\n=== Decryption ===" << std::endl;
    chacha20_encrypt(ciphertext, test_key, test_nonce, test_counter, 
                    decrypted, NUM_TEST_BLOCKS);
    
    // Print first 16 bytes of decrypted
    std::cout << "Decrypted (first 16 bytes): ";
    for (int i = 0; i < 16; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') 
                  << (int)decrypted[i] << " ";
    }
    std::cout << std::endl;
    
    // Validation
    std::cout << "\n=== Validation ===" << std::endl;
    bool encryption_different = false;
    bool decryption_correct = true;
    
    for (int i = 0; i < NUM_TEST_BLOCKS * 64; i++) {
        if (test_plaintext[i] != ciphertext[i]) {
            encryption_different = true;
        }
        if (test_plaintext[i] != decrypted[i]) {
            decryption_correct = false;
            std::cout << "Mismatch at byte " << i << ": " 
                      << std::hex << (int)test_plaintext[i] 
                      << " != " << (int)decrypted[i] << std::endl;
            break;
        }
    }
    
    if (encryption_different && decryption_correct) {
        std::cout << "✓ ChaCha20 test PASSED!" << std::endl;
        std::cout << "  - Encryption produces different output ✓" << std::endl;
        std::cout << "  - Decryption recovers original data ✓" << std::endl;
        return 0;
    } else {
        std::cout << "✗ ChaCha20 test FAILED!" << std::endl;
        if (!encryption_different) {
            std::cout << "  - Ciphertext same as plaintext ✗" << std::endl;
        }
        if (!decryption_correct) {
            std::cout << "  - Decryption failed ✗" << std::endl;
        }
        return 1;
    }
}