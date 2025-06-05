#include <iostream>
#include <cmath>
#include "activation.h"

#define SIZE 1024
#define TOLERANCE 0.01  // Tolerance for float comparison

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
    // Arrays for testing
    float input[SIZE], output[SIZE];
    float expected[SIZE];
    
    // Test each activation function
    const char* function_names[] = {"ReLU", "Sigmoid", "Tanh"};
    
    for (int function_type = 0; function_type < 3; function_type++) {
        std::cout << "Testing " << function_names[function_type] << " activation function..." << std::endl;
        
        // Generate test data with a range of values
        for (int i = 0; i < SIZE; i++) {
            float value = (float)i / (SIZE/16) - 8.0f;  // Range from -8 to 8
            input[i] = value;
            
            // Calculate expected output using software functions
            switch (function_type) {
                case 0: // ReLU
                    expected[i] = sw_relu(value);
                    break;
                case 1: // Sigmoid
                    expected[i] = sw_sigmoid(value);
                    break;
                case 2: // Tanh
                    expected[i] = sw_tanh(value);
                    break;
            }
        }
        
        // Call the kernel function
        activation_kernel(input, output, SIZE, function_type);
        
        // Verify results
        bool pass = true;
        int error_count = 0;
        float max_error = 0.0f;
        
        for (int i = 0; i < SIZE; i++) {
            float hw_result = output[i];
            float sw_result = expected[i];
            float error = fabs(hw_result - sw_result);
            max_error = std::max(max_error, error);
            
            if (error > TOLERANCE) {
                if (error_count < 10) {  // Only show up to 10 errors
                    std::cout << "Error at index " << i << ": input = " << input[i] 
                            << ", hardware result = " << hw_result 
                            << ", software result = " << sw_result 
                            << ", error = " << error << std::endl;
                }
                error_count++;
                pass = false;
            }
        }
        
        if (pass) {
            std::cout << "Test PASSED for " << function_names[function_type] << "!" << std::endl;
        } else {
            std::cout << "Test FAILED for " << function_names[function_type] << " with " 
                     << error_count << " errors!" << std::endl;
            std::cout << "Maximum error: " << max_error << std::endl;
        }
        std::cout << "--------------------" << std::endl;
    }
    
    return 0;
}