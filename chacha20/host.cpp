#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <cstring>
#include <cstdlib>

// XRT includes for Xilinx Runtime
#include "xrt/xrt_bo.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_kernel.h"

#define CHACHA20_BLOCK_SIZE 64  // 512-bit block (64 bytes)
#define CHACHA20_KEY_SIZE 32    // 256-bit key (32 bytes)
#define CHACHA20_NONCE_SIZE 12  // 96-bit nonce (12 bytes)

class ChaCha20Host {
private:
    xrt::device device;
    xrt::kernel kernel;
    xrt::bo bo_plaintext, bo_key, bo_nonce, bo_ciphertext;
    
public:
    ChaCha20Host(const std::string& xclbin_path, int device_id = 0) {
        try {
            // Initialize device
            device = xrt::device(device_id);
            auto uuid = device.load_xclbin(xclbin_path);
            
            // Create kernel
            kernel = xrt::kernel(device, uuid, "chacha20_encrypt");
            
            std::cout << "✓ ChaCha20 Hardware accelerator initialized successfully" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error initializing ChaCha20 accelerator: " << e.what() << std::endl;
            throw;
        }
    }
    
    void allocateBuffers(int max_blocks) {
        size_t plaintext_size = max_blocks * CHACHA20_BLOCK_SIZE;
        size_t ciphertext_size = max_blocks * CHACHA20_BLOCK_SIZE;
        size_t key_size = CHACHA20_KEY_SIZE;
        size_t nonce_size = CHACHA20_NONCE_SIZE;
        
        try {
            // Allocate buffer objects
            bo_plaintext = xrt::bo(device, plaintext_size, kernel.group_id(0));
            bo_key = xrt::bo(device, key_size, kernel.group_id(1));
            bo_nonce = xrt::bo(device, nonce_size, kernel.group_id(2));
            bo_ciphertext = xrt::bo(device, ciphertext_size, kernel.group_id(4));
            
            std::cout << "✓ Buffers allocated for " << max_blocks << " blocks" << std::endl;
            std::cout << "  - Plaintext/Ciphertext: " << plaintext_size << " bytes" << std::endl;
            std::cout << "  - Key: " << key_size << " bytes" << std::endl;
            std::cout << "  - Nonce: " << nonce_size << " bytes" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error allocating buffers: " << e.what() << std::endl;
            throw;
        }
    }
    
    void encrypt(const uint8_t* plaintext, const uint8_t* key, const uint8_t* nonce, 
                uint32_t counter, uint8_t* ciphertext, int num_blocks) {
        try {
            size_t plaintext_size = num_blocks * CHACHA20_BLOCK_SIZE;
            size_t ciphertext_size = num_blocks * CHACHA20_BLOCK_SIZE;
            
            // Map buffers and copy data
            auto plaintext_map = bo_plaintext.map<uint8_t*>();
            auto key_map = bo_key.map<uint8_t*>();
            auto nonce_map = bo_nonce.map<uint8_t*>();
            auto ciphertext_map = bo_ciphertext.map<uint8_t*>();
            
            std::memcpy(plaintext_map, plaintext, plaintext_size);
            std::memcpy(key_map, key, CHACHA20_KEY_SIZE);
            std::memcpy(nonce_map, nonce, CHACHA20_NONCE_SIZE);
            
            // Sync buffers to device
            bo_plaintext.sync(XCL_BO_SYNC_BO_TO_DEVICE);
            bo_key.sync(XCL_BO_SYNC_BO_TO_DEVICE);
            bo_nonce.sync(XCL_BO_SYNC_BO_TO_DEVICE);
            
            // Start timing
            auto start = std::chrono::high_resolution_clock::now();
            
            // Run kernel (plaintext, key, nonce, counter, ciphertext, num_blocks)
            auto run = kernel(bo_plaintext, bo_key, bo_nonce, counter, bo_ciphertext, num_blocks);
            run.wait();
            
            // End timing
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            // Sync result back
            bo_ciphertext.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
            
            // Copy result
            std::memcpy(ciphertext, ciphertext_map, ciphertext_size);
            
            std::cout << "✓ Encryption completed in " << duration.count() << " μs" << std::endl;
            
            // Calculate throughput
            double data_mb = (double)(plaintext_size) / (1024.0 * 1024.0);
            double time_sec = (double)duration.count() / 1000000.0;
            double throughput = data_mb / time_sec;
            
            std::cout << "✓ Throughput: " << std::fixed << std::setprecision(2) 
                      << throughput << " MB/s" << std::endl;
                      
        } catch (const std::exception& e) {
            std::cerr << "Error during encryption: " << e.what() << std::endl;
            throw;
        }
    }
    
