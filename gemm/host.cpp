#include <iostream>
#include <vector>
#include <cmath>
#include <xrt.h>
#include <experimental/xrt_kernel.h>
#include <chrono>
#include <memory>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <cstring>

// Matrix dimensions - keep matched with gemm.h
#define M_SIZE 32  // Matrix A: M_SIZE x K_SIZE
#define K_SIZE 32  // Matrix B: K_SIZE x N_SIZE
#define N_SIZE 32  // Matrix C: M_SIZE x N_SIZE

// Define the systolic array size
#define SYSTOLIC_SIZE 8

// Number of repeated operations to increase computational load
#define NUM_ITERATIONS 1000

// For validating results
bool is_close(float a, float b, float rtol=1e-5, float atol=1e-8) {
    return std::abs(a - b) <= (atol + rtol * std::abs(b));
}

// Function for aligned memory allocation
void* aligned_alloc(size_t alignment, size_t size) {
    void* ptr = nullptr;
#ifdef _WIN32
    ptr = _aligned_malloc(size, alignment);
#else
    if (posix_memalign(&ptr, alignment, size) != 0) {
        ptr = nullptr;
    }
#endif
    return ptr;
}

// Function to free aligned memory
void aligned_free(void* ptr) {
#ifdef _WIN32
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

// Reference GEMM implementation for verification
void gemm_reference(const float *A, const float *B, float *C,
                   float alpha, float beta,
                   int m, int k, int n) {
    // Create a copy of C to work with
    std::vector<float> C_temp(m * n);
    
    // Apply beta to C
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            C_temp[i * n + j] = beta * C[i * n + j];
        }
    }
    
    // Compute matrix multiplication and apply alpha
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            float sum = 0.0f;
            for (int l = 0; l < k; l++) {
                sum += A[i * k + l] * B[l * n + j];
            }
            C_temp[i * n + j] += alpha * sum;
        }
    }
    
    // Copy result back to C
    std::memcpy(C, C_temp.data(), m * n * sizeof(float));
}

