#include <iostream>
#include <cstdlib>
#include <cmath>
#include "batchnorm.h"

#define BATCH_SIZE 1024
#define EPSILON 0.00001f

int main() {
    float input[BATCH_SIZE], gamma[N], beta[N], mean[N], variance[N], output[BATCH_SIZE];
    float expected[BATCH_SIZE];

    // Initialize parameters
    for (int i = 0; i < N; i++) {
        gamma[i] = 1.0f;       // Scale factor
        beta[i] = 0.0f;        // Shift factor
        mean[i] = 5.0f;        // Example mean
        variance[i] = 4.0f;    // Example variance
    }

    // Initialize input data
    for (int i = 0; i < BATCH_SIZE; i++) {
        input[i] = i % 20;  // Some test values
    }

    // Calculate expected results for verification
    for (int i = 0; i < BATCH_SIZE; i++) {
        int channel = i % N;
        expected[i] = gamma[channel] * (input[i] - mean[channel]) / sqrt(variance[channel] + EPSILON) + beta[channel];
    }

    // Call the function to be synthesized
    batchnorm(input, gamma, beta, mean, variance, output, BATCH_SIZE, EPSILON);

    // Verify results
    bool pass = true;
    for (int i = 0; i < BATCH_SIZE; i++) {
        float diff = std::abs(output[i] - expected[i]);
        if (diff > 0.001f) {
            std::cout << "ERROR: output[" << i << "] = " << output[i]
                      << ", expected " << expected[i] 
                      << ", diff = " << diff << std::endl;
            pass = false;
            if (i > 10) break; // Only show first few errors
        }
    }

    if (pass) {
        std::cout << "Test passed successfully.\n";
        return 0;
    } else {
        std::cout << "Test failed.\n";
        return 1;
    }
}