    ~ChaCha20Host() {
        std::cout << "✓ ChaCha20 Host cleanup completed" << std::endl;
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

void runTestVectors(ChaCha20Host& chacha20) {
    std::cout << "\n=== ChaCha20 Test Vectors ===" << std::endl;
    
    // Test vector 1: RFC 8439 test case
    uint8_t key1[32] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    
    uint8_t nonce1[12] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4a,
        0x00, 0x00, 0x00, 0x00
    };
    
    uint32_t counter1 = 1;
    
    // Test plaintext - "Ladies and Gentlemen of the class of '99..."
    uint8_t plaintext1[64];
    const char* test_text = "Ladies and Gentlemen of the class of '99: If I could offer you ";
    std::memcpy(plaintext1, test_text, 64);
    
    uint8_t ciphertext1[64];
    uint8_t decrypted1[64];
    
    std::cout << "\nTest 1: Single block encryption/decryption" << std::endl;
    printHex("Key", key1, 32);
    printHex("Nonce", nonce1, 12);
    std::cout << "Counter: " << counter1 << std::endl;
    printHex("Plaintext (first 32 bytes)", plaintext1, 32);
    
    // Encrypt
    chacha20.encrypt(plaintext1, key1, nonce1, counter1, ciphertext1, 1);
    printHex("Ciphertext (first 32 bytes)", ciphertext1, 32);
    
    // Decrypt (ChaCha20 is symmetric)
    chacha20.encrypt(ciphertext1, key1, nonce1, counter1, decrypted1, 1);
    printHex("Decrypted (first 32 bytes)", decrypted1, 32);
    
    // Verify
    bool success = std::memcmp(plaintext1, decrypted1, 64) == 0;
    std::cout << "✓ Decryption verification: " << (success ? "PASSED" : "FAILED") << std::endl;
}

void runPerformanceTest(ChaCha20Host& chacha20) {
    std::cout << "\n=== Performance Test ===" << std::endl;
    
    const int test_blocks[] = {1, 4, 16, 64, 256};
    const int num_tests = sizeof(test_blocks) / sizeof(test_blocks[0]);
    
    // Generate random key and nonce
    uint8_t key[32];
    uint8_t nonce[12];
    for (int i = 0; i < 32; i++) {
        key[i] = rand() & 0xFF;
    }
    for (int i = 0; i < 12; i++) {
        nonce[i] = rand() & 0xFF;
    }
    uint32_t counter = 1;
    
    for (int t = 0; t < num_tests; t++) {
        int blocks = test_blocks[t];
        size_t data_size = blocks * CHACHA20_BLOCK_SIZE;
        
        std::vector<uint8_t> plaintext(data_size);
        std::vector<uint8_t> ciphertext(data_size);
        
        // Generate random plaintext
        for (size_t i = 0; i < data_size; i++) {
            plaintext[i] = rand() & 0xFF;
        }
        
        std::cout << "\nTest " << (t+1) << ": " << blocks << " blocks (" 
                  << data_size << " bytes, " << std::fixed << std::setprecision(1)
                  << (double)data_size / 1024.0 << " KB)" << std::endl;
        
        chacha20.encrypt(plaintext.data(), key, nonce, counter, ciphertext.data(), blocks);
    }
}

void runStressTest(ChaCha20Host& chacha20) {
    std::cout << "\n=== Stress Test ===" << std::endl;
    
    const int max_blocks = 512;  // 32KB per iteration
    const int iterations = 50;
    
    std::vector<uint8_t> plaintext(max_blocks * CHACHA20_BLOCK_SIZE);
    std::vector<uint8_t> ciphertext(max_blocks * CHACHA20_BLOCK_SIZE);
    uint8_t key[32];
    uint8_t nonce[12];
    uint32_t counter = 1;
    
    // Generate random data
    for (size_t i = 0; i < plaintext.size(); i++) {
        plaintext[i] = rand() & 0xFF;
    }
    for (int i = 0; i < 32; i++) {
        key[i] = rand() & 0xFF;
    }
    for (int i = 0; i < 12; i++) {
        nonce[i] = rand() & 0xFF;
    }
    
    std::cout << "Running " << iterations << " iterations of " << max_blocks 
              << " blocks each (" << (max_blocks * CHACHA20_BLOCK_SIZE / 1024) << " KB per iteration)..." << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; i++) {
        chacha20.encrypt(plaintext.data(), key, nonce, counter, ciphertext.data(), max_blocks);
        if ((i + 1) % 10 == 0) {
            std::cout << "Completed " << (i + 1) << "/" << iterations << " iterations" << std::endl;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    double total_data_mb = (double)(iterations * max_blocks * CHACHA20_BLOCK_SIZE) / (1024.0 * 1024.0);
    double time_sec = (double)duration.count() / 1000.0;
    double avg_throughput = total_data_mb / time_sec;
    
    std::cout << "✓ Stress test completed!" << std::endl;
    std::cout << "Total data processed: " << std::fixed << std::setprecision(2) 
              << total_data_mb << " MB" << std::endl;
    std::cout << "Total time: " << std::setprecision(3) << time_sec << " seconds" << std::endl;
    std::cout << "Average throughput: " << std::setprecision(2) << avg_throughput << " MB/s" << std::endl;
}

void runStreamingTest(ChaCha20Host& chacha20) {
    std::cout << "\n=== Streaming Test (Multiple Counters) ===" << std::endl;
    
    uint8_t key[32];
    uint8_t nonce[12];
    for (int i = 0; i < 32; i++) key[i] = i;
    for (int i = 0; i < 12; i++) nonce[i] = i;
    
    const int blocks_per_stream = 4;
    const int num_streams = 8;
    
    std::vector<uint8_t> plaintext(blocks_per_stream * CHACHA20_BLOCK_SIZE);
    std::vector<uint8_t> ciphertext(blocks_per_stream * CHACHA20_BLOCK_SIZE);
    
    // Initialize plaintext
    for (size_t i = 0; i < plaintext.size(); i++) {
        plaintext[i] = (uint8_t)(i % 256);
    }
    
    std::cout << "Encrypting " << num_streams << " streams of " << blocks_per_stream 
              << " blocks each with different counters..." << std::endl;
    
    for (int stream = 0; stream < num_streams; stream++) {
        uint32_t counter = stream + 1;
        
        std::cout << "Stream " << (stream + 1) << " (counter=" << counter << "): ";
        chacha20.encrypt(plaintext.data(), key, nonce, counter, ciphertext.data(), blocks_per_stream);
        
        // Print first 16 bytes of ciphertext for verification
        for (int i = 0; i < 16; i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)ciphertext[i];
            if (i < 15) std::cout << " ";
        }
        std::cout << std::dec << "..." << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <xclbin_path> [device_id]" << std::endl;
        std::cerr << "Example: " << argv[0] << " chacha20_encrypt.xclbin 0" << std::endl;
        return 1;
    }
    
    std::string xclbin_path = argv[1];
    int device_id = (argc > 2) ? std::atoi(argv[2]) : 0;
    
    try {
        std::cout << "=== ChaCha20 Hardware Accelerator Host Application ===" << std::endl;
        std::cout << "XCLBIN: " << xclbin_path << std::endl;
        std::cout << "Device ID: " << device_id << std::endl;
        std::cout << "ChaCha20 Block Size: " << CHACHA20_BLOCK_SIZE << " bytes" << std::endl;
        std::cout << "ChaCha20 Key Size: " << CHACHA20_KEY_SIZE << " bytes" << std::endl;
        std::cout << "ChaCha20 Nonce Size: " << CHACHA20_NONCE_SIZE << " bytes" << std::endl;
        
        // Initialize ChaCha20 accelerator
        ChaCha20Host chacha20(xclbin_path, device_id);
        
        // Allocate buffers for maximum test size
        chacha20.allocateBuffers(512);
        
        // Run tests
        runTestVectors(chacha20);
        runPerformanceTest(chacha20);
        runStreamingTest(chacha20);
        runStressTest(chacha20);
        
        std::cout << "\n=== All ChaCha20 tests completed successfully! ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Application failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}