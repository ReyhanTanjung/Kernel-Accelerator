#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <cstring>
#include <cstdlib>

#define AES_BLOCK_SIZE 16
#define AES_KEY_SIZE 16

// AES S-box
static const uint8_t sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

// Round constants
static const uint8_t rcon[11] = {
    0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
};

class AESCPU {
private:
    uint8_t roundKeys[11][16];
    
    void keyExpansion(const uint8_t* key) {
        // Copy original key
        memcpy(roundKeys[0], key, 16);
        
        for (int round = 1; round <= 10; round++) {
            uint8_t temp[4];
            
            // Copy last 4 bytes of previous round key
            for (int i = 0; i < 4; i++) {
                temp[i] = roundKeys[round-1][12 + i];
            }
            
            // Rotate word
            uint8_t t = temp[0];
            temp[0] = temp[1];
            temp[1] = temp[2];
            temp[2] = temp[3];
            temp[3] = t;
            
            // Apply S-box
            for (int i = 0; i < 4; i++) {
                temp[i] = sbox[temp[i]];
            }
            
            // XOR with round constant
            temp[0] ^= rcon[round];
            
            // Generate new round key
            for (int i = 0; i < 4; i++) {
                roundKeys[round][i] = roundKeys[round-1][i] ^ temp[i];
            }
            
            for (int i = 4; i < 16; i++) {
                roundKeys[round][i] = roundKeys[round-1][i] ^ roundKeys[round][i-4];
            }
        }
    }
    
    void subBytes(uint8_t state[16]) {
        for (int i = 0; i < 16; i++) {
            state[i] = sbox[state[i]];
        }
    }
    
    void shiftRows(uint8_t state[16]) {
        uint8_t temp[16];
        
        // Row 0: no shift
        temp[0] = state[0]; temp[4] = state[4]; temp[8] = state[8]; temp[12] = state[12];
        
        // Row 1: shift left by 1
        temp[1] = state[5]; temp[5] = state[9]; temp[9] = state[13]; temp[13] = state[1];
        
        // Row 2: shift left by 2
        temp[2] = state[10]; temp[6] = state[14]; temp[10] = state[2]; temp[14] = state[6];
        
        // Row 3: shift left by 3
        temp[3] = state[15]; temp[7] = state[3]; temp[11] = state[7]; temp[15] = state[11];
        
        memcpy(state, temp, 16);
    }
    
    uint8_t gfMul(uint8_t a, uint8_t b) {
        uint8_t result = 0;
        uint8_t hi_bit_set;
        
        for (int counter = 0; counter < 8; counter++) {
            if ((b & 1) == 1) {
                result ^= a;
            }
            hi_bit_set = (a & 0x80);
            a <<= 1;
            if (hi_bit_set == 0x80) {
                a ^= 0x1b;
            }
            b >>= 1;
        }
        return result;
    }
    
    void mixColumns(uint8_t state[16]) {
        uint8_t temp[16];
        
        for (int col = 0; col < 4; col++) {
            int offset = col * 4;
            temp[offset + 0] = gfMul(0x02, state[offset + 0]) ^ gfMul(0x03, state[offset + 1]) ^ state[offset + 2] ^ state[offset + 3];
            temp[offset + 1] = state[offset + 0] ^ gfMul(0x02, state[offset + 1]) ^ gfMul(0x03, state[offset + 2]) ^ state[offset + 3];
            temp[offset + 2] = state[offset + 0] ^ state[offset + 1] ^ gfMul(0x02, state[offset + 2]) ^ gfMul(0x03, state[offset + 3]);
            temp[offset + 3] = gfMul(0x03, state[offset + 0]) ^ state[offset + 1] ^ state[offset + 2] ^ gfMul(0x02, state[offset + 3]);
        }
        
        memcpy(state, temp, 16);
    }
    
    void addRoundKey(uint8_t state[16], int round) {
        for (int i = 0; i < 16; i++) {
            state[i] ^= roundKeys[round][i];
        }
    }
    
