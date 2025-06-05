#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <fstream>

// XRT includes for Xilinx Runtime
#include "xrt/xrt_bo.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_kernel.h"

#define SHA256_BLOCK_SIZE 64
#define SHA256_DIGEST_SIZE 32

class SHA256Host {
private:
    xrt::device device;
    xrt::kernel kernel;
    xrt::bo bo_input, bo_output;
    
    void pad_message(const uint8_t* message, size_t msg_len, std::vector<uint8_t>& padded, int& num_blocks) {
        // Calculate padding
        size_t pad_len = 64 - ((msg_len + 9) % 64);
        if (pad_len == 64) pad_len = 0;
        size_t total_len = msg_len + 1 + pad_len + 8;
        
        num_blocks = total_len / 64;
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
    SHA256Host(const std::string& xclbin_path, int device_id = 0) {
        try {
            // Initialize device
            device = xrt::device(device_id);
            auto uuid = device.load_xclbin(xclbin_path);
            
            // Create kernel
            kernel = xrt::kernel(device, uuid, "sha256_hash");
            
            std::cout << "✓ SHA-256 Hardware accelerator initialized successfully" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error initializing SHA-256 accelerator: " << e.what() << std::endl;
            throw;
        }
    }
    
    void allocateBuffers(int max_blocks) {
        size_t input_size = max_blocks * SHA256_BLOCK_SIZE;
        size_t output_size = SHA256_DIGEST_SIZE;
        
        try {
            // Allocate buffer objects
            bo_input = xrt::bo(device, input_size, kernel.group_id(0));
            bo_output = xrt::bo(device, output_size, kernel.group_id(1));
            
            std::cout << "✓ Buffers allocated for " << max_blocks << " blocks" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error allocating buffers: " << e.what() << std::endl;
            throw;
        }
    }
    
    void hash(const uint8_t* message, size_t msg_len, uint8_t* hash_out) {
        try {
            // Pad message
            std::vector<uint8_t> padded;
            int num_blocks;
            pad_message(message, msg_len, padded, num_blocks);
            
            // Map buffers and copy data
            auto input_map = bo_input.map<uint8_t*>();
            auto output_map = bo_output.map<uint8_t*>();
            
            std::memcpy(input_map, padded.data(), padded.size());
            
            // Sync buffers to device
            bo_input.sync(XCL_BO_SYNC_BO_TO_DEVICE);
            
            // Start timing
            auto start = std::chrono::high_resolution_clock::now();
            
            // Run kernel
            auto run = kernel(bo_input, bo_output, num_blocks);
            run.wait();
            
            // End timing
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            // Sync result back
            bo_output.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
            
            // Copy result
            std::memcpy(hash_out, output_map, SHA256_DIGEST_SIZE);
            
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
    
    ~SHA256Host() {
        std::cout << "✓ SHA-256 Host cleanup completed" << std::endl;
    }
};

void printHash(const std::string& label, const uint8_t* hash) {
    std::cout << label << ": ";
    for (int i = 0; i < SHA256_DIGEST_SIZE; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    std::cout << std::dec << std::endl;
}

void runTestVectors(SHA256Host& sha) {
    std::cout << "\n=== SHA-256 Test Vectors ===" << std::endl;
    
    // Test 1: Empty string
    {
        std::cout << "\nTest 1: Empty string" << std::endl;
        uint8_t hash[SHA256_DIGEST_SIZE];
        sha.hash((const uint8_t*)"", 0, hash);
        printHash("Hash", hash);
        std::cout << "Expected: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855" << std::endl;
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

void runPerformanceTest(SHA256Host& sha) {
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

void runStressTest(SHA256Host& sha) {
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

void runFileHashTest(SHA256Host& sha) {
    std::cout << "\n=== File Hash Test ===" << std::endl;
    
    // Create test file
    const char* filename = "test_file.bin";
    const size_t file_size = 10 * 1024 * 1024; // 10 MB
    
    std::cout << "Creating " << file_size / (1024 * 1024) << " MB test file..." << std::endl;
    
    std::ofstream file(filename, std::ios::binary);
    std::vector<uint8_t> buffer(1024 * 1024);
    
    for (size_t i = 0; i < file_size; i += buffer.size()) {
        for (size_t j = 0; j < buffer.size(); j++) {
            buffer[j] = rand() & 0xFF;
        }
        file.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
    }
    file.close();
    
    // Read and hash file
    std::cout << "Hashing file..." << std::endl;
    
    std::ifstream infile(filename, std::ios::binary | std::ios::ate);
    size_t actual_size = infile.tellg();
    infile.seekg(0);
    
    std::vector<uint8_t> file_data(actual_size);
    infile.read(reinterpret_cast<char*>(file_data.data()), actual_size);
    infile.close();
    
    uint8_t hash[SHA256_DIGEST_SIZE];
    sha.hash(file_data.data(), file_data.size(), hash);
    
    printHash("File hash", hash);
    
    // Cleanup
    std::remove(filename);
    std::cout << "✓ Test file removed" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <xclbin_path> [device_id]" << std::endl;
        std::cerr << "Example: " << argv[0] << " sha256_hash.xclbin 0" << std::endl;
        return 1;
    }
    
    std::string xclbin_path = argv[1];
    int device_id = (argc > 2) ? std::atoi(argv[2]) : 0;
    
    try {
        std::cout << "=== SHA-256 Hardware Accelerator Host Application ===" << std::endl;
        std::cout << "XCLBIN: " << xclbin_path << std::endl;
        std::cout << "Device ID: " << device_id << std::endl;
        
        // Initialize SHA-256 accelerator
        SHA256Host sha(xclbin_path, device_id);
        
        // Allocate buffers for maximum test size
        sha.allocateBuffers(2048); // Support up to 2048 blocks (~128KB)
        
        // Run tests
        runTestVectors(sha);
        runPerformanceTest(sha);
        runStressTest(sha);
        runFileHashTest(sha);
        
        std::cout << "\n=== All tests completed successfully! ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Application failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}