#include <iostream>
#include <iomanip>
#include <cstring>
#include "sha3.h"

// Helper function to print bytes as hex
void print_hex(const uint8_t* data, int len) {
    for (int i = 0; i < len; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
    }
}

int main() {
    std::cout << "SHA3-256 Simplified FPGA Implementation Test\n";
    std::cout << "============================================\n\n";
    
    // Test 1: Empty message
    {
        std::cout << "Test 1: Empty message\n";
        uint8_t message[1] = {0};
        uint8_t hash[SHA3_256_HASH_SIZE];
        
        sha3_256(message, 0, hash, 1);
        
        std::cout << "Input: (empty)\n";
        std::cout << "Hash:  ";
        print_hex(hash, SHA3_256_HASH_SIZE);
        std::cout << "\n\n";
    }
    
    // Test 2: Simple message "abc"
    {
        std::cout << "Test 2: Message 'abc'\n";
        const char* input = "abc";
        uint8_t message[SHA3_256_RATE];
        memset(message, 0, SHA3_256_RATE);
        memcpy(message, input, strlen(input));
        
        uint8_t hash[SHA3_256_HASH_SIZE];
        
        sha3_256(message, strlen(input), hash, 1);
        
        std::cout << "Input: " << input << "\n";
        std::cout << "Hash:  ";
        print_hex(hash, SHA3_256_HASH_SIZE);
        std::cout << "\n\n";
    }
    
    // Test 3: Longer message
    {
        std::cout << "Test 3: Pattern message\n";
        uint8_t message[SHA3_256_RATE];
        
        // Create a pattern
        for (int i = 0; i < SHA3_256_RATE; i++) {
            message[i] = i & 0xFF;
        }
        
        uint8_t hash[SHA3_256_HASH_SIZE];
        
        sha3_256(message, SHA3_256_RATE, hash, 1);
        
        std::cout << "Input: Pattern (0x00, 0x01, 0x02, ..., 0x87)\n";
        std::cout << "Hash:  ";
        print_hex(hash, SHA3_256_HASH_SIZE);
        std::cout << "\n\n";
    }
    
    // Test 4: Multi-block message
    {
        std::cout << "Test 4: Multi-block message\n";
        const uint32_t total_size = SHA3_256_RATE * 2 + 50; // 2.x blocks
        uint8_t large_message[total_size];
        
        // Fill with pattern
        for (uint32_t i = 0; i < total_size; i++) {
            large_message[i] = (i * 37 + 123) & 0xFF;
        }
        
        uint32_t num_blocks = (total_size + SHA3_256_RATE - 1) / SHA3_256_RATE;
        uint8_t hash[SHA3_256_HASH_SIZE];
        
        sha3_256(large_message, total_size, hash, num_blocks);
        
        std::cout << "Input: " << total_size << " bytes, " << num_blocks << " blocks\n";
        std::cout << "Hash:  ";
        print_hex(hash, SHA3_256_HASH_SIZE);
        std::cout << "\n\n";
    }
    
    // Test 5: Consistency check
    {
        std::cout << "Test 5: Consistency check (same input twice)\n";
        const char* input = "Hello FPGA World!";
        uint8_t message[SHA3_256_RATE];
        memset(message, 0, SHA3_256_RATE);
        memcpy(message, input, strlen(input));
        
        uint8_t hash1[SHA3_256_HASH_SIZE];
        uint8_t hash2[SHA3_256_HASH_SIZE];
        
        sha3_256(message, strlen(input), hash1, 1);
        sha3_256(message, strlen(input), hash2, 1);
        
        std::cout << "Input: " << input << "\n";
        std::cout << "Hash1: ";
        print_hex(hash1, SHA3_256_HASH_SIZE);
        std::cout << "\nHash2: ";
        print_hex(hash2, SHA3_256_HASH_SIZE);
        
        bool consistent = true;
        for (int i = 0; i < SHA3_256_HASH_SIZE; i++) {
            if (hash1[i] != hash2[i]) {
                consistent = false;
                break;
            }
        }
        
        std::cout << "\nConsistency: " << (consistent ? "PASS" : "FAIL") << "\n\n";
        
        if (!consistent) {
            std::cout << "ERROR: Inconsistent results!\n";
            return 1;
        }
    }
    
    std::cout << "All tests completed successfully!\n";
    std::cout << "Note: This is a simplified SHA-3 implementation for FPGA testing.\n";
    std::cout << "For production use, implement the full 24-round Keccak-f function.\n";
    
    return 0;
}