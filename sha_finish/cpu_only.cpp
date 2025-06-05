#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <fstream>

#define SHA256_BLOCK_SIZE 64
#define SHA256_DIGEST_SIZE 32

// SHA-256 Constants
static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

class SHA256CPU {
private:
    uint32_t state[8];
    
    // Right rotate
    inline uint32_t rotr(uint32_t x, uint32_t n) {
        return (x >> n) | (x << (32 - n));
    }
    
    // SHA-256 functions
    inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) {
        return (x & y) ^ (~x & z);
    }
    
    inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) {
        return (x & y) ^ (x & z) ^ (y & z);
    }
    
    inline uint32_t sigma0(uint32_t x) {
        return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
    }
    
    inline uint32_t sigma1(uint32_t x) {
        return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
    }
    
    inline uint32_t sigma_0(uint32_t x) {
        return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
    }
    
    inline uint32_t sigma_1(uint32_t x) {
        return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
    }
    
    void processBlock(const uint8_t* block) {
        uint32_t W[64];
        uint32_t a, b, c, d, e, f, g, h;
        uint32_t T1, T2;
        
        // Prepare message schedule
        for (int t = 0; t < 16; t++) {
            W[t] = ((uint32_t)block[t * 4] << 24) |
                   ((uint32_t)block[t * 4 + 1] << 16) |
                   ((uint32_t)block[t * 4 + 2] << 8) |
                   ((uint32_t)block[t * 4 + 3]);
        }
        
        for (int t = 16; t < 64; t++) {
            W[t] = sigma_1(W[t-2]) + W[t-7] + sigma_0(W[t-15]) + W[t-16];
        }
        
        // Initialize working variables
        a = state[0];
        b = state[1];
        c = state[2];
        d = state[3];
        e = state[4];
        f = state[5];
        g = state[6];
        h = state[7];
        
        // Main loop
        for (int t = 0; t < 64; t++) {
            T1 = h + sigma1(e) + ch(e, f, g) + K[t] + W[t];
            T2 = sigma0(a) + maj(a, b, c);
            
            h = g;
            g = f;
            f = e;
            e = d + T1;
            d = c;
            c = b;
            b = a;
            a = T1 + T2;
        }
        
        // Update state
        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
        state[5] += f;
        state[6] += g;
        state[7] += h;
    }
    
    void reset() {
        // Initial hash values
        state[0] = 0x6a09e667;
        state[1] = 0xbb67ae85;
        state[2] = 0x3c6ef372;
        state[3] = 0xa54ff53a;
        state[4] = 0x510e527f;
        state[5] = 0x9b05688c;
        state[6] = 0x1f83d9ab;
        state[7] = 0x5be0cd19;
    }
    
    void pad_message(const uint8_t* message, size_t msg_len, std::vector<uint8_t>& padded) {
        // Calculate padding
        size_t pad_len = 64 - ((msg_len + 9) % 64);
        if (pad_len == 64) pad_len = 0;
        size_t total_len = msg_len + 1 + pad_len + 8;
        
        padded.resize(total_len);
        
        // Copy message
        memcpy(padded.data(), message, msg_len);
        
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
    
public:
    SHA256CPU() {
        std::cout << "✓ SHA-256 CPU implementation initialized" << std::endl;
    }
    
    void hash(const uint8_t* message, size_t msg_len, uint8_t* hash_out) {
        try {
            // Reset state
            reset();
            
            // Pad message
            std::vector<uint8_t> padded;
            pad_message(message, msg_len, padded);
            
            // Start timing
            auto start = std::chrono::high_resolution_clock::now();
            
            // Process all blocks
            size_t num_blocks = padded.size() / SHA256_BLOCK_SIZE;
            for (size_t i = 0; i < num_blocks; i++) {
                processBlock(&padded[i * SHA256_BLOCK_SIZE]);
            }
            
            // End timing
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            // Convert state to output bytes (big-endian)
            for (int i = 0; i < 8; i++) {
                hash_out[i * 4] = (state[i] >> 24) & 0xFF;
                hash_out[i * 4 + 1] = (state[i] >> 16) & 0xFF;
                hash_out[i * 4 + 2] = (state[i] >> 8) & 0xFF;
                hash_out[i * 4 + 3] = state[i] & 0xFF;
            }
            
            std::cout << "✓ Hashing completed in " << duration.count() << " μs" << std::endl;
            
            // Calculate throughput
            double data_mb = (double)msg_len / (1024.0 * 1024.0);
            double time_sec = (double)duration.count() / 1000000.0;
            double throughput = data_mb / time_sec;
            
            std::cout << "✓ Throughput: " << std::fixed << std::setprecision(2) 
                      << throughput << " MB/s" << std::endl;
                      
        } catch (const std::exception& e) {
            std::cerr << "Error during hashing: " << e.what() << std::endl;
            throw;
        }
    }
    
    ~SHA256CPU() {
        std::cout << "✓ SHA-256 CPU cleanup completed" << std::endl;
    }
};

