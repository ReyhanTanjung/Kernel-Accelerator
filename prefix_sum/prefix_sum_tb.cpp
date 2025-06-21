#include <iostream>
#include <cstdlib>
#include "prefix_sum.h"

#define TEST_SIZE 1024

int main() {
    // Static arrays to avoid dynamic allocation issues in cosim
    static int input[TEST_SIZE];
    static int output[TEST_SIZE];
    static int golden[TEST_SIZE];
    
    // Initialize input data
    std::cout << "Initializing test data..." << std::endl;
    for (int i = 0; i < TEST_SIZE; i++) {
        input[i] = i + 1;  // Simple ascending sequence: 1, 2, 3, ...
    }
    
    // Compute golden reference (expected output)
    golden[0] = input[0];
    for (int i = 1; i < TEST_SIZE; i++) {
        golden[i] = golden[i-1] + input[i];
    }
    
    // Call the function to be synthesized
    std::cout << "Running prefix sum kernel..." << std::endl;
    prefix_sum(input, output, TEST_SIZE);
    
    // Verify results
    std::cout << "Verifying results..." << std::endl;
    bool pass = true;
    int error_count = 0;
    
    for (int i = 0; i < TEST_SIZE; i++) {
        if (output[i] != golden[i]) {
            if (error_count < 10) {  // Only show first 10 errors
                std::cout << "ERROR: output[" << i << "] = " << output[i]
                          << ", expected " << golden[i] << std::endl;
            }
            pass = false;
            error_count++;
        }
    }
    
    if (pass) {
        std::cout << "✅ Test PASSED successfully!" << std::endl;
        std::cout << "Sample results:" << std::endl;
        std::cout << "Input:  [1, 2, 3, 4, 5, ...]" << std::endl;
        std::cout << "Output: [" << output[0] << ", " << output[1] << ", " 
                  << output[2] << ", " << output[3] << ", " << output[4] << ", ...]" << std::endl;
        return 0;
    } else {
        std::cout << "❌ Test FAILED!" << std::endl;
        std::cout << "Total errors: " << error_count << std::endl;
        return 1;
    }
}