// Function to print part of a matrix for debugging
void print_matrix_section(const char* name, const float* matrix, int rows, int cols, 
                         int max_rows=5, int max_cols=5) {
    std::cout << "Matrix " << name << " (" << rows << "x" << cols << ") first " 
              << max_rows << "x" << max_cols << " elements:" << std::endl;
    
    for (int i = 0; i < std::min(rows, max_rows); i++) {
        for (int j = 0; j < std::min(cols, max_cols); j++) {
            std::cout << std::fixed << std::setprecision(4) << matrix[i * cols + j] << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "USAGE: " << argv[0] << " <xclbin>" << std::endl;
        return 1;
    }
    
    std::string xclbin_path(argv[1]);
    
    try {
        std::cout << "Systolic Array GEMM Host Application" << std::endl;
        std::cout << "Running " << NUM_ITERATIONS << " iterations of " 
                  << M_SIZE << "x" << K_SIZE << " * " << K_SIZE << "x" << N_SIZE 
                  << " matrix multiplication" << std::endl;
        std::cout << "Systolic array size: " << SYSTOLIC_SIZE << "x" << SYSTOLIC_SIZE << std::endl;
        std::cout << std::string(60, '-') << std::endl;
        
        // Allocate aligned memory for matrices
        size_t alignment = 4096; // 4K alignment
        
        // Use aligned memory for matrices
        float* A_aligned = static_cast<float*>(aligned_alloc(alignment, M_SIZE * K_SIZE * sizeof(float)));
        float* B_aligned = static_cast<float*>(aligned_alloc(alignment, K_SIZE * N_SIZE * sizeof(float)));
        float* C_aligned = static_cast<float*>(aligned_alloc(alignment, M_SIZE * N_SIZE * sizeof(float)));
        float* C_golden = static_cast<float*>(aligned_alloc(alignment, M_SIZE * N_SIZE * sizeof(float)));
        
        if (!A_aligned || !B_aligned || !C_aligned || !C_golden) {
            std::cerr << "ERROR: Failed to allocate aligned memory" << std::endl;
            return 1;
        }
        
        // Using smart pointers to ensure memory is freed
        std::unique_ptr<float, decltype(&aligned_free)> A_ptr(A_aligned, aligned_free);
        std::unique_ptr<float, decltype(&aligned_free)> B_ptr(B_aligned, aligned_free);
        std::unique_ptr<float, decltype(&aligned_free)> C_ptr(C_aligned, aligned_free);
        std::unique_ptr<float, decltype(&aligned_free)> C_golden_ptr(C_golden, aligned_free);

        std::cout << "Initializing matrices...\n";
        
        // Initialize matrices with test values - match the patterns in test bench
        for (int i = 0; i < M_SIZE; i++) {
            for (int j = 0; j < K_SIZE; j++) {
                A_aligned[i * K_SIZE + j] = (i + j) * 0.1f;
            }
        }
        
        for (int i = 0; i < K_SIZE; i++) {
            for (int j = 0; j < N_SIZE; j++) {
                B_aligned[i * N_SIZE + j] = (i * j) * 0.01f;
            }
        }
        
        for (int i = 0; i < M_SIZE; i++) {
            for (int j = 0; j < N_SIZE; j++) {
                C_aligned[i * N_SIZE + j] = i - j;
                C_golden[i * N_SIZE + j] = C_aligned[i * N_SIZE + j];
            }
        }
        
        // Print small sections of input matrices for verification
        if (M_SIZE <= 32 && K_SIZE <= 32 && N_SIZE <= 32) {
            print_matrix_section("A", A_aligned, M_SIZE, K_SIZE);
            print_matrix_section("B", B_aligned, K_SIZE, N_SIZE);
            print_matrix_section("C (initial)", C_aligned, M_SIZE, N_SIZE);
        }
        
        // GEMM parameters - match those in test bench
        float alpha = 1.5f;
        float beta = 0.8f;
        
        // Compute reference result for first iteration for verification
        std::cout << "Computing reference results...\n";
        gemm_reference(A_aligned, B_aligned, C_golden, alpha, beta, M_SIZE, K_SIZE, N_SIZE);
        
        // Print small section of expected result for verification
        if (M_SIZE <= 32 && N_SIZE <= 32) {
            print_matrix_section("C_expected", C_golden, M_SIZE, N_SIZE);
        }
        
        // Setup XRT device and kernel
        std::cout << "Setting up XRT device and kernel...\n";
        auto device = xrt::device(0);
        auto uuid = device.load_xclbin(xclbin_path);
        auto kernel = xrt::kernel(device, uuid, "gemm", xrt::kernel::cu_access_mode::exclusive);
        
        // Create XRT buffers for data
        std::cout << "Creating XRT buffers...\n";
        auto A_buf = xrt::bo(device, A_aligned, M_SIZE * K_SIZE * sizeof(float), kernel.group_id(0));
        auto B_buf = xrt::bo(device, B_aligned, K_SIZE * N_SIZE * sizeof(float), kernel.group_id(1)); 
        auto C_buf = xrt::bo(device, C_aligned, M_SIZE * N_SIZE * sizeof(float), kernel.group_id(2));
        
        // Sync input data to device
        std::cout << "Transferring input data to device...\n";
        A_buf.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        B_buf.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        C_buf.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        
        // Execute GEMM kernel multiple times to increase workload
        std::cout << "Executing Systolic Array GEMM kernel " << NUM_ITERATIONS << " times...\n";
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
            auto run = kernel(A_buf, B_buf, C_buf, alpha, beta, M_SIZE, K_SIZE, N_SIZE);
            run.wait();
            
            // For all iterations except the last one, we need to sync back the C buffer
            // and use it as input for the next iteration
            if (iter < NUM_ITERATIONS - 1) {
                C_buf.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
                C_buf.sync(XCL_BO_SYNC_BO_TO_DEVICE);
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        std::cout << "Kernel execution completed in " << duration_ms << " ms for " 
                  << NUM_ITERATIONS << " iterations" << std::endl;
        std::cout << "Average time per iteration: " << duration_ms / NUM_ITERATIONS << " ms" << std::endl;
        
        // Sync results back to host
        std::cout << "Retrieving results from device...\n";
        C_buf.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        
        // Print small section of result for verification after first iteration
        if (M_SIZE <= 32 && N_SIZE <= 32) {
            print_matrix_section("C (result after iterations)", C_aligned, M_SIZE, N_SIZE);
        }
        
        // Verify results from first iteration
        std::cout << "Verifying results (first iteration only)...\n";
        bool pass = true;
        int error_count = 0;
        float max_error = 0.0f;
        float avg_error = 0.0f;
        int total_elements = M_SIZE * N_SIZE;
        
        // For multiple iterations, we can only verify if results are reasonable
        // since we don't have a reference for all iterations
        for (int i = 0; i < M_SIZE; i++) {
            for (int j = 0; j < N_SIZE; j++) {
                int idx = i * N_SIZE + j;
                
                // Check if the value is finite (not NaN or infinity)
                if (!std::isfinite(C_aligned[idx])) {
                    std::cout << "Error at C[" << i << "][" << j << "]: " 
                              << "Value is not finite: " << C_aligned[idx] << std::endl;
                    pass = false;
                    error_count++;
                }
                
                // Only for the first iteration - compare with golden reference
                float error = std::abs(C_aligned[idx] - C_golden[idx]);
                avg_error += error;
                max_error = std::max(max_error, error);
                
                if (!is_close(C_aligned[idx], C_golden[idx]) && error_count < 10) {
                    std::cout << "Error at C[" << i << "][" << j << "]: " 
                              << C_aligned[idx] << " vs expected " << C_golden[idx] 
                              << ", diff = " << error << std::endl;
                    pass = false;
                    error_count++;
                }
            }
        }
        
        // Calculate average error
        avg_error /= total_elements;
        
        if (pass) {
            std::cout << "Verification PASSED!\n";
        } else {
            std::cout << "Verification FAILED with " << error_count << " errors!\n";
            std::cout << "Maximum error: " << max_error << std::endl;
            std::cout << "Average error: " << avg_error << std::endl;
        }
        
        // Calculate total operations across all iterations
        double operations_per_iter = 2.0 * M_SIZE * N_SIZE * K_SIZE;
        double total_operations = operations_per_iter * NUM_ITERATIONS;
        double gflops = (total_operations / 1.0e9) / (duration_ms / 1000.0);
        
        // Bandwidth: read A, B, and read/write C for each iteration
        double bytes_per_iter = (M_SIZE * K_SIZE + K_SIZE * N_SIZE + 2 * M_SIZE * N_SIZE) * sizeof(float);
        double total_bytes = bytes_per_iter * NUM_ITERATIONS;
        double bandwidth_gbps = (total_bytes / 1.0e9) / (duration_ms / 1000.0);
        
        std::cout << "\n" << std::string(60, '-') << "\n";
        std::cout << "Systolic Array FPGA Performance Metrics:\n";
        std::cout << std::string(60, '-') << "\n";
        std::cout << "  Matrix dimensions: A(" << M_SIZE << "x" << K_SIZE 
                  << ") * B(" << K_SIZE << "x" << N_SIZE << ")\n";
        std::cout << "  Systolic array size: " << SYSTOLIC_SIZE << "x" << SYSTOLIC_SIZE << "\n";
        std::cout << "  Number of iterations: " << NUM_ITERATIONS << "\n";
        std::cout << "  Total time: " << duration_ms << " ms\n";
        std::cout << "  Time per iteration: " << duration_ms / NUM_ITERATIONS << " ms\n";
        std::cout << "  Computation: " << std::fixed << std::setprecision(6) << gflops << " GFLOPS\n";
        std::cout << "  Memory bandwidth: " << std::fixed << std::setprecision(6) << bandwidth_gbps << " GB/s\n";
        std::cout << std::string(60, '-') << "\n";
        
        return pass ? 0 : 1;
        
    } catch (const std::exception &e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }
}