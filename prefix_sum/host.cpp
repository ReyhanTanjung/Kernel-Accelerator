#include <iostream>
#include <vector>
#include <chrono>
#include <xrt.h>
#include <experimental/xrt_kernel.h>

#define SIZE 1024 * 1024  // 1 million elements

int main() {
    std::cout << "=== Prefix Sum FPGA Accelerator Test ===" << std::endl;
    
    // Initialize data
    std::vector<int> input(SIZE), output(SIZE), golden(SIZE);

    // Create test data - simple ascending sequence
    std::cout << "Initializing input data..." << std::endl;
    for (int i = 0; i < SIZE; i++) {
        input[i] = (i % 100) + 1;  // Values 1-100 repeating to avoid overflow
    }

    // Compute golden reference for verification
    std::cout << "Computing golden reference..." << std::endl;
    golden[0] = input[0];
    for (int i = 1; i < SIZE; i++) {
        golden[i] = golden[i-1] + input[i];
    }

    try {
        // Setup XRT device and kernel
        std::cout << "Setting up FPGA device..." << std::endl;
        auto device = xrt::device(0);
        auto uuid = device.load_xclbin("prefix_sum_hw.xclbin");
        auto kernel = xrt::kernel(device, uuid, "prefix_sum", xrt::kernel::cu_access_mode::exclusive);

        // Create buffer objects - separate memory banks for better performance
        std::cout << "Creating buffer objects..." << std::endl;
        auto input_buf = xrt::bo(device, SIZE * sizeof(int), kernel.group_id(0));
        auto output_buf = xrt::bo(device, SIZE * sizeof(int), kernel.group_id(1));

        // Map the buffer objects into host memory
        auto input_map = input_buf.map<int*>();
        auto output_map = output_buf.map<int*>();

        // Copy data to mapped memory
        std::cout << "Copying input data to device memory..." << std::endl;
        for (int i = 0; i < SIZE; i++) {
            input_map[i] = input[i];
        }

        // Synchronize input buffer with device memory
        input_buf.sync(XCL_BO_SYNC_BO_TO_DEVICE);

        std::cout << "Starting kernel execution..." << std::endl;
        auto start = std::chrono::high_resolution_clock::now();

        // Execute the kernel
        auto run = kernel(input_buf, output_buf, SIZE);
        run.wait();

        auto end = std::chrono::high_resolution_clock::now();
        double duration_ms = std::chrono::duration<double, std::milli>(end - start).count();

        // Get the output data
        std::cout << "Retrieving results from device..." << std::endl;
        output_buf.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

        // Copy results back to host vector
        for (int i = 0; i < SIZE; i++) {
            output[i] = output_map[i];
        }

        // Verify results
        std::cout << "Verifying results..." << std::endl;
        bool error = false;
        int error_count = 0;
        
        for (int i = 0; i < SIZE; i++) {
            if (output[i] != golden[i]) {
                if (error_count < 10) {  // Only show first few errors
                    std::cout << "Error at index " << i << ": " 
                              << output[i] << " != " << golden[i] << std::endl;
                }
                error = true;
                error_count++;
            }
        }

        if (!error) {
            std::cout << "✅ Verification PASSED!" << std::endl;
        } else {
            std::cout << "❌ Verification FAILED!" << std::endl;
            std::cout << "Total errors: " << error_count << std::endl;
        }

        // Performance metrics
        std::cout << "\n=== Performance Results ===" << std::endl;
        std::cout << "Array size: " << SIZE << " elements" << std::endl;
        std::cout << "Kernel execution time: " << duration_ms << " ms" << std::endl;
        
        // Calculate throughput (reading input + writing output)
        double data_size_gb = (double)(SIZE * sizeof(int) * 2) / (1024 * 1024 * 1024);
        double throughput_gbps = data_size_gb / (duration_ms / 1000.0);
        
        std::cout << "Data processed: " << data_size_gb << " GB" << std::endl;
        std::cout << "Memory throughput: " << throughput_gbps << " GB/s" << std::endl;
        
        // Calculate operations per second
        double ops_per_sec = (SIZE - 1) / (duration_ms / 1000.0);  // SIZE-1 additions
        std::cout << "Operations per second: " << ops_per_sec / 1e6 << " MOPS" << std::endl;

        // Sample output display
        std::cout << "\n=== Sample Results ===" << std::endl;
        std::cout << "First 10 elements:" << std::endl;
        std::cout << "Input:  ";
        for (int i = 0; i < 10; i++) {
            std::cout << input[i] << " ";
        }
        std::cout << std::endl;
        
        std::cout << "Output: ";
        for (int i = 0; i < 10; i++) {
            std::cout << output[i] << " ";
        }
        std::cout << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}