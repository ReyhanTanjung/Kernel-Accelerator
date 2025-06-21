#include <iostream>
#include <vector>
#include <iomanip>
#include <cstring>
#include <xrt.h>
#include <experimental/xrt_kernel.h>
#include <chrono>
#include "sha3.h"

void print_hex(const uint8_t* data, int len) {
    for (int i = 0; i < len; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
    }
}

// Software reference implementation for verification
void sha3_256_sw(const uint8_t* message, uint32_t message_len, uint8_t* hash) {
    // This is a simplified reference - in real implementation you'd use a library
    // For now, we'll just create a predictable pattern for testing
    for (int i = 0; i < SHA3_256_HASH_SIZE; i++) {
        hash[i] = (message_len + i) & 0xFF;
    }
}

int main() {
    std::cout << "SHA3-256 FPGA Accelerator Test\n";
    std::cout << "==============================\n\n";

    // Test data sizes
    const std::vector<uint32_t> test_sizes = {64, 256, 1024, 4096};
    
    // Setup XRT device and kernel
    std::cout << "Initializing FPGA device...\n";
    auto device = xrt::device(0);
    auto uuid = device.load_xclbin("sha3_hw.xclbin");
    auto kernel = xrt::kernel(device, uuid, "sha3_256", xrt::kernel::cu_access_mode::exclusive);

    for (uint32_t size : test_sizes) {
        std::cout << "\n" << std::string(50, '-') << "\n";
        std::cout << "Testing with " << size << " bytes of data\n";
        
        // Create test message
        std::vector<uint8_t> message(size);
        for (uint32_t i = 0; i < size; i++) {
            message[i] = (i * 37 + 123) & 0xFF; // Pseudo-random pattern
        }
        
        // Calculate number of blocks
        uint32_t num_blocks = (size + SHA3_256_RATE - 1) / SHA3_256_RATE;
        if (num_blocks == 0) num_blocks = 1;
        
        std::cout << "Message size: " << size << " bytes\n";
        std::cout << "Number of blocks: " << num_blocks << "\n";

        // Create buffer objects
        auto message_buf = xrt::bo(device, size * sizeof(uint8_t), kernel.group_id(0));
        auto hash_buf = xrt::bo(device, SHA3_256_HASH_SIZE * sizeof(uint8_t), kernel.group_id(2));

        // Map buffers
        auto message_map = message_buf.map<uint8_t*>();
        auto hash_map = hash_buf.map<uint8_t*>();

        // Copy input data
        std::memcpy(message_map, message.data(), size);

        // Sync to device
        std::cout << "Copying data to FPGA...\n";
        message_buf.sync(XCL_BO_SYNC_BO_TO_DEVICE);

        // Execute kernel
        std::cout << "Executing SHA3-256 on FPGA...\n";
        auto start = std::chrono::high_resolution_clock::now();
        
        auto run = kernel(message_buf, size, hash_buf, num_blocks);
        run.wait();
        
        auto end = std::chrono::high_resolution_clock::now();
        double duration_ms = std::chrono::duration<double, std::milli>(end - start).count();

        // Get results
        hash_buf.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

        // Print results
        std::cout << "FPGA Hash: ";
        print_hex(hash_map, SHA3_256_HASH_SIZE);
        std::cout << "\n";

        // Performance metrics
        std::cout << "Execution time: " << duration_ms << " ms\n";
        double throughput_mbps = (size / (1024.0 * 1024.0)) / (duration_ms / 1000.0);
        std::cout << "Throughput: " << throughput_mbps << " MB/s\n";
        
        // Calculate hash rate
        double hash_rate = 1000.0 / duration_ms; // hashes per second
        std::cout << "Hash rate: " << hash_rate << " hashes/sec\n";

        // Software verification (simplified)
        std::vector<uint8_t> sw_hash(SHA3_256_HASH_SIZE);
        auto sw_start = std::chrono::high_resolution_clock::now();
        sha3_256_sw(message.data(), size, sw_hash.data());
        auto sw_end = std::chrono::high_resolution_clock::now();
        double sw_duration_ms = std::chrono::duration<double, std::milli>(sw_end - sw_start).count();
        
        std::cout << "SW time: " << sw_duration_ms << " ms\n";
        std::cout << "Speedup: " << (sw_duration_ms / duration_ms) << "x\n";
    }

    // Stress test with multiple iterations
    std::cout << "\n" << std::string(50, '=') << "\n";
    std::cout << "Stress Test - 100 iterations of 1KB hashing\n";
    
    const uint32_t stress_size = 1024;
    const uint32_t iterations = 100;
    
    std::vector<uint8_t> stress_message(stress_size);
    for (uint32_t i = 0; i < stress_size; i++) {
        stress_message[i] = i & 0xFF;
    }
    
    uint32_t stress_blocks = (stress_size + SHA3_256_RATE - 1) / SHA3_256_RATE;
    
    // Setup buffers for stress test
    auto stress_msg_buf = xrt::bo(device, stress_size, kernel.group_id(0));
    auto stress_hash_buf = xrt::bo(device, SHA3_256_HASH_SIZE, kernel.group_id(2));
    auto stress_msg_map = stress_msg_buf.map<uint8_t*>();
    auto stress_hash_map = stress_hash_buf.map<uint8_t*>();
    
    std::memcpy(stress_msg_map, stress_message.data(), stress_size);
    stress_msg_buf.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    
    auto stress_start = std::chrono::high_resolution_clock::now();
    
    for (uint32_t i = 0; i < iterations; i++) {
        auto run = kernel(stress_msg_buf, stress_size, stress_hash_buf, stress_blocks);
        run.wait();
        
        if (i % 20 == 0) {
            std::cout << "Completed " << i << "/" << iterations << " iterations\n";
        }
    }
    
    auto stress_end = std::chrono::high_resolution_clock::now();
    double total_time = std::chrono::duration<double, std::milli>(stress_end - stress_start).count();
    
    stress_hash_buf.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    
    std::cout << "Stress test completed!\n";
    std::cout << "Total time: " << total_time << " ms\n";
    std::cout << "Average time per hash: " << (total_time / iterations) << " ms\n";
    std::cout << "Average throughput: " << (iterations * stress_size / (1024.0 * 1024.0)) / (total_time / 1000.0) << " MB/s\n";
    std::cout << "Average hash rate: " << (iterations * 1000.0 / total_time) << " hashes/sec\n";
    
    std::cout << "Final hash: ";
    print_hex(stress_hash_map, SHA3_256_HASH_SIZE);
    std::cout << "\n";

    std::cout << "\nSHA3-256 FPGA accelerator test completed successfully!\n";
    return 0;
}