    void encryptBlock(const uint8_t* plaintext, uint8_t* ciphertext) {
        uint8_t state[16];
        memcpy(state, plaintext, 16);
        
        // Initial round key addition
        addRoundKey(state, 0);
        
        // 9 main rounds
        for (int round = 1; round <= 9; round++) {
            subBytes(state);
            shiftRows(state);
            mixColumns(state);
            addRoundKey(state, round);
        }
        
        // Final round (no MixColumns)
        subBytes(state);
        shiftRows(state);
        addRoundKey(state, 10);
        
        memcpy(ciphertext, state, 16);
    }
    
public:
    AESCPU() {
        std::cout << "✓ AES CPU implementation initialized" << std::endl;
    }
    
    void encrypt(const uint8_t* plaintext, const uint8_t* key, uint8_t* ciphertext, int num_blocks) {
        try {
            // Expand the key once
            keyExpansion(key);
            
            // Start timing
            auto start = std::chrono::high_resolution_clock::now();
            
            // Encrypt all blocks
            for (int i = 0; i < num_blocks; i++) {
                encryptBlock(plaintext + (i * AES_BLOCK_SIZE), 
                           ciphertext + (i * AES_BLOCK_SIZE));
            }
            
            // End timing
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            std::cout << "✓ Encryption completed in " << duration.count() << " μs" << std::endl;
            
            // Calculate throughput
            size_t total_size = num_blocks * AES_BLOCK_SIZE;
            double data_mb = (double)total_size / (1024.0 * 1024.0);
            double time_sec = (double)duration.count() / 1000000.0;
            double throughput = data_mb / time_sec;
            
            std::cout << "✓ Throughput: " << std::fixed << std::setprecision(2) 
                      << throughput << " MB/s" << std::endl;
                      
        } catch (const std::exception& e) {
            std::cerr << "Error during encryption: " << e.what() << std::endl;
            throw;
        }
    }
    
    ~AESCPU() {
        std::cout << "✓ AES CPU cleanup completed" << std::endl;
    }
};

void printHex(const std::string& label, const uint8_t* data, int size) {
    std::cout << label << ": ";
    for (int i = 0; i < size; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
        if ((i + 1) % 16 == 0 && i + 1 < size) std::cout << "\n" << std::string(label.length() + 2, ' ');
        else if (i + 1 < size) std::cout << " ";
    }
    std::cout << std::dec << std::endl;
}

void runTestVectors(AESCPU& aes) {
    std::cout << "\n=== AES Test Vectors ===" << std::endl;
    
    // Test vector 1: NIST test case
    uint8_t key1[16] = {
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
    };
    
    uint8_t plaintext1[16] = {
        0x32, 0x43, 0xf6, 0xa8, 0x88, 0x5a, 0x30, 0x8d,
        0x31, 0x31, 0x98, 0xa2, 0xe0, 0x37, 0x07, 0x34
    };
    
    uint8_t ciphertext1[16];
    
    std::cout << "\nTest 1: Single block encryption" << std::endl;
    printHex("Key", key1, 16);
    printHex("Plaintext", plaintext1, 16);
    
    aes.encrypt(plaintext1, key1, ciphertext1, 1);
    printHex("Ciphertext", ciphertext1, 16);
    
    // Expected result for verification
    uint8_t expected[16] = {
        0x39, 0x25, 0x84, 0x1d, 0x02, 0xdc, 0x09, 0xfb,
        0xdc, 0x11, 0x85, 0x97, 0x19, 0x6a, 0x0b, 0x32
    };
    
    bool correct = true;
    for (int i = 0; i < 16; i++) {
        if (ciphertext1[i] != expected[i]) {
            correct = false;
            break;
        }
    }
    
    if (correct) {
        std::cout << "✓ Test vector PASSED!" << std::endl;
    } else {
        std::cout << "✗ Test vector FAILED!" << std::endl;
        printHex("Expected", expected, 16);
    }
}

