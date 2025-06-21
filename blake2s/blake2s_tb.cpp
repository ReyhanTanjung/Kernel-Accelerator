#include <iostream>
#include <cstring>
#include <iomanip>
#include "blake2s.h"

void print_hash(const uint8_t* hash, int len) {
    for (int i = 0; i < len; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    std::cout << std::dec << std::endl;
}

int main() {
    // Test case 1: Empty string
    {
        std::cout << "Test 1: Empty string" << std::endl;
        const char* input = "";
        uint8_t output[32];
        
        blake2s_hash((const uint8_t*)input, strlen(input), output, 32, nullptr, 0);
        
        std::cout << "Input: \"" << input << "\"" << std::endl;
        std::cout << "Output: ";
        print_hash(output, 32);
        
        // Expected: 69217a3079908094e11121d042354a7c1f55b6482ca1a51e1b250dfd1ed0eef9
        const uint8_t expected1[] = {
            0x69, 0x21, 0x7a, 0x30, 0x79, 0x90, 0x80, 0x94,
            0xe1, 0x11, 0x21, 0xd0, 0x42, 0x35, 0x4a, 0x7c,
            0x1f, 0x55, 0xb6, 0x48, 0x2c, 0xa1, 0xa5, 0x1e,
            0x1b, 0x25, 0x0d, 0xfd, 0x1e, 0xd0, 0xee, 0xf9
        };
        
        bool match = true;
        for (int i = 0; i < 32; i++) {
            if (output[i] != expected1[i]) {
                match = false;
                break;
            }
        }
        std::cout << "Result: " << (match ? "PASS" : "FAIL") << std::endl << std::endl;
    }
    
    // Test case 2: "abc"
    {
        std::cout << "Test 2: \"abc\"" << std::endl;
        const char* input = "abc";
        uint8_t output[32];
        
        blake2s_hash((const uint8_t*)input, strlen(input), output, 32, nullptr, 0);
        
        std::cout << "Input: \"" << input << "\"" << std::endl;
        std::cout << "Output: ";
        print_hash(output, 32);
        
        // Expected: 508c5e8c327c14e2e1a72ba34eeb452f37458b209ed63a294d999b4c86675982
        const uint8_t expected2[] = {
            0x50, 0x8c, 0x5e, 0x8c, 0x32, 0x7c, 0x14, 0xe2,
            0xe1, 0xa7, 0x2b, 0xa3, 0x4e, 0xeb, 0x45, 0x2f,
            0x37, 0x45, 0x8b, 0x20, 0x9e, 0xd6, 0x3a, 0x29,
            0x4d, 0x99, 0x9b, 0x4c, 0x86, 0x67, 0x59, 0x82
        };
        
        bool match = true;
        for (int i = 0; i < 32; i++) {
            if (output[i] != expected2[i]) {
                match = false;
                break;
            }
        }
        std::cout << "Result: " << (match ? "PASS" : "FAIL") << std::endl << std::endl;
    }
    
    // Test case 3: Longer message
    {
        std::cout << "Test 3: Longer message" << std::endl;
        const char* input = "The quick brown fox jumps over the lazy dog";
        uint8_t output[32];
        
        blake2s_hash((const uint8_t*)input, strlen(input), output, 32, nullptr, 0);
        
        std::cout << "Input: \"" << input << "\"" << std::endl;
        std::cout << "Output: ";
        print_hash(output, 32);
        std::cout << "Result: Generated (no reference to compare)" << std::endl << std::endl;
    }
    
    // Test case 4: With key
    {
        std::cout << "Test 4: With key" << std::endl;
        const char* input = "hello world";
        const char* key = "secret key";
        uint8_t output[32];
        
        blake2s_hash((const uint8_t*)input, strlen(input), output, 32, 
                     (const uint8_t*)key, strlen(key));
        
        std::cout << "Input: \"" << input << "\"" << std::endl;
        std::cout << "Key: \"" << key << "\"" << std::endl;
        std::cout << "Output: ";
        print_hash(output, 32);
        std::cout << "Result: Generated (no reference to compare)" << std::endl << std::endl;
    }
    
    // Test case 5: Different output length
    {
        std::cout << "Test 5: 16-byte output" << std::endl;
        const char* input = "test";
        uint8_t output[16];
        
        blake2s_hash((const uint8_t*)input, strlen(input), output, 16, nullptr, 0);
        
        std::cout << "Input: \"" << input << "\"" << std::endl;
        std::cout << "Output (16 bytes): ";
        print_hash(output, 16);
        std::cout << "Result: Generated (no reference to compare)" << std::endl << std::endl;
    }
    
    std::cout << "All tests completed." << std::endl;
    return 0;
}