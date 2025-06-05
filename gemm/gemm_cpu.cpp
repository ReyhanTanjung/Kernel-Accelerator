#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <string>
#include <iomanip>
#include <thread>
#include <algorithm>

// Matrix dimensions - matching the FPGA implementation
#define M_SIZE 32  // Matrix A: M_SIZE x K_SIZE
#define K_SIZE 32  // Matrix B: K_SIZE x N_SIZE
#define N_SIZE 32  // Matrix C: M_SIZE x N_SIZE

// Number of repeated operations to increase computational load
#define NUM_ITERATIONS 1000

// For validating results
bool is_close(float a, float b, float rtol=1e-5, float atol=1e-8) {
    return std::abs(a - b) <= (atol + rtol * std::abs(b));
}

// Basic CPU implementation of GEMM: C = alpha*A*B + beta*C
void gemm_cpu(const float *A, const float *B, float *C, 
              float alpha, float beta, 
              int m, int k, int n) {
    // Simple triple loop implementation
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            float sum = 0.0f;
            for (int l = 0; l < k; l++) {
                sum += A[i * k + l] * B[l * n + j];
            }
            C[i * n + j] = alpha * sum + beta * C[i * n + j];
        }
    }
}

// Multi-threaded CPU implementation of GEMM
void gemm_cpu_multithreaded(const float *A, const float *B, float *C, 
                           float alpha, float beta, 
                           int m, int k, int n) {
    // Determine number of threads to use (adjust based on your CPU)
    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 4; // Fallback if hardware_concurrency() fails
    
    // Adjust if we have more threads than rows
    num_threads = std::min(num_threads, (unsigned int)m);
    
    std::vector<std::thread> threads(num_threads);
    
    // Launch threads
    for (unsigned int t = 0; t < num_threads; t++) {
        threads[t] = std::thread([=] {
            // Each thread processes a subset of rows
            int start_row = t * m / num_threads;
            int end_row = (t + 1) * m / num_threads;
            
            for (int i = start_row; i < end_row; i++) {
                for (int j = 0; j < n; j++) {
                    float sum = 0.0f;
                    for (int l = 0; l < k; l++) {
                        sum += A[i * k + l] * B[l * n + j];
                    }
                    C[i * n + j] = alpha * sum + beta * C[i * n + j];
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
}

// Cache-optimized CPU implementation
void gemm_cpu_optimized(const float *A, const float *B, float *C, 
                        float alpha, float beta, 
                        int m, int k, int n) {
    // Using a block size for cache optimization
    const int BLOCK_SIZE = 8;
    
    // Temporary storage for the result
    std::vector<float> C_temp(m * n, 0.0f);
    
    // First apply beta to C
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            C_temp[i * n + j] = beta * C[i * n + j];
        }
    }
    
    // Blocked matrix multiplication for better cache locality
    for (int i0 = 0; i0 < m; i0 += BLOCK_SIZE) {
        for (int j0 = 0; j0 < n; j0 += BLOCK_SIZE) {
            for (int k0 = 0; k0 < k; k0 += BLOCK_SIZE) {
                // Process blocks
                for (int i = i0; i < std::min(i0 + BLOCK_SIZE, m); i++) {
                    for (int j = j0; j < std::min(j0 + BLOCK_SIZE, n); j++) {
                        float sum = 0.0f;
                        for (int l = k0; l < std::min(k0 + BLOCK_SIZE, k); l++) {
                            sum += A[i * k + l] * B[l * n + j];
                        }
                        C_temp[i * n + j] += alpha * sum;
                    }
                }
            }
        }
    }
    
    // Copy back to C
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            C[i * n + j] = C_temp[i * n + j];
        }
    }
}

int main() {
    std::cout << "CPU Implementation: Heavy Computation GEMM Test" << std::endl;
    std::cout << "Running " << NUM_ITERATIONS << " iterations of " 
              << M_SIZE << "x" << K_SIZE << " * " << K_SIZE << "x" << N_SIZE 
              << " matrix multiplication" << std::endl;
    std::cout << std::string(60, '-') << "\n";
    
    // Allocate memory for matrices
    std::vector<float> A(M_SIZE * K_SIZE);
    std::vector<float> B(K_SIZE * N_SIZE);
    std::vector<float> C_basic(M_SIZE * N_SIZE);
    std::vector<float> C_optimized(M_SIZE * N_SIZE);
    std::vector<float> C_multithreaded(M_SIZE * N_SIZE);
    std::vector<float> C_expected(M_SIZE * N_SIZE);
    
    std::cout << "Initializing matrices...\n";
    
    // Initialize matrices with the same test values as the FPGA implementation
    for (int i = 0; i < M_SIZE; i++) {
        for (int j = 0; j < K_SIZE; j++) {
            A[i * K_SIZE + j] = (i + j) * 0.1f;
        }
    }
    
    for (int i = 0; i < K_SIZE; i++) {
        for (int j = 0; j < N_SIZE; j++) {
            B[i * N_SIZE + j] = (i * j) * 0.01f;
        }
    }
    
    // Initialize all C matrices with the same values
    for (int i = 0; i < M_SIZE; i++) {
        for (int j = 0; j < N_SIZE; j++) {
            float initial_value = i - j;
            C_basic[i * N_SIZE + j] = initial_value;
            C_optimized[i * N_SIZE + j] = initial_value;
            C_multithreaded[i * N_SIZE + j] = initial_value;
            C_expected[i * N_SIZE + j] = initial_value;
        }
    }
    
    // GEMM parameters - match those in FPGA implementation
    float alpha = 1.5f;
    float beta = 0.8f;
    
    // Compute reference result for first iteration only (for verification)
    std::cout << "Computing reference results...\n";
    gemm_cpu(A.data(), B.data(), C_expected.data(), alpha, beta, M_SIZE, K_SIZE, N_SIZE);
    
    // Run and time basic CPU implementation with multiple iterations
    std::cout << "Running basic CPU implementation for " << NUM_ITERATIONS << " iterations...\n";
    auto start_basic = std::chrono::high_resolution_clock::now();
    
    for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
        gemm_cpu(A.data(), B.data(), C_basic.data(), alpha, beta, M_SIZE, K_SIZE, N_SIZE);
    }
    
    auto end_basic = std::chrono::high_resolution_clock::now();
    auto duration_basic_ms = std::chrono::duration<double, std::milli>(end_basic - start_basic).count();
    
    std::cout << "Basic CPU implementation completed " << NUM_ITERATIONS << " iterations in " 
              << duration_basic_ms << " ms" << std::endl;
    std::cout << "Average time per iteration: " << duration_basic_ms / NUM_ITERATIONS << " ms" << std::endl;
    
    // Run and time optimized CPU implementation with multiple iterations
    std::cout << "\nRunning optimized CPU implementation for " << NUM_ITERATIONS << " iterations...\n";
    auto start_opt = std::chrono::high_resolution_clock::now();
    
    for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
        gemm_cpu_optimized(A.data(), B.data(), C_optimized.data(), alpha, beta, M_SIZE, K_SIZE, N_SIZE);
    }
    
    auto end_opt = std::chrono::high_resolution_clock::now();
    auto duration_opt_ms = std::chrono::duration<double, std::milli>(end_opt - start_opt).count();
    
    std::cout << "Optimized CPU implementation completed " << NUM_ITERATIONS << " iterations in " 
              << duration_opt_ms << " ms" << std::endl;
    std::cout << "Average time per iteration: " << duration_opt_ms / NUM_ITERATIONS << " ms" << std::endl;
    
    // Run and time multi-threaded CPU implementation with multiple iterations
    std::cout << "\nRunning multi-threaded CPU implementation for " << NUM_ITERATIONS << " iterations...\n";
    std::cout << "Using " << std::thread::hardware_concurrency() << " threads" << std::endl;
    auto start_mt = std::chrono::high_resolution_clock::now();
    
    for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
        gemm_cpu_multithreaded(A.data(), B.data(), C_multithreaded.data(), alpha, beta, M_SIZE, K_SIZE, N_SIZE);
    }
    
    auto end_mt = std::chrono::high_resolution_clock::now();
    auto duration_mt_ms = std::chrono::duration<double, std::milli>(end_mt - start_mt).count();
    
    std::cout << "Multi-threaded CPU implementation completed " << NUM_ITERATIONS << " iterations in " 
              << duration_mt_ms << " ms" << std::endl;
    std::cout << "Average time per iteration: " << duration_mt_ms / NUM_ITERATIONS << " ms" << std::endl;
    
    // Calculate and display performance metrics
    // For GEMM, operations = 2*M*N*K (MNK multiply-adds) per iteration
    double operations_per_iter = 2.0 * M_SIZE * N_SIZE * K_SIZE;
    double total_operations = operations_per_iter * NUM_ITERATIONS;
    
    double gflops_basic = (total_operations / 1.0e9) / (duration_basic_ms / 1000.0);
    double gflops_opt = (total_operations / 1.0e9) / (duration_opt_ms / 1000.0);
    double gflops_mt = (total_operations / 1.0e9) / (duration_mt_ms / 1000.0);
    
    std::cout << "\n" << std::string(60, '-') << "\n";
    std::cout << "CPU Performance Comparison:\n";
    std::cout << std::string(60, '-') << "\n";
    std::cout << "Matrix dimensions: A(" << M_SIZE << "x" << K_SIZE 
              << ") * B(" << K_SIZE << "x" << N_SIZE << ")\n";
    std::cout << "Number of iterations: " << NUM_ITERATIONS << "\n";
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Basic CPU implementation:         " << duration_basic_ms << " ms, " 
              << gflops_basic << " GFLOPS\n";
    std::cout << "Optimized CPU implementation:     " << duration_opt_ms << " ms, " 
              << gflops_opt << " GFLOPS\n";
    std::cout << "Multi-threaded CPU implementation: " << duration_mt_ms << " ms, " 
              << gflops_mt << " GFLOPS\n";
    std::cout << std::string(60, '-') << "\n";
    
    // Section for FPGA results (to be filled in manually)
    std::cout << "FPGA implementation (enter manually): [Time] ms, [GFLOPS]\n";
    std::cout << std::string(60, '-') << "\n";
    
    // Space for adding speedup comparison after running both implementations
    std::cout << "\nTo compare speedup, update with FPGA results after running both tests.\n";
    
    return 0;
}