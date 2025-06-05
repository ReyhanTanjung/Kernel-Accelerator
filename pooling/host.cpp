#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <string>
#include <memory>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <iomanip>
#include <random>

// XRT includes
#include "xrt/xrt_bo.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_kernel.h"
#include "experimental/xrt_profile.h"

// Include our kernel header
#include "pooling.h"

// Test configuration parameters
#define TEST_HEIGHT 64
#define TEST_WIDTH 64
#define TEST_CHANNELS 16
#define TEST_POOL_SIZE 2
#define TEST_POOL_STRIDE 2

// Calculate output dimensions
#define TEST_OUT_HEIGHT ((TEST_HEIGHT - TEST_POOL_SIZE) / TEST_POOL_STRIDE + 1)
#define TEST_OUT_WIDTH ((TEST_WIDTH - TEST_POOL_SIZE) / TEST_POOL_STRIDE + 1)

// Function to perform max pooling on CPU (for verification)
void max_pooling_cpu(const float* input, float* output, 
                     int height, int width, int channels,
                     int pool_size, int pool_stride) {
    int out_height = (height - pool_size) / pool_stride + 1;
    int out_width = (width - pool_size) / pool_stride + 1;
    
    for (int c = 0; c < channels; c++) {
        for (int h = 0; h < out_height; h++) {
            for (int w = 0; w < out_width; w++) {
                float max_val = -std::numeric_limits<float>::max();
                
                for (int ph = 0; ph < pool_size; ph++) {
                    for (int pw = 0; pw < pool_size; pw++) {
                        int in_row = h * pool_stride + ph;
                        int in_col = w * pool_stride + pw;
                        
                        if (in_row < height && in_col < width) {
                            int in_idx = c * height * width + in_row * width + in_col;
                            max_val = std::max(max_val, input[in_idx]);
                        }
                    }
                }
                
                int out_idx = c * out_height * out_width + h * out_width + w;
                output[out_idx] = max_val;
            }
        }
    }
}

// Function to perform average pooling on CPU (for verification)
void avg_pooling_cpu(const float* input, float* output, 
                     int height, int width, int channels,
                     int pool_size, int pool_stride) {
    int out_height = (height - pool_size) / pool_stride + 1;
    int out_width = (width - pool_size) / pool_stride + 1;
    
    for (int c = 0; c < channels; c++) {
        for (int h = 0; h < out_height; h++) {
            for (int w = 0; w < out_width; w++) {
                float sum = 0.0f;
                int count = 0;
                
                for (int ph = 0; ph < pool_size; ph++) {
                    for (int pw = 0; pw < pool_size; pw++) {
                        int in_row = h * pool_stride + ph;
                        int in_col = w * pool_stride + pw;
                        
                        if (in_row < height && in_col < width) {
                            int in_idx = c * height * width + in_row * width + in_col;
                            sum += input[in_idx];
                            count++;
                        }
                    }
                }
                
                int out_idx = c * out_height * out_width + h * out_width + w;
                output[out_idx] = sum / count;
            }
        }
    }
}

