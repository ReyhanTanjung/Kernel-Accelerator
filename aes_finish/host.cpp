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

#define AES_BLOCK_SIZE 16
#define AES_KEY_SIZE 16

class AESHost {
private:
    xrt::device device;
    xrt::kernel kernel;
    xrt::bo bo_plaintext, bo_key, bo_ciphertext;
    
public:
    AESHost(const std::string& xclbin_path, int device_id = 0) {
        try {
            // Initialize device
            device = xrt::device(device_id);
            auto uuid = device.load_xclbin(xclbin_path);
            
            // Create kernel
            kernel = xrt::kernel(device, uuid, "aes_encrypt");
            
            std::cout << "✓ AES Hardware accelerator initialized successfully" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error initializing AES accelerator: " << e.what() << std::endl;
            throw;
        }
    }
    
    void allocateBuffers(int max_blocks) {
        size_t plaintext_size = max_blocks * AES_BLOCK_SIZE;
        size_t ciphertext_size = max_blocks * AES_BLOCK_SIZE;
        size_t key_size = AES_KEY_SIZE;
        
        try {
            // Allocate buffer objects
            bo_plaintext = xrt::bo(device, plaintext_size, kernel.group_id(0));
            bo_key = xrt::bo(device, key_size, kernel.group_id(1));
            bo_ciphertext = xrt::bo(device, ciphertext_size, kernel.group_id(2));
            
            std::cout << "✓ Buffers allocated for " << max_blocks << " blocks" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error allocating buffers: " << e.what() << std::endl;
            throw;
        }
    }
    
    void encrypt(const uint8_t* plaintext, const uint8_t* key, uint8_t* ciphertext, int num_blocks) {
        try {
            size_t plaintext_size = num_blocks * AES_BLOCK_SIZE;
            size_t ciphertext_size = num_blocks * AES_BLOCK_SIZE;
            
            // Map buffers and copy data
            auto plaintext_map = bo_plaintext.map<uint8_t*>();
            auto key_map = bo_key.map<uint8_t*>();
            auto ciphertext_map = bo_ciphertext.map<uint8_t*>();
            
            std::memcpy(plaintext_map, plaintext, plaintext_size);
            std::memcpy(key_map, key, AES_KEY_SIZE);
            
            // Sync buffers to device
            bo_plaintext.sync(XCL_BO_SYNC_BO_TO_DEVICE);
            bo_key.sync(XCL_BO_SYNC_BO_TO_DEVICE);
            
            // Start timing
            auto start = std::chrono::high_resolution_clock::now();
            
            // Run kernel
            auto run = kernel(bo_plaintext, bo_key, bo_ciphertext, num_blocks);
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
    
    ~AESHost() {
        std::cout << "✓ AES Host cleanup completed" << std::endl;
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

void runTestVectors(AESHost& aes) {
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
}

void runPerformanceTest(AESHost& aes) {
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

void runStressTest(AESHost& aes) {
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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <xclbin_path> [device_id]" << std::endl;
        std::cerr << "Example: " << argv[0] << " aes_encrypt.xclbin 0" << std::endl;
        return 1;
    }
    
    std::string xclbin_path = argv[1];
    int device_id = (argc > 2) ? std::atoi(argv[2]) : 0;
    
    try {
        std::cout << "=== AES Hardware Accelerator Host Application ===" << std::endl;
        std::cout << "XCLBIN: " << xclbin_path << std::endl;
        std::cout << "Device ID: " << device_id << std::endl;
        
        // Initialize AES accelerator
        AESHost aes(xclbin_path, device_id);
        
        // Allocate buffers for maximum test size
        aes.allocateBuffers(1024);
        
        // Run tests
        runTestVectors(aes);
        runPerformanceTest(aes);
        runStressTest(aes);
        
        std::cout << "\n=== All tests completed successfully! ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Application failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}