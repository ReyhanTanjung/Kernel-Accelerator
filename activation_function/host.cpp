#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <xrt.h>
#include <experimental/xrt_kernel.h>

#define SIZE 1024 * 1024  // 1 million elements

// Software reference functions for verification
float sw_relu(float x) {
    return (x > 0) ? x : 0;
}

float sw_sigmoid(float x) {
    return 1.0f / (1.0f + exp(-x));
}

float sw_tanh(float x) {
    return tanh(x);
}

int main() {
    // Initialize data
    std::vector<float> input(SIZE), output(SIZE), expected(SIZE);
    
    // Create test data with a range of values
    for (int i = 0; i < SIZE; i++) {
        input[i] = (float)(i % 4096) / 256.0f - 8.0f;  // Range from -8 to 8
    }
    
    const char* function_names[] = {"ReLU", "Sigmoid", "Tanh"};
    
    // Setup XRT device and kernel
    try {
        auto device = xrt::device(0);
        auto uuid = device.load_xclbin("activation_kernel.xclbin");
        auto kernel = xrt::kernel(device, uuid, "activation_kernel", xrt::kernel::cu_access_mode::exclusive);
        
        // Test each activation function
        for (int function_type = 0; function_type < 3; function_type++) {
            std::cout << "\n---------- Testing " << function_names[function_type] << " ----------" << std::endl;
            
            // Calculate expected outputs for verification
            for (int i = 0; i < SIZE; i++) {
                switch (function_type) {
                    case 0: // ReLU
                        expected[i] = sw_relu(input[i]);
                        break;
                    case 1: // Sigmoid
                        expected[i] = sw_sigmoid(input[i]);
                        break;
                    case 2: // Tanh
                        expected[i] = sw_tanh(input[i]);
                        break;
                }
            }
            
            // Use buffers for input and output data
            auto input_buf = xrt::bo(device, SIZE * sizeof(float), kernel.group_id(0));
            auto output_buf = xrt::bo(device, SIZE * sizeof(float), kernel.group_id(1));
            
            // Map the buffer objects to host memory
            auto input_map = input_buf.map<float*>();
            auto output_map = output_buf.map<float*>();
            
            // Copy data to the input buffer
            for (int i = 0; i < SIZE; i++) {
                input_map[i] = input[i];
            }
            
            // Sync input buffer to device memory
            std::cout << "Syncing input buffer to device memory..." << std::endl;
            input_buf.sync(XCL_BO_SYNC_BO_TO_DEVICE);
            
            // Run CPU version for comparison
            std::cout << "Running CPU version..." << std::endl;
            auto cpu_start = std::chrono::high_resolution_clock::now();
            
            for (int i = 0; i < SIZE; i++) {
                switch (function_type) {
                    case 0: // ReLU
                        output[i] = sw_relu(input[i]);
                        break;
                    case 1: // Sigmoid
                        output[i] = sw_sigmoid(input[i]);
                        break;
                    case 2: // Tanh
                        output[i] = sw_tanh(input[i]);
                        break;
                }
            }
            
            auto cpu_end = std::chrono::high_resolution_clock::now();
            double cpu_ms = std::chrono::duration<double, std::milli>(cpu_end - cpu_start).count();
            
            // Run FPGA kernel
            std::cout << "Running FPGA kernel..." << std::endl;
            auto fpga_start = std::chrono::high_resolution_clock::now();
            
            auto run = kernel(input_buf, output_buf, SIZE, function_type);
            run.wait();
            
            auto fpga_end = std::chrono::high_resolution_clock::now();
            double fpga_ms = std::chrono::duration<double, std::milli>(fpga_end - fpga_start).count();
            
            // Get results from device
            std::cout << "Getting results from device..." << std::endl;
            output_buf.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
            
            // Verify results
            bool pass = true;
            int error_count = 0;
            float max_error = 0.0f;
            float tolerance = 0.01f;
            
            for (int i = 0; i < SIZE; i++) {
                float hw_result = output_map[i];
                float sw_result = expected[i];
                float error = fabs(hw_result - sw_result);
                max_error = std::max(max_error, error);
                
                if (error > tolerance) {
                    error_count++;
                    if (error_count <= 10) {
                        std::cout << "Error at index " << i << ": hw=" << hw_result 
                                << ", sw=" << sw_result << ", error=" << error << std::endl;
                    }
                }
            }
            
            if (error_count > 0) {
                std::cout << "Total errors: " << error_count << " (out of " << SIZE << ")" << std::endl;
                std::cout << "Max error: " << max_error << std::endl;
                std::cout << "Verification FAILED!" << std::endl;
            } else {
                std::cout << "Verification PASSED!" << std::endl;
            }
            
            // Report performance
            double size_gb = (double)(SIZE * sizeof(float) * 2) / (1024 * 1024 * 1024); // Input + output in GB
            double fpga_throughput = size_gb / (fpga_ms / 1000.0);
            double cpu_throughput = size_gb / (cpu_ms / 1000.0);
            
            std::cout << "\nPerformance Summary for " << function_names[function_type] << ":" << std::endl;
            std::cout << "Data size: " << size_gb << " GB" << std::endl;
            std::cout << "CPU time: " << cpu_ms << " ms" << std::endl;
            std::cout << "FPGA time: " << fpga_ms << " ms" << std::endl;
            std::cout << "CPU throughput: " << cpu_throughput << " GB/s" << std::endl;
            std::cout << "FPGA throughput: " << fpga_throughput << " GB/s" << std::endl;
            std::cout << "Speedup: " << cpu_ms / fpga_ms << "x" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}