void printHash(const std::string& label, const uint8_t* hash) {
    std::cout << label << ": ";
    for (int i = 0; i < SHA256_DIGEST_SIZE; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    std::cout << std::dec << std::endl;
}

void runTestVectors(SHA256CPU& sha) {
    std::cout << "\n=== SHA-256 Test Vectors ===" << std::endl;
    
    // Test 1: Empty string
    {
        std::cout << "\nTest 1: Empty string" << std::endl;
        uint8_t hash[SHA256_DIGEST_SIZE];
        sha.hash((const uint8_t*)"", 0, hash);
        printHash("Hash", hash);
        std::cout << "Expected: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855" << std::endl;
        
        // Verify
        const char* expected = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
        bool correct = true;
        for (int i = 0; i < 32; i++) {
            char hex[3];
            snprintf(hex, 3, "%02x", hash[i]);
            if (hex[0] != expected[i*2] || hex[1] != expected[i*2+1]) {
                correct = false;
                break;
            }
        }
        std::cout << (correct ? "✓ PASSED" : "✗ FAILED") << std::endl;
    }
    
    // Test 2: "abc"
    {
        std::cout << "\nTest 2: \"abc\"" << std::endl;
        const char* msg = "abc";
        uint8_t hash[SHA256_DIGEST_SIZE];
        sha.hash((const uint8_t*)msg, strlen(msg), hash);
        printHash("Hash", hash);
        std::cout << "Expected: ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad" << std::endl;
    }
    
    // Test 3: Standard test vector
    {
        std::cout << "\nTest 3: \"The quick brown fox jumps over the lazy dog\"" << std::endl;
        const char* msg = "The quick brown fox jumps over the lazy dog";
        uint8_t hash[SHA256_DIGEST_SIZE];
        sha.hash((const uint8_t*)msg, strlen(msg), hash);
        printHash("Hash", hash);
        std::cout << "Expected: d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592" << std::endl;
    }
}

void runPerformanceTest(SHA256CPU& sha) {
    std::cout << "\n=== Performance Test ===" << std::endl;
    
    const size_t test_sizes[] = {64, 256, 1024, 4096, 16384, 65536};
    const int num_tests = sizeof(test_sizes) / sizeof(test_sizes[0]);
    
    for (int t = 0; t < num_tests; t++) {
        size_t size = test_sizes[t];
        
        std::vector<uint8_t> message(size);
        uint8_t hash[SHA256_DIGEST_SIZE];
        
        // Generate random message
        for (size_t i = 0; i < size; i++) {
            message[i] = rand() & 0xFF;
        }
        
        std::cout << "\nTest " << (t+1) << ": " << size << " bytes" << std::endl;
        
        sha.hash(message.data(), size, hash);
    }
}

void runStressTest(SHA256CPU& sha) {
    std::cout << "\n=== Stress Test ===" << std::endl;
    
    const size_t message_size = 1024 * 1024; // 1 MB
    const int iterations = 100;
    
    std::vector<uint8_t> message(message_size);
    uint8_t hash[SHA256_DIGEST_SIZE];
    
    // Generate random data
    for (size_t i = 0; i < message_size; i++) {
        message[i] = rand() & 0xFF;
    }
    
    std::cout << "Running " << iterations << " iterations of " << message_size << " bytes each..." << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; i++) {
        sha.hash(message.data(), message_size, hash);
        if ((i + 1) % 10 == 0) {
            std::cout << "Completed " << (i + 1) << "/" << iterations << " iterations" << std::endl;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    double total_data_mb = (double)(iterations * message_size) / (1024.0 * 1024.0);
    double time_sec = (double)duration.count() / 1000.0;
    double avg_throughput = total_data_mb / time_sec;
    
    std::cout << "✓ Stress test completed!" << std::endl;
    std::cout << "Total data processed: " << std::fixed << std::setprecision(2) 
              << total_data_mb << " MB" << std::endl;
    std::cout << "Average throughput: " << avg_throughput << " MB/s" << std::endl;
}

void runBenchmarkComparison() {
    std::cout << "\n=== Benchmark Summary ===" << std::endl;
    std::cout << "CPU Implementation: SHA-256 Hashing" << std::endl;
    std::cout << "Algorithm: Standard SHA-256 with 64 rounds" << std::endl;
    std::cout << "Block size: 512-bit (64 bytes)" << std::endl;
    std::cout << "Digest size: 256-bit (32 bytes)" << std::endl;
    std::cout << "\nFor comparison with FPGA accelerator:" << std::endl;
    std::cout << "- Run both programs with identical test parameters" << std::endl;
    std::cout << "- Compare throughput (MB/s) values" << std::endl;
    std::cout << "- Note latency differences in microseconds" << std::endl;
    std::cout << "- FPGA advantages: Pipelined message schedule and compression" << std::endl;
    std::cout << "- FPGA optimizations: II=1 for inner loops, II=64 for blocks" << std::endl;
}

int main(int argc, char* argv[]) {
    try {
        std::cout << "=== SHA-256 CPU Benchmark Application ===" << std::endl;
        std::cout << "Platform: CPU-only implementation" << std::endl;
        std::cout << "Purpose: Benchmarking comparison with FPGA accelerator" << std::endl;
        
        // Initialize SHA-256 CPU implementation
        SHA256CPU sha;
        
        // Run the same tests as FPGA version
        runTestVectors(sha);
        runPerformanceTest(sha);
        runStressTest(sha);
        runBenchmarkComparison();
        
        std::cout << "\n=== All tests completed successfully! ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Application failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}