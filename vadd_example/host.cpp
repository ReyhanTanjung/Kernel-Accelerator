#include <iostream>
#include <vector>
#include <xrt.h>
#include <experimental/xrt_kernel.h>
#include <chrono>

#define SIZE 1024 * 1024  // 1 million elements

int main() {
    // Initialize data
    std::vector<int> a(SIZE), b(SIZE), c(SIZE), c_golden(SIZE);

    // Create test data
    for (int i = 0; i < SIZE; i++) {
        a[i] = i;
        b[i] = i * 2;
        c_golden[i] = a[i] + b[i]; // For verification
    }

    // Setup XRT device and kernel
    auto device = xrt::device(0);
    auto uuid = device.load_xclbin("vadd_hw.xclbin");
    auto kernel = xrt::kernel(device, uuid, "vadd", xrt::kernel::cu_access_mode::exclusive);

    // Use separate buffers for each data stream to enable better memory parallelism
    auto a_buf = xrt::bo(device, SIZE * sizeof(int), kernel.group_id(0));
    auto b_buf = xrt::bo(device, SIZE * sizeof(int), kernel.group_id(1));
    auto c_buf = xrt::bo(device, SIZE * sizeof(int), kernel.group_id(2));

    // Map the buffer objects into host memory for easy access
    auto a_map = a_buf.map<int*>();
    auto b_map = b_buf.map<int*>();
    auto c_map = c_buf.map<int*>();

    // Copy data to mapped memory
    for (int i = 0; i < SIZE; i++) {
        a_map[i] = a[i];
        b_map[i] = b[i];
    }

    // Synchronize buffer content with device memory
    std::cout << "Syncing input buffers to device memory...\n";
    a_buf.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    b_buf.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    std::cout << "Starting kernel execution...\n";
    auto start = std::chrono::high_resolution_clock::now();

    // Execute the kernel once
    auto run = kernel(a_buf, b_buf, c_buf, SIZE);
    run.wait();

    auto end = std::chrono::high_resolution_clock::now();
    double duration_ms = std::chrono::duration<double, std::milli>(end - start).count();

    // Get the output data
    std::cout << "Getting results from device...\n";
    c_buf.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

    // Verify results
    bool error = false;
    for (int i = 0; i < SIZE; i++) {
        if (c_map[i] != c_golden[i]) {
            std::cout << "Error at index " << i << ": " << c_map[i] << " != " << c_golden[i] << std::endl;
            error = true;
            if (i > 10) break; // Only show first few errors
        }
    }

    if (!error) {
        std::cout << "Verification PASSED!\n";
    } else {
        std::cout << "Verification FAILED!\n";
    }

    std::cout << "Kernel execution time: " << duration_ms << " ms\n";
    
    // Calculate throughput
    double size_gb = (double)(SIZE * sizeof(int) * 3) / (1024 * 1024 * 1024); // 3 arrays in GB
    double throughput_gbps = size_gb / (duration_ms / 1000.0);
    
    std::cout << "Data size: " << size_gb << " GB\n";
    std::cout << "Throughput: " << throughput_gbps << " GB/s\n";

    return 0;
}
