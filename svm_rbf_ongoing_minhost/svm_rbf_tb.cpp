// svm_rbf_test.cpp
#include <iostream>
#include <cstdlib>
#include <vector>
#include <cmath>
#include "svm_rbf.h"

// Helper function to generate random data between -1 and 1
data_t random_data() {
    return (data_t)(2.0 * rand() / RAND_MAX - 1.0);
}

// Helper function to compute RBF kernel on CPU for verification
data_t rbf_kernel(const data_t* x, const data_t* y, int n_features, data_t gamma) {
    data_t squared_distance = 0.0;
    for (int i = 0; i < n_features; i++) {
        data_t diff = x[i] - y[i];
        squared_distance += diff * diff;
    }
    return exp(-gamma * squared_distance);
}

int main() {
    // Seed random number generator
    srand(42);
    
    // Test parameters
    const int n_features = 16;  // Number of features
    const int n_sv = 64;        // Number of support vectors
    const data_t gamma = 0.1;   // RBF kernel parameter
    const data_t bias = -0.5;   // SVM bias

    // Allocate memory
    std::vector<data_t> x_test(n_features);
    std::vector<data_t> support_vectors(n_sv * n_features);
    std::vector<data_t> alphas(n_sv);
    data_t decision_value = 0.0;
    data_t expected_value = 0.0;

    // Initialize test data
    std::cout << "Initializing test data..." << std::endl;
    for (int i = 0; i < n_features; i++) {
        x_test[i] = random_data();
    }

    for (int i = 0; i < n_sv; i++) {
        for (int j = 0; j < n_features; j++) {
            support_vectors[i * n_features + j] = random_data();
        }
        // Make some alpha values positive and some negative as in real SVM
        alphas[i] = random_data() * 2.0;  // Random alpha between -2 and 2
    }

    // Compute expected result on CPU
    std::cout << "Computing reference result on CPU..." << std::endl;
    for (int i = 0; i < n_sv; i++) {
        data_t kernel_val = rbf_kernel(&x_test[0], &support_vectors[i * n_features], n_features, gamma);
        expected_value += alphas[i] * kernel_val;
    }
    expected_value += bias;

    // Call the hardware function
    std::cout << "Running hardware implementation..." << std::endl;
    svm_rbf_kernel(
        &x_test[0],
        &support_vectors[0],
        &alphas[0],
        gamma,
        bias,
        n_features,
        n_sv,
        &decision_value
    );

    // Verify results
    std::cout << "Hardware result: " << decision_value << std::endl;
    std::cout << "Reference result: " << expected_value << std::endl;
    
    // Check if close enough (floating point comparison)
    const double epsilon = 1e-4;
    bool pass = std::abs(decision_value - expected_value) < epsilon;
    
    if (pass) {
        std::cout << "TEST PASSED!" << std::endl;
    } else {
        std::cout << "TEST FAILED: Results don't match" << std::endl;
        std::cout << "Difference: " << std::abs(decision_value - expected_value) << std::endl;
    }

    // Performance test with larger data
    std::cout << "\nRunning performance test..." << std::endl;
    
    // Time measurement (for C-sim only)
    clock_t start = clock();
    
    // Run multiple iterations for timing
    const int iterations = 100;
    for (int i = 0; i < iterations; i++) {
        svm_rbf_kernel(
            &x_test[0],
            &support_vectors[0],
            &alphas[0],
            gamma,
            bias,
            n_features,
            n_sv,
            &decision_value
        );
    }
    
    clock_t end = clock();
    double cpu_time = (double)(end - start) / CLOCKS_PER_SEC;
    std::cout << "Execution time for " << iterations << " iterations: " << cpu_time << " seconds" << std::endl;
    
    return pass ? 0 : 1;
}