// Function to print a feature map slice (for debugging)
void print_feature_map(const float* data, int height, int width, int channels, 
                      int channel = 0, int max_h = 8, int max_w = 8) {
    std::cout << "Feature map (channel " << channel << ", first " 
              << max_h << "x" << max_w << " elements):" << std::endl;
    
    for (int h = 0; h < std::min(height, max_h); h++) {
        for (int w = 0; w < std::min(width, max_w); w++) {
            int idx = channel * height * width + h * width + w;
            std::cout << std::fixed << std::setprecision(2) << std::setw(6) << data[idx] << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

// Function to verify results
bool verify_results(const float* fpga_output, const float* cpu_output, 
                   int size, float tolerance = 1e-4) {
    bool match = true;
    int error_count = 0;
    float max_diff = 0.0f;
    
    for (int i = 0; i < size; i++) {
        float diff = std::abs(fpga_output[i] - cpu_output[i]);
        max_diff = std::max(max_diff, diff);
        
        if (diff > tolerance) {
            if (error_count < 10) {
                std::cout << "Error at index " << i << ": FPGA=" << fpga_output[i] 
                          << ", CPU=" << cpu_output[i] << ", diff=" << diff << std::endl;
            }
            error_count++;
            match = false;
        }
    }
    
    if (match) {
        std::cout << "Verification PASSED! Max difference: " << max_diff << std::endl;
    } else {
        std::cout << "Verification FAILED with " << error_count 
                  << " errors out of " << size << " elements." << std::endl;
        std::cout << "Max difference: " << max_diff << std::endl;
    }
    
    return match;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <xclbin_file>" << std::endl;
        return 1;
    }

    std::string xclbin_file = argv[1];
    std::cout << "Using XCLBIN file: " << xclbin_file << std::endl;
    
    // Configuration parameters
    const int height = TEST_HEIGHT;
    const int width = TEST_WIDTH;
    const int channels = TEST_CHANNELS;
    const int pool_size = TEST_POOL_SIZE;
    const int pool_stride = TEST_POOL_STRIDE;
    
    // Calculate output dimensions
    const int out_height = (height - pool_size) / pool_stride + 1;
    const int out_width = (width - pool_size) / pool_stride + 1;
    
    // Calculate buffer sizes
    const size_t in_size = channels * height * width;
    const size_t out_size = channels * out_height * out_width;
    const size_t in_bytes = in_size * sizeof(float);
    const size_t out_bytes = out_size * sizeof(float);
    
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Input: " << height << "x" << width << "x" << channels << std::endl;
    std::cout << "  Output: " << out_height << "x" << out_width << "x" << channels << std::endl;
    std::cout << "  Pool size: " << pool_size << "x" << pool_size << std::endl;
    std::cout << "  Pool stride: " << pool_stride << std::endl;
    
    // Allocate host buffers
    std::vector<float> input_data(in_size);
    std::vector<float> fpga_output_max(out_size);
    std::vector<float> fpga_output_avg(out_size);
    std::vector<float> cpu_output_max(out_size);
    std::vector<float> cpu_output_avg(out_size);
    
    // Initialize input data with random values
    std::cout << "Initializing input data..." << std::endl;
    std::mt19937 gen(42); // Fixed seed for reproducibility
    std::uniform_real_distribution<float> dis(-10.0f, 10.0f);
    
    for (size_t i = 0; i < in_size; i++) {
        input_data[i] = dis(gen);
    }
    
    // Print a slice of input data for debugging
    print_feature_map(input_data.data(), height, width, channels);
    
    try {
        // Load XRT runtime and device
        std::cout << "Initializing XRT runtime..." << std::endl;
        auto device = xrt::device(0); // Use first device
        auto uuid = device.load_xclbin(xclbin_file);
        
        // Create kernel instance
        std::cout << "Creating kernel..." << std::endl;
        auto kernel = xrt::kernel(device, uuid, "pooling");
        
        // Allocate device buffers
        std::cout << "Allocating device buffers..." << std::endl;
        auto input_buf = xrt::bo(device, in_bytes, kernel.group_id(0));
        auto output_buf = xrt::bo(device, out_bytes, kernel.group_id(1));
        
        // Copy input data to device
        std::cout << "Copying input data to device..." << std::endl;
        input_buf.write(input_data.data());
        input_buf.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        
        // Run MAX pooling kernel
        std::cout << "Running MAX pooling kernel..." << std::endl;
        auto pool_type = POOL_MAX;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        auto run = kernel(input_buf, output_buf, height, width, channels, 
                         pool_size, pool_stride, static_cast<int>(pool_type));
        run.wait();
        
        auto end = std::chrono::high_resolution_clock::now();
        double max_duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        // Get MAX pooling results
        output_buf.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        output_buf.read(fpga_output_max.data());
        
        // Run AVG pooling kernel
        std::cout << "Running AVG pooling kernel..." << std::endl;
        pool_type = POOL_AVG;
        
        start = std::chrono::high_resolution_clock::now();
        
        run = kernel(input_buf, output_buf, height, width, channels, 
                    pool_size, pool_stride, static_cast<int>(pool_type));
        run.wait();
        
        end = std::chrono::high_resolution_clock::now();
        double avg_duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        // Get AVG pooling results
        output_buf.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        output_buf.read(fpga_output_avg.data());
        
        // Run CPU reference implementations
        std::cout << "Running CPU reference implementations..." << std::endl;
        
        auto cpu_start = std::chrono::high_resolution_clock::now();
        max_pooling_cpu(input_data.data(), cpu_output_max.data(), 
                        height, width, channels, pool_size, pool_stride);
        auto cpu_end = std::chrono::high_resolution_clock::now();
        double cpu_max_duration_ms = std::chrono::duration<double, std::milli>(cpu_end - cpu_start).count();
        
        cpu_start = std::chrono::high_resolution_clock::now();
        avg_pooling_cpu(input_data.data(), cpu_output_avg.data(), 
                       height, width, channels, pool_size, pool_stride);
        cpu_end = std::chrono::high_resolution_clock::now();
        double cpu_avg_duration_ms = std::chrono::duration<double, std::milli>(cpu_end - cpu_start).count();
        
        // Print output data slices for debugging
        std::cout << "MAX Pooling Results:" << std::endl;
        print_feature_map(fpga_output_max.data(), out_height, out_width, channels);
        
        std::cout << "AVG Pooling Results:" << std::endl;
        print_feature_map(fpga_output_avg.data(), out_height, out_width, channels);
        
        // Verify results
        std::cout << "Verifying MAX pooling results..." << std::endl;
        bool max_verified = verify_results(fpga_output_max.data(), cpu_output_max.data(), out_size);
        
        std::cout << "Verifying AVG pooling results..." << std::endl;
        bool avg_verified = verify_results(fpga_output_avg.data(), cpu_output_avg.data(), out_size);
        
        // Print performance metrics
        std::cout << std::endl << "Performance Metrics:" << std::endl;
        std::cout << "------------------------------------------------------" << std::endl;
        std::cout << "MAX Pooling FPGA: " << max_duration_ms << " ms" << std::endl;
        std::cout << "MAX Pooling CPU:  " << cpu_max_duration_ms << " ms" << std::endl;
        std::cout << "MAX Pooling Speedup: " << cpu_max_duration_ms / max_duration_ms << "x" << std::endl;
        std::cout << std::endl;
        std::cout << "AVG Pooling FPGA: " << avg_duration_ms << " ms" << std::endl;
        std::cout << "AVG Pooling CPU:  " << cpu_avg_duration_ms << " ms" << std::endl;
        std::cout << "AVG Pooling Speedup: " << cpu_avg_duration_ms / avg_duration_ms << "x" << std::endl;
        std::cout << "------------------------------------------------------" << std::endl;
        
        // Calculate throughput
        double input_gb = in_bytes / (1024.0 * 1024.0 * 1024.0);
        double output_gb = out_bytes / (1024.0 * 1024.0 * 1024.0);
        double total_gb = input_gb + output_gb;
        
        double max_throughput = total_gb / (max_duration_ms / 1000.0);
        double avg_throughput = total_gb / (avg_duration_ms / 1000.0);
        
        std::cout << "MAX Pooling Throughput: " << max_throughput << " GB/s" << std::endl;
        std::cout << "AVG Pooling Throughput: " << avg_throughput << " GB/s" << std::endl;
        
        // Return success if verification passed
        return (max_verified && avg_verified) ? 0 : 1;
        
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }
}