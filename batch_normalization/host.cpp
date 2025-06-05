#include <iostream>
#include <vector>
#include <xrt.h>
#include <experimental/xrt_kernel.h>
#include <chrono>
#include <cmath>  // Added cmath header for sqrt function

#define BATCH_SIZE 1024 * 1024  // 1 million elements
#define N 1024                  // Number of channels
#define EPSILON 0.00001f        // Small value for numerical stability

int main() {
    // Initialize data
    std::vector<float> input(BATCH_SIZE), gamma(N), beta(N), mean(N), variance(N);
    std::vector<float> output(BATCH_SIZE), output_golden(BATCH_SIZE);

    // Create test data
    for (int i = 0; i < N; i++) {
        gamma[i] = 1.0f;       // Scale factor
        beta[i] = 0.0f;        // Shift factor
        mean[i] = 5.0f;        // Example mean
        variance[i] = 4.0f;    // Example variance
    }

    for (int i = 0; i < BATCH_SIZE; i++) {
        input[i] = i % 20;  // Some test values
        int channel = i % N;
        output_golden[i] = gamma[channel] * (input[i] - mean[channel]) / sqrt(variance[channel] + EPSILON) + beta[channel];
    }

    // Setup XRT device and kernel
    auto device = xrt::device(0);
    auto uuid = device.load_xclbin("batchnorm_hw.xclbin");
    auto kernel = xrt::kernel(device, uuid, "batchnorm", xrt::kernel::cu_access_mode::exclusive);

    // Use separate buffers for each data stream
    auto input_buf = xrt::bo(device, BATCH_SIZE * sizeof(float), kernel.group_id(0));
    auto gamma_buf = xrt::bo(device, N * sizeof(float), kernel.group_id(1));
    auto beta_buf = xrt::bo(device, N * sizeof(float), kernel.group_id(2));
    auto mean_buf = xrt::bo(device, N * sizeof(float), kernel.group_id(3));
    auto variance_buf = xrt::bo(device, N * sizeof(float), kernel.group_id(4));
    auto output_buf = xrt::bo(device, BATCH_SIZE * sizeof(float), kernel.group_id(5));

    // Map the buffer objects into host memory
    auto input_map = input_buf.map<float*>();
    auto gamma_map = gamma_buf.map<float*>();
    auto beta_map = beta_buf.map<float*>();
    auto mean_map = mean_buf.map<float*>();
    auto variance_map = variance_buf.map<float*>();
    auto output_map = output_buf.map<float*>();

    // Copy data to mapped memory
    for (int i = 0; i < BATCH_SIZE; i++) {
        input_map[i] = input[i];
    }
    
    for (int i = 0; i < N; i++) {
        gamma_map[i] = gamma[i];
        beta_map[i] = beta[i];
        mean_map[i] = mean[i];
        variance_map[i] = variance[i];
    }

    // Synchronize buffer content with device memory
    std::cout << "Syncing input buffers to device memory...\n";
    input_buf.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    gamma_buf.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    beta_buf.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    mean_buf.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    variance_buf.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    std::cout << "Starting kernel execution...\n";
    auto start = std::chrono::high_resolution_clock::now();

    // Execute the kernel
    auto run = kernel(input_buf, gamma_buf, beta_buf, mean_buf, variance_buf, 
                     output_buf, BATCH_SIZE, EPSILON);
    run.wait();

    auto end = std::chrono::high_resolution_clock::now();
    double duration_ms = std::chrono::duration<double, std::milli>(end - start).count();

    // Get the output data
    std::cout << "Getting results from device...\n";
    output_buf.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

    // Verify results
    bool error = false;
    for (int i = 0; i < BATCH_SIZE; i++) {
        float diff = std::abs(output_map[i] - output_golden[i]);
        if (diff > 0.001f) {
            std::cout << "Error at index " << i << ": " << output_map[i] << " != " << output_golden[i] 
                      << ", diff = " << diff << std::endl;
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
    // Input + gamma + beta + mean + variance + output
    double size_gb = (double)((BATCH_SIZE + N * 4 + BATCH_SIZE) * sizeof(float)) / (1024 * 1024 * 1024);
    double throughput_gbps = size_gb / (duration_ms / 1000.0);
    
    std::cout << "Data size: " << size_gb << " GB\n";
    std::cout << "Throughput: " << throughput_gbps << " GB/s\n";

    return 0;
}