void runPerformanceTest(AESCPU& aes) {
    std::cout << "\n=== Performance Test ===" << std::endl;
    
    const int test_blocks[] = {1, 4, 16, 64, 256};
    const int num_tests = sizeof(test_blocks) / sizeof(test_blocks[0]);
    
    // Generate random key
    uint8_t key[16];
    for (int i = 0; i < 16; i++) {
        key[i] = rand() & 0xFF;
    }
    
    for (int t = 0; t < num_tests; t++) {
        int blocks = test_blocks[t];
        size_t data_size = blocks * AES_BLOCK_SIZE;
        
        std::vector<uint8_t> plaintext(data_size);
        std::vector<uint8_t> ciphertext(data_size);
        
        // Generate random plaintext
        for (size_t i = 0; i < data_size; i++) {
            plaintext[i] = rand() & 0xFF;
        }
        
        std::cout << "\nTest " << (t+1) << ": " << blocks << " blocks (" 
                  << data_size << " bytes)" << std::endl;
        
        aes.encrypt(plaintext.data(), key, ciphertext.data(), blocks);
    }
}

void runStressTest(AESCPU& aes) {
    std::cout << "\n=== Stress Test ===" << std::endl;
    
    const int max_blocks = 1024;
    const int iterations = 100;
    
    std::vector<uint8_t> plaintext(max_blocks * AES_BLOCK_SIZE);
    std::vector<uint8_t> ciphertext(max_blocks * AES_BLOCK_SIZE);
    uint8_t key[16];
    
    // Generate random data
    for (size_t i = 0; i < plaintext.size(); i++) {
        plaintext[i] = rand() & 0xFF;
    }
    for (int i = 0; i < 16; i++) {
        key[i] = rand() & 0xFF;
    }
    
    std::cout << "Running " << iterations << " iterations of " << max_blocks << " blocks each..." << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; i++) {
        aes.encrypt(plaintext.data(), key, ciphertext.data(), max_blocks);
        if ((i + 1) % 10 == 0) {
            std::cout << "Completed " << (i + 1) << "/" << iterations << " iterations" << std::endl;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    double total_data_mb = (double)(iterations * max_blocks * AES_BLOCK_SIZE) / (1024.0 * 1024.0);
    double time_sec = (double)duration.count() / 1000.0;
    double avg_throughput = total_data_mb / time_sec;
    
    std::cout << "✓ Stress test completed!" << std::endl;
    std::cout << "Total data processed: " << std::fixed << std::setprecision(2) 
              << total_data_mb << " MB" << std::endl;
    std::cout << "Average throughput: " << avg_throughput << " MB/s" << std::endl;
}

void runBenchmarkComparison() {
    std::cout << "\n=== Benchmark Summary ===" << std::endl;
    std::cout << "CPU Implementation: AES-128 Encryption" << std::endl;
    std::cout << "Algorithm: Standard AES with lookup tables" << std::endl;
    std::cout << "Block size: 128-bit (16 bytes)" << std::endl;
    std::cout << "Key size: 128-bit (16 bytes)" << std::endl;
    std::cout << "\nFor comparison with FPGA accelerator:" << std::endl;
    std::cout << "- Run both programs with identical test parameters" << std::endl;
    std::cout << "- Compare throughput (MB/s) values" << std::endl;
    std::cout << "- Note latency differences in microseconds" << std::endl;
    std::cout << "- Consider power consumption differences" << std::endl;
}

int main(int argc, char* argv[]) {
    try {
        std::cout << "=== AES CPU Benchmark Application ===" << std::endl;
        std::cout << "Platform: CPU-only implementation" << std::endl;
        std::cout << "Purpose: Benchmarking comparison with FPGA accelerator" << std::endl;
        
        // Initialize AES CPU implementation
        AESCPU aes;
        
        // Run the same tests as FPGA version
        runTestVectors(aes);
        runPerformanceTest(aes);
        runStressTest(aes);
        runBenchmarkComparison();
        
        std::cout << "\n=== All tests completed successfully! ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Application failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}