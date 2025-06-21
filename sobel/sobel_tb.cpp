#include <iostream>
#include <cstdlib>
#include <cmath>
#include "sobel.h"

#define TEST_WIDTH 16
#define TEST_HEIGHT 16

// Generate very simple test image with clear vertical edge
void generate_test_image(pixel_t *image, int width, int height) {
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            // Simple vertical edge: left half = 0, right half = 255
            if (col < width/2) {
                image[row * width + col] = 0;   // Black
            } else {
                image[row * width + col] = 255; // White
            }
        }
    }
}

// Reference Sobel implementation for verification
void sobel_reference(const pixel_t *input, gradient_t *output, int width, int height) {
    const int sobel_x[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    const int sobel_y[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};
    
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            if (row == 0 || row == height-1 || col == 0 || col == width-1) {
                output[row * width + col] = 0; // Border pixels
            } else {
                int grad_x = 0, grad_y = 0;
                
                for (int i = -1; i <= 1; i++) {
                    for (int j = -1; j <= 1; j++) {
                        pixel_t pixel = input[(row + i) * width + (col + j)];
                        grad_x += pixel * sobel_x[i + 1][j + 1];
                        grad_y += pixel * sobel_y[i + 1][j + 1];
                    }
                }
                
                grad_x = abs(grad_x);
                grad_y = abs(grad_y);
                int result = grad_x + grad_y;
                output[row * width + col] = (result > 255) ? 255 : result;
            }
        }
    }
}

int main() {
    // Allocate memory
    pixel_t *input = new pixel_t[TEST_WIDTH * TEST_HEIGHT];
    gradient_t *output_hw = new gradient_t[TEST_WIDTH * TEST_HEIGHT];
    gradient_t *output_ref = new gradient_t[TEST_WIDTH * TEST_HEIGHT];
    
    // Generate test image
    generate_test_image(input, TEST_WIDTH, TEST_HEIGHT);
    
    std::cout << "Running Sobel filter test..." << std::endl;
    std::cout << "Image size: " << TEST_WIDTH << "x" << TEST_HEIGHT << std::endl;
    
    // Run hardware implementation
    sobel_filter(input, output_hw, TEST_WIDTH, TEST_HEIGHT);
    
    // Run reference implementation
    sobel_reference(input, output_ref, TEST_WIDTH, TEST_HEIGHT);
    
    // Verify results
    bool pass = true;
    int errors = 0;
    int max_error = 0;
    
    for (int i = 0; i < TEST_WIDTH * TEST_HEIGHT; i++) {
        int diff = abs(output_hw[i] - output_ref[i]);
        if (diff > max_error) {
            max_error = diff;
        }
        
        // Should be exact match now
        if (diff > 0) {
            if (errors < 10) { // Show first 10 errors
                int row = i / TEST_WIDTH;
                int col = i % TEST_WIDTH;
                std::cout << "ERROR at (" << row << "," << col << "): " 
                         << "HW=" << output_hw[i] 
                         << ", REF=" << output_ref[i] 
                         << ", DIFF=" << diff << std::endl;
            }
            errors++;
            pass = false;
        }
    }
    
    std::cout << "Total errors: " << errors << std::endl;
    std::cout << "Maximum error: " << max_error << std::endl;
    
    if (pass) {
        std::cout << "✓ Test PASSED successfully!" << std::endl;
    } else {
        std::cout << "✗ Test FAILED!" << std::endl;
    }
    
    // Print some sample results for debugging
    std::cout << "\nFull test image results:" << std::endl;
    std::cout << "Input:" << std::endl;
    for (int row = 0; row < TEST_HEIGHT; row++) {
        for (int col = 0; col < TEST_WIDTH; col++) {
            std::cout << (int)input[row * TEST_WIDTH + col] << "\t";
        }
        std::cout << std::endl;
    }
    
    std::cout << "\nHW Output (edges):" << std::endl;
    for (int row = 0; row < TEST_HEIGHT; row++) {
        for (int col = 0; col < TEST_WIDTH; col++) {
            std::cout << (int)output_hw[row * TEST_WIDTH + col] << "\t";
        }
        std::cout << std::endl;
    }
    
    std::cout << "\nReference Output (edges):" << std::endl;
    for (int row = 0; row < TEST_HEIGHT; row++) {
        for (int col = 0; col < TEST_WIDTH; col++) {
            std::cout << (int)output_ref[row * TEST_WIDTH + col] << "\t";
        }
        std::cout << std::endl;
    }
    
    // Cleanup
    delete[] input;
    delete[] output_hw;
    delete[] output_ref;
    
    return pass ? 0 : 1;
}