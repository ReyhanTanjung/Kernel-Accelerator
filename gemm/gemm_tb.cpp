// #include <iostream>
// #include <cstdlib>
// #include <cmath>
// #include "gemm.h"

// bool is_close(float a, float b, float atol=1e-5) {
//     return std::abs(a - b) < atol;
// }

// int main() {
//     // Alokasi memori untuk matriks
//     float A[M * K], B[K * N], C[M * N], C_expected[M * N];
    
//     // Inisialisasi matriks dengan nilai uji
//     for (int i = 0; i < M; i++) {
//         for (int j = 0; j < K; j++) {
//             A[i * K + j] = (i + j) * 0.1f;  // Pattern: (i+j)*0.1
//         }
//     }
    
//     for (int i = 0; i < K; i++) {
//         for (int j = 0; j < N; j++) {
//             B[i * N + j] = (i * j) * 0.01f;  // Pattern: (i*j)*0.01
//         }
//     }
    
//     for (int i = 0; i < M; i++) {
//         for (int j = 0; j < N; j++) {
//             C[i * N + j] = i - j;  // Pattern: i-j
//             C_expected[i * N + j] = C[i * N + j];  // Salin ke expected
//         }
//     }
    
//     // Set parameter GEMM
//     float alpha = 1.5f;
//     float beta = 0.8f;
    
//     // Hitung hasil yang diharapkan menggunakan metode referensi
//     for (int i = 0; i < M; i++) {
//         for (int j = 0; j < N; j++) {
//             float sum = 0.0f;
//             for (int l = 0; l < K; l++) {
//                 sum += A[i * K + l] * B[l * N + j];
//             }
//             C_expected[i * N + j] = alpha * sum + beta * C_expected[i * N + j];
//         }
//     }
    
//     // Panggil implementasi HLS
//     std::cout << "Running HLS GEMM implementation...\n";
//     gemm(A, B, C, alpha, beta, M, K, N);
    
//     // Verifikasi hasil
//     bool pass = true;
//     int error_count = 0;
//     for (int i = 0; i < M; i++) {
//         for (int j = 0; j < N; j++) {
//             if (!is_close(C[i * N + j], C_expected[i * N + j])) {
//                 std::cout << "Error at C[" << i << "][" << j << "]: " 
//                           << C[i * N + j] << " != " << C_expected[i * N + j] << std::endl;
//                 pass = false;
//                 error_count++;
                
//                 // Hanya tampilkan 10 error pertama
//                 if (error_count >= 10) {
//                     std::cout << "Too many errors, stopping verification.\n";
//                     i = M;  // keluar dari loop luar
//                     break;  // keluar dari loop dalam
//                 }
//             }
//         }
//     }
    
//     if (pass) {
//         std::cout << "Test PASSED! All results match.\n";
//         return 0;
//     } else {
//         std::cout << "Test FAILED with " << error_count << " errors.\n";
//         return 1;
//     }
// }

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <iomanip>
#include "gemm.h"

// Define the systolic array size for testbench validation
#define SYSTOLIC_SIZE 8

// For validating results
bool is_close(float a, float b, float atol=1e-5) {
    return std::abs(a - b) < atol;
}

// Reference GEMM implementation for verification
void gemm_reference(const float *A, const float *B, float *C,
                   float alpha, float beta,
                   int m, int k, int n) {
    // Apply beta to C
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            C[i * n + j] = beta * C[i * n + j];
        }
    }
    
    // Compute matrix multiplication and apply alpha
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            float sum = 0.0f;
            for (int l = 0; l < k; l++) {
                sum += A[i * k + l] * B[l * n + j];
            }
            C[i * n + j] += alpha * sum;
        }
    }
}

// Function to initialize matrices with test patterns
void initialize_matrices(float *A, float *B, float *C, int m, int k, int n) {
    // Initialize A with pattern (i+j)*0.1
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < k; j++) {
            A[i * k + j] = (i + j) * 0.1f;
        }
    }
    
    // Initialize B with pattern (i*j)*0.01
    for (int i = 0; i < k; i++) {
        for (int j = 0; j < n; j++) {
            B[i * n + j] = (i * j) * 0.01f;
        }
    }
    
    // Initialize C with pattern i-j
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            C[i * n + j] = i - j;
        }
    }
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

int main() {
    std::cout << "Starting Systolic Array GEMM Testbench..." << std::endl;
    
    // Allocate memory for matrices
    float *A = new float[M * K];
    float *B = new float[K * N];
    float *C = new float[M * N];
    float *C_expected = new float[M * N];
    
    // Initialize matrices with test patterns
    initialize_matrices(A, B, C, M, K, N);
    
    // Make a copy of C for reference calculation
    std::memcpy(C_expected, C, M * N * sizeof(float));
    
    // GEMM parameters
    float alpha = 1.5f;
    float beta = 0.8f;
    
    // Print test parameters
    std::cout << "Test parameters:" << std::endl;
    std::cout << "  Matrix dimensions: A(" << M << "x" << K << ") * B(" << K << "x" << N << ")" << std::endl;
    std::cout << "  Systolic array size: " << SYSTOLIC_SIZE << "x" << SYSTOLIC_SIZE << std::endl;
    std::cout << "  Alpha: " << alpha << ", Beta: " << beta << std::endl;
    std::cout << std::endl;
    
    // Print small sections of input matrices for verification
    print_matrix_section("A", A, M, K);
    print_matrix_section("B", B, K, N);
    print_matrix_section("C (initial)", C, M, N);
    
    // Compute expected results using reference implementation
    std::cout << "Computing reference results..." << std::endl;
    gemm_reference(A, B, C_expected, alpha, beta, M, K, N);
    
    // Print small section of expected result for verification
    print_matrix_section("C_expected", C_expected, M, N);
    
    // Call the systolic array GEMM implementation
    std::cout << "Running systolic array GEMM implementation..." << std::endl;
    gemm(A, B, C, alpha, beta, M, K, N);
    
    // Print small section of result for verification
    print_matrix_section("C (result)", C, M, N);
    
    // Verify results
    std::cout << "Verifying results..." << std::endl;
    bool pass = true;
    int error_count = 0;
    float max_error = 0.0f;
    float avg_error = 0.0f;
    
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            float error = std::abs(C[i * N + j] - C_expected[i * N + j]);
            avg_error += error;
            max_error = std::max(max_error, error);
            
            if (!is_close(C[i * N + j], C_expected[i * N + j])) {
                if (error_count < 10) {
                    std::cout << "Error at C[" << i << "][" << j << "]: " 
                              << C[i * N + j] << " vs expected " << C_expected[i * N + j]
                              << ", diff = " << error << std::endl;
                }
                pass = false;
                error_count++;
            }
        }
    }
    
    // Calculate average error
    avg_error /= (M * N);
    
    // Output test results
    if (pass) {
        std::cout << "TEST PASSED! All results match." << std::endl;
    } else {
        std::cout << "TEST FAILED with " << error_count << " errors." << std::endl;
        std::cout << "Maximum error: " << max_error << std::endl;
        std::cout << "Average error: " << avg_error << std::endl;
    }
    
    // Free allocated memory
    delete[] A;
    delete[] B;
    delete[] C;
    delete[] C_expected;
    
    return pass ? 0 : 1;
}