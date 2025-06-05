#include <iostream>
#include <iomanip>
#include <cstring>
#include "sha256.h"

void print_hash(const uint8_t hash[32]) {
    for (int i = 0; i < 32; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    std::cout << std::dec << std::endl;
}

void pad_message(const char* message, uint8_t* padded, int* num_blocks) {
    size_t msg_len = strlen(message);
    size_t total_len = msg_len;
    
    // Calculate padding
    size_t pad_len = 64 - ((msg_len + 9) % 64);
    if (pad_len == 64) pad_len = 0;
    total_len = msg_len + 1 + pad_len + 8;
    
    *num_blocks = total_len / 64;
    
    // Copy message
    memcpy(padded, message, msg_len);
    
    // Add padding
    padded[msg_len] = 0x80;
    for (size_t i = msg_len + 1; i < msg_len + 1 + pad_len; i++) {
        padded[i] = 0x00;
    }
    
    // Add length in bits (big-endian)
    uint64_t bit_len = msg_len * 8;
    for (int i = 0; i < 8; i++) {
        padded[msg_len + 1 + pad_len + i] = (bit_len >> (56 - i * 8)) & 0xFF;
    }
}

int main() {
    std::cout << "=== SHA-256 Hardware Accelerator Test ===" << std::endl;
    
    // Test 1: Empty string
    {
        std::cout << "\nTest 1: Empty string" << std::endl;
        uint8_t padded[64];
        uint8_t hash[32];
        int num_blocks;
        
        pad_message("", padded, &num_blocks);
        sha256_hash(padded, hash, num_blocks);
        
        std::cout << "Hash: ";
        print_hash(hash);
        std::cout << "Expected: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855" << std::endl;
    }
    
    // Test 2: "abc"
    {
        std::cout << "\nTest 2: \"abc\"" << std::endl;
        uint8_t padded[64];
        uint8_t hash[32];
        int num_blocks;
        
        pad_message("abc", padded, &num_blocks);
        sha256_hash(padded, hash, num_blocks);
        
        std::cout << "Hash: ";
        print_hash(hash);
        std::cout << "Expected: ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad" << std::endl;
    }
    
    // Test 3: Longer message
    {
        std::cout << "\nTest 3: \"The quick brown fox jumps over the lazy dog\"" << std::endl;
        const char* msg = "The quick brown fox jumps over the lazy dog";
        uint8_t padded[128];
        uint8_t hash[32];
        int num_blocks;
        
        pad_message(msg, padded, &num_blocks);
        sha256_hash(padded, hash, num_blocks);
        
        std::cout << "Hash: ";
        print_hash(hash);
        std::cout << "Expected: d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592" << std::endl;
    }
    
    // Test 4: Multiple blocks
    {
        std::cout << "\nTest 4: Multiple blocks (128 bytes)" << std::endl;
        const char* msg = "This is a longer message that will span multiple SHA-256 blocks to test the pipelined implementation.";
        uint8_t padded[256];
        uint8_t hash[32];
        int num_blocks;
        
        pad_message(msg, padded, &num_blocks);
        std::cout << "Message length: " << strlen(msg) << " bytes" << std::endl;
        std::cout << "Number of blocks: " << num_blocks << std::endl;
        
        sha256_hash(padded, hash, num_blocks);
        
        std::cout << "Hash: ";
        print_hash(hash);
    }
    
    // Performance test
    {
        std::cout << "\n=== Performance Features ===" << std::endl;
        std::cout << "• Message schedule: Pipelined expansion with II=1" << std::endl;
        std::cout << "• Compression function: Fully pipelined with II=1" << std::endl;
        std::cout << "• Block processing: Pipelined with II=64" << std::endl;
        std::cout << "• Memory interfaces: AXI4 with separate bundles" << std::endl;
        std::cout << "• Arrays: Partitioned for parallel access" << std::endl;
        std::cout << "• Constants: Stored in lookup tables" << std::endl;
    }
    
    std::cout << "\n✓ All tests completed!" << std::endl;
    return 0;
}