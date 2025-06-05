/**
* Copyright (C) 2025
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/

#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <random>

// XRT includes
#include "xrt/xrt_bo.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_kernel.h"

// Maximum dimensions
#define MAX_DIM 16
#define MAX_DATA_SIZE (MAX_DIM * MAX_DIM)
#define MAX_VECTOR_SIZE (MAX_DIM)
#define MAX_MATRIX_SIZE (MAX_DIM * MAX_DIM)

// Utility function to generate structured data
void generate_dataset(float* data, int rows, int cols) {
    // Use fixed seed for deterministic results
    std::mt19937 gen(42);
    std::uniform_real_distribution<float> dist(-0.1f, 0.1f);
    
    // Generate structured data with correlations + small random noise
    for (int i = 0; i < rows; i++) {
        float base_value = static_cast<float>(i) / rows;
        for (int j = 0; j < cols; j++) {
            if (j == 0) {
                // First component - base pattern
                data[i * cols + j] = base_value + dist(gen);
            } else if (j == 1) {
                // Second component - correlated with first
                data[i * cols + j] = 0.7f * data[i * cols + 0] + 0.3f * dist(gen);
            } else if (j == 2) {
                // Third component - negatively correlated with first 
                data[i * cols + j] = -0.4f * data[i * cols + 0] + 0.6f * dist(gen);
            } else {
                // Other components - different patterns
                data[i * cols + j] = 0.5f * std::sin(base_value * j) + 0.2f * dist(gen);
            }
        }
    }
}

// Utility function to print a matrix
void print_matrix(const float* matrix, int rows, int cols, const std::string& name) {
    std::cout << name << " (" << rows << "x" << cols << "):" << std::endl;
    for (int i = 0; i < std::min(rows, 10); i++) {
        for (int j = 0; j < std::min(cols, 10); j++) {
            std::cout << std::setw(10) << std::fixed << std::setprecision(4) << matrix[i * cols + j] << " ";
        }
        if (cols > 10) std::cout << "...";
        std::cout << std::endl;
    }
    if (rows > 10) std::cout << "..." << std::endl;
    std::cout << std::endl;
}

// Utility function to print a vector
void print_vector(const float* vector, int size, const std::string& name) {
    std::cout << name << " (size=" << size << "):" << std::endl;
    for (int i = 0; i < std::min(size, 10); i++) {
        std::cout << std::setw(10) << std::fixed << std::setprecision(4) << vector[i] << " ";
    }
    if (size > 10) std::cout << "...";
    std::cout << std::endl << std::endl;
}

// CPU implementation of PCA for verification
void cpu_pca(
    const float* data,
    float* mean,
    float* covariance,
    int rows,
    int cols
) {
    // Step 1: Compute mean
    for (int j = 0; j < cols; j++) {
        float sum = 0.0f;
        for (int i = 0; i < rows; i++) {
            sum += data[i * cols + j];
        }
        mean[j] = sum / rows;
    }
    
    // Step 2: Center the data and compute covariance
    std::vector<float> centered_data(rows * cols);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            centered_data[i * cols + j] = data[i * cols + j] - mean[j];
        }
    }
    
    // Step 3: Compute covariance matrix
    for (int i = 0; i < cols; i++) {
        for (int j = 0; j < cols; j++) {
            float sum = 0.0f;
            for (int k = 0; k < rows; k++) {
                sum += centered_data[k * cols + i] * centered_data[k * cols + j];
            }
            covariance[i * cols + j] = sum / (rows - 1);
        }
    }
}

// Function to verify mean and covariance
bool verify_results(
    const float* ref_mean,
    const float* ref_covariance,
    const float* fpga_mean,
    const float* fpga_covariance,
    int cols,
    float tolerance = 1e-4
) {
    bool mean_ok = true;
    bool cov_ok = true;
    
    // Verify mean
    for (int i = 0; i < cols; i++) {
        if (std::fabs(ref_mean[i] - fpga_mean[i]) > tolerance) {
            std::cout << "Mean mismatch at index " << i << ": "
                      << ref_mean[i] << " vs " << fpga_mean[i] << std::endl;
            mean_ok = false;
        }
    }
    
    // Verify covariance
    for (int i = 0; i < cols; i++) {
        for (int j = 0; j < cols; j++) {
            if (std::fabs(ref_covariance[i * cols + j] - fpga_covariance[i * cols + j]) > tolerance) {
                std::cout << "Covariance mismatch at index (" << i << "," << j << "): "
                          << ref_covariance[i * cols + j] << " vs " << fpga_covariance[i * cols + j] << std::endl;
                cov_ok = false;
            }
        }
    }
    
    return mean_ok && cov_ok;
}

// Function to verify eigenvectors - basic verification of Av = λv relationship
void verify_eigenvectors(
    const float* covariance,
    const float* eigenvalues,
    const float* eigenvectors,
    int cols
) {
    std::cout << "\nEigenvector verification (Av ≈ λv check):" << std::endl;
    
    // For each eigenvector
    for (int i = 0; i < cols; i++) {
        // Extract the eigenvector
        std::vector<float> v(cols);
        for (int j = 0; j < cols; j++) {
            v[j] = eigenvectors[j * cols + i];
        }
        
        // Calculate |v| (vector magnitude)
        float v_magnitude = 0.0f;
        for (int j = 0; j < cols; j++) {
            v_magnitude += v[j] * v[j];
        }
        v_magnitude = std::sqrt(v_magnitude);
        
        // Calculate A*v
        std::vector<float> Av(cols, 0.0f);
        for (int j = 0; j < cols; j++) {
            for (int k = 0; k < cols; k++) {
                Av[j] += covariance[j * cols + k] * v[k];
            }
        }
        
        // Calculate λ*v
        std::vector<float> lambda_v(cols);
        for (int j = 0; j < cols; j++) {
            lambda_v[j] = eigenvalues[i] * v[j];
        }
        
        // Calculate |Av - λv| / |v|
        float error = 0.0f;
        for (int j = 0; j < cols; j++) {
            error += (Av[j] - lambda_v[j]) * (Av[j] - lambda_v[j]);
        }
        error = std::sqrt(error) / v_magnitude;
        
        std::cout << "Eigenvalue " << i << " = " << eigenvalues[i] 
                  << ", Relative error = " << error << std::endl;
        
        if (error > 0.1f) {
            std::cout << "  Warning: High error for eigenvector " << i << std::endl;
        }
    }
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    // Check command line arguments
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <XCLBIN File> <rows>" << std::endl;
        return EXIT_FAILURE;
    }
    
    // Parse arguments
    std::string binary_file = argv[1];
    int rows = std::stoi(argv[2]);
    int cols = 8; // Fixed number of columns for this example
    
    if (rows > MAX_DIM) {
        std::cout << "Warning: Number of rows (" << rows << ") exceeds MAX_DIM (" 
                  << MAX_DIM << "). Using MAX_DIM instead." << std::endl;
        rows = MAX_DIM;
    }
    
    if (cols > MAX_DIM) {
        std::cout << "Warning: Number of columns (" << cols << ") exceeds MAX_DIM (" 
                  << MAX_DIM << "). Using MAX_DIM instead." << std::endl;
        cols = MAX_DIM;
    }
    
    std::cout << "Running PCA with " << rows << " rows and " << cols << " columns." << std::endl;
    
    // Generate input data
    std::vector<float> data(MAX_DATA_SIZE, 0.0f);
    generate_dataset(data.data(), rows, cols);
    
    // Allocate memory for outputs
    std::vector<float> fpga_mean(MAX_VECTOR_SIZE, 0.0f);
    std::vector<float> fpga_covariance(MAX_MATRIX_SIZE, 0.0f);
    std::vector<float> fpga_eigenvalues(MAX_VECTOR_SIZE, 0.0f);
    std::vector<float> fpga_eigenvectors(MAX_MATRIX_SIZE, 0.0f);
    
    // CPU reference results
    std::vector<float> cpu_mean(cols, 0.0f);
    std::vector<float> cpu_covariance(cols * cols, 0.0f);
    
    // Print input data
    print_matrix(data.data(), rows, cols, "Input Data");
    
    //////////////////////////////////////////////////////////////////////////
    // XRT FPGA Setup and Execution
    //////////////////////////////////////////////////////////////////////////
    
    // Initialize XRT
    std::cout << "Initializing XRT..." << std::endl;
    auto device = xrt::device(0); // Open the first device
    auto uuid = device.load_xclbin(binary_file);
    auto kernel = xrt::kernel(device, uuid, "pca_eigen_kernel");
    
    // Allocate device buffers
    std::cout << "Allocating device buffers..." << std::endl;
    auto data_buf = xrt::bo(device, data.data(), MAX_DATA_SIZE * sizeof(float), kernel.group_id(0));
    auto mean_buf = xrt::bo(device, fpga_mean.data(), MAX_VECTOR_SIZE * sizeof(float), kernel.group_id(1));
    auto cov_buf = xrt::bo(device, fpga_covariance.data(), MAX_MATRIX_SIZE * sizeof(float), kernel.group_id(2));
    auto eval_buf = xrt::bo(device, fpga_eigenvalues.data(), MAX_VECTOR_SIZE * sizeof(float), kernel.group_id(3));
    auto evec_buf = xrt::bo(device, fpga_eigenvectors.data(), MAX_MATRIX_SIZE * sizeof(float), kernel.group_id(4));
    
    // Sync input buffers to device
    std::cout << "Transferring data to device..." << std::endl;
    data_buf.sync_to_device();
    
    // Start timing
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Execute the kernel
    std::cout << "Executing kernel..." << std::endl;
    auto run = kernel(data_buf, mean_buf, cov_buf, eval_buf, evec_buf, rows, cols);
    
    // Wait for kernel completion
    std::cout << "Waiting for kernel completion..." << std::endl;
    run.wait();
    
    // Stop timing
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    // Sync output buffers from device
    std::cout << "Transferring results from device..." << std::endl;
    mean_buf.sync_from_device();
    cov_buf.sync_from_device();
    eval_buf.sync_from_device();
    evec_buf.sync_from_device();
    
    //////////////////////////////////////////////////////////////////////////
    // Result Verification
    //////////////////////////////////////////////////////////////////////////
    
    // Compute reference results on CPU
    std::cout << "Computing reference results on CPU..." << std::endl;
    cpu_pca(data.data(), cpu_mean.data(), cpu_covariance.data(), rows, cols);
    
    // Print results
    print_vector(fpga_mean.data(), cols, "FPGA Mean Vector");
    print_vector(cpu_mean.data(), cols, "CPU Mean Vector");
    
    print_matrix(fpga_covariance.data(), cols, cols, "FPGA Covariance Matrix");
    print_matrix(cpu_covariance.data(), cols, cols, "CPU Covariance Matrix");
    
    print_vector(fpga_eigenvalues.data(), cols, "FPGA Eigenvalues");
    print_matrix(fpga_eigenvectors.data(), cols, cols, "FPGA Eigenvectors");
    
    // Verify results
    bool results_match = verify_results(
        cpu_mean.data(),
        cpu_covariance.data(),
        fpga_mean.data(),
        fpga_covariance.data(),
        cols
    );
    
    // Verify eigenvectors
    verify_eigenvectors(
        fpga_covariance.data(),
        fpga_eigenvalues.data(),
        fpga_eigenvectors.data(),
        cols
    );
    
    // Report timing and verification results
    std::cout << "Kernel execution time: " << duration.count() / 1000.0 << " ms" << std::endl;
    
    if (results_match) {
        std::cout << "Verification PASSED for mean and covariance!" << std::endl;
    } else {
        std::cout << "Verification FAILED for mean and covariance!" << std::endl;
    }
    
    std::cout << "PCA Eigenvalue decomposition completed successfully!" << std::endl;
    
    return results_match ? EXIT_SUCCESS : EXIT_FAILURE;
}