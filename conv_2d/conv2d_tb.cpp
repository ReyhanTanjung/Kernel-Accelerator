#include <iostream>
#include <cstdlib>
#include <cmath>
#include "conv2d.h"

// Test dimensions - reduce size to avoid memory issues
#define TEST_HEIGHT 16
#define TEST_WIDTH 16
#define TEST_KERNEL_SIZE 3

// Tolerance for floating-point comparison
#define EPSILON 1e-5

// Reference implementation of 2D convolution for verification
void conv2d_reference(
    const float input[MAX_INPUT_SIZE],
    const float kernel[MAX_KERNEL_SIZE_SQ],
    float output[MAX_OUTPUT_SIZE],
    int height,
    int width,
    int kernel_size
) {
    int output_height = height - kernel_size + 1;
    int output_width = width - kernel_size + 1;
    
    for (int y = 0; y < output_height; y++) {
        for (int x = 0; x < output_width; x++) {
            float sum = 0.0f;
            
            for (int ky = 0; ky < kernel_size; ky++) {
                for (int kx = 0; kx < kernel_size; kx++) {
                    int input_y = y + ky;
                    int input_x = x + kx;
                    sum += input[input_y * width + input_x] * kernel[ky * kernel_size + kx];
                }
            }
            
            output[y * output_width + x] = sum;
        }
    }
}

int main() {
    // Static allocation to avoid dynamic memory issues in HLS
    float input[MAX_INPUT_SIZE];
    float kernel[MAX_KERNEL_SIZE_SQ];
    
    // Calculate output dimensions
    int output_height = TEST_HEIGHT - TEST_KERNEL_SIZE + 1;
    int output_width = TEST_WIDTH - TEST_KERNEL_SIZE + 1;
    
    // Output buffers
    float output[MAX_OUTPUT_SIZE];
    float reference_output[MAX_OUTPUT_SIZE];
    
    // Initialize input with test pattern
    for (int y = 0; y < TEST_HEIGHT; y++) {
        for (int x = 0; x < TEST_WIDTH; x++) {
            int idx = y * TEST_WIDTH + x;
            input[idx] = static_cast<float>(idx % 16) / 16.0f;
        }
    }
    
    // Initialize kernel with a simple filter (Gaussian-like)
    kernel[0] = 1.0f/16.0f; kernel[1] = 2.0f/16.0f; kernel[2] = 1.0f/16.0f;
    kernel[3] = 2.0f/16.0f; kernel[4] = 4.0f/16.0f; kernel[5] = 2.0f/16.0f;
    kernel[6] = 1.0f/16.0f; kernel[7] = 2.0f/16.0f; kernel[8] = 1.0f/16.0f;
    
    std::cout << "Running reference implementation...\n";
    // Run reference implementation
    conv2d_reference(input, kernel, reference_output, TEST_HEIGHT, TEST_WIDTH, TEST_KERNEL_SIZE);
    
    std::cout << "Running HLS implementation...\n";
    // Run the HLS implementation to be tested
    conv2d(input, kernel, output, TEST_HEIGHT, TEST_WIDTH, TEST_KERNEL_SIZE);
    
    // Verify results
    bool pass = true;
    std::cout << "Verifying results...\n";
    for (int y = 0; y < output_height; y++) {
        for (int x = 0; x < output_width; x++) {
            int idx = y * output_width + x;
            if (std::fabs(output[idx] - reference_output[idx]) > EPSILON) {
                std::cout << "Error at position (" << y << "," << x << "): " 
                          << output[idx] << " (HLS) vs " 
                          << reference_output[idx] << " (ref), "
                          << "diff = " << std::fabs(output[idx] - reference_output[idx]) << std::endl;
                pass = false;
                if (y * output_width + x >= 10) {
                    std::cout << "Showing only first 10 errors...\n";
                    y = output_height; // Break outer loop
                    break;
                }
            }
        }
    }
    
    if (pass) {
        std::cout << "Test PASSED! The convolution output matches the reference.\n";
    } else {
        std::cout << "Test FAILED! The convolution output does not match the reference.\n";
    }
    
    return pass ? 0 : 1;
}