#include <iostream>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include "pooling.h"

// Test parameters
#define TEST_HEIGHT 8
#define TEST_WIDTH 8
#define TEST_CHANNELS 3
#define TEST_POOL_SIZE 2
#define TEST_POOL_STRIDE 2

// Helper function to copy data into array with boundary checking
void safe_memcpy(float* dest, const float* src, size_t size) {
    std::memcpy(dest, src, size * sizeof(float));
}

// CPU reference implementation for max pooling
void max_pooling_ref(const float *input, float *output, int height, int width, int channels, 
                    int pool_size, int pool_stride) {
    int out_height = (height - pool_size) / pool_stride + 1;
    int out_width = (width - pool_size) / pool_stride + 1;
    
    for (int c = 0; c < channels; c++) {
        for (int h = 0; h < out_height; h++) {
            for (int w = 0; w < out_width; w++) {
                float max_val = -3.40282e+38; // Min float value
                
                for (int ph = 0; ph < pool_size; ph++) {
                    for (int pw = 0; pw < pool_size; pw++) {
                        int in_row = h * pool_stride + ph;
                        int in_col = w * pool_stride + pw;
                        int in_idx = c * height * width + in_row * width + in_col;
                        
                        max_val = std::max(max_val, input[in_idx]);
                    }
                }
                
                int out_idx = c * out_height * out_width + h * out_width + w;
                output[out_idx] = max_val;
            }
        }
    }
}

// CPU reference implementation for average pooling
void avg_pooling_ref(const float *input, float *output, int height, int width, int channels, 
                    int pool_size, int pool_stride) {
    int out_height = (height - pool_size) / pool_stride + 1;
    int out_width = (width - pool_size) / pool_stride + 1;
    
    for (int c = 0; c < channels; c++) {
        for (int h = 0; h < out_height; h++) {
            for (int w = 0; w < out_width; w++) {
                float sum = 0.0f;
                
                for (int ph = 0; ph < pool_size; ph++) {
                    for (int pw = 0; pw < pool_size; pw++) {
                        int in_row = h * pool_stride + ph;
                        int in_col = w * pool_stride + pw;
                        int in_idx = c * height * width + in_row * width + in_col;
                        
                        sum += input[in_idx];
                    }
                }
                
                int out_idx = c * out_height * out_width + h * out_width + w;
                output[out_idx] = sum / (pool_size * pool_size);
            }
        }
    }
}

int main() {
    int height = TEST_HEIGHT;
    int width = TEST_WIDTH;
    int channels = TEST_CHANNELS;
    int pool_size = TEST_POOL_SIZE;
    int pool_stride = TEST_POOL_STRIDE;
    
    int in_size = channels * height * width;
    int out_height = (height - pool_size) / pool_stride + 1;
    int out_width = (width - pool_size) / pool_stride + 1;
    int out_size = channels * out_height * out_width;
    
    // Use statically allocated arrays to match HLS function interface
    static float input_array[INPUT_BUFFER_SIZE] = {0};
    static float output_max_array[OUTPUT_BUFFER_SIZE] = {0};
    static float output_avg_array[OUTPUT_BUFFER_SIZE] = {0};
    
    // Dynamic arrays for reference implementation
    float *ref_output_max = new float[out_size];
    float *ref_output_avg = new float[out_size];
    
    // Initialize input data with some pattern
    for (int c = 0; c < channels; c++) {
        for (int h = 0; h < height; h++) {
            for (int w = 0; w < width; w++) {
                int idx = c * height * width + h * width + w;
                input_array[idx] = c * 10 + h + w * 0.1f;  // Some pattern
            }
        }
    }
    
    // Run HLS kernel for max pooling
    pooling(input_array, output_max_array, height, width, channels, pool_size, pool_stride, POOL_MAX);
    
    // Run HLS kernel for average pooling
    pooling(input_array, output_avg_array, height, width, channels, pool_size, pool_stride, POOL_AVG);
    
    // Run reference implementations
    max_pooling_ref(input_array, ref_output_max, height, width, channels, pool_size, pool_stride);
    avg_pooling_ref(input_array, ref_output_avg, height, width, channels, pool_size, pool_stride);
    
    // Verify results for max pooling
    bool max_pass = true;
    for (int i = 0; i < out_size; i++) {
        if (std::abs(output_max_array[i] - ref_output_max[i]) > 1e-5) {
            std::cout << "MAX POOLING ERROR: output_max[" << i << "] = " << output_max_array[i]
                      << ", expected " << ref_output_max[i] << std::endl;
            max_pass = false;
            break;
        }
    }
    
    // Verify results for average pooling
    bool avg_pass = true;
    for (int i = 0; i < out_size; i++) {
        if (std::abs(output_avg_array[i] - ref_output_avg[i]) > 1e-5) {
            std::cout << "AVG POOLING ERROR: output_avg[" << i << "] = " << output_avg_array[i]
                      << ", expected " << ref_output_avg[i] << std::endl;
            avg_pass = false;
            break;
        }
    }
    
    if (max_pass && avg_pass) {
        std::cout << "Both MAX and AVG Pooling tests passed successfully!\n";
        
        // Print a sample of the output for verification
        std::cout << "\nSample MAX pooling output (first channel, first 4x4 block):\n";
        for (int h = 0; h < std::min(out_height, 4); h++) {
            for (int w = 0; w < std::min(out_width, 4); w++) {
                int idx = h * out_width + w;
                std::cout << output_max_array[idx] << "\t";
            }
            std::cout << std::endl;
        }
        
        std::cout << "\nSample AVG pooling output (first channel, first 4x4 block):\n";
        for (int h = 0; h < std::min(out_height, 4); h++) {
            for (int w = 0; w < std::min(out_width, 4); w++) {
                int idx = h * out_width + w;
                std::cout << output_avg_array[idx] << "\t";
            }
            std::cout << std::endl;
        }
    } else {
        if (!max_pass) std::cout << "MAX Pooling test failed.\n";
        if (!avg_pass) std::cout << "AVG Pooling test failed.\n";
    }
    
    // Clean up
    delete[] ref_output_max;
    delete[] ref_output_avg;
    
    return (max_pass && avg_pass) ? 0 : 1;
}