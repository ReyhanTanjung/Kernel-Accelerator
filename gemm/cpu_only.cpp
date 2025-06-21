#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <cstring>

#define M_SIZE 32
#define K_SIZE 32
#define N_SIZE 32
#define NUM_ITERATIONS 1000

void gemm_cpu(float* A, float* B, float* C, float alpha, float beta, int M, int K, int N) {
    std::vector<float> C_temp(M * N);

    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            float acc = 0.0f;
            for (int k = 0; k < K; ++k) {
                acc += A[i * K + k] * B[k * N + j];
            }
            C_temp[i * N + j] = alpha * acc + beta * C[i * N + j];
        }
    }

    std::memcpy(C, C_temp.data(), M * N * sizeof(float));
}

int main() {
    std::vector<float> A(M_SIZE * K_SIZE);
    std::vector<float> B(K_SIZE * N_SIZE);
    std::vector<float> C(M_SIZE * N_SIZE);

    float alpha = 1.5f;
    float beta = 0.8f;

    // Initialize matrices (same as in FPGA test)
    for (int i = 0; i < M_SIZE; ++i)
        for (int j = 0; j < K_SIZE; ++j)
            A[i * K_SIZE + j] = (i + j) * 0.1f;

    for (int i = 0; i < K_SIZE; ++i)
        for (int j = 0; j < N_SIZE; ++j)
            B[i * N_SIZE + j] = (i * j) * 0.01f;

    for (int i = 0; i < M_SIZE; ++i)
        for (int j = 0; j < N_SIZE; ++j)
            C[i * N_SIZE + j] = i - j;

    std::cout << "Running CPU GEMM for " << NUM_ITERATIONS << " iterations...\n";
    auto start = std::chrono::high_resolution_clock::now();

    for (int iter = 0; iter < NUM_ITERATIONS; ++iter) {
        gemm_cpu(A.data(), B.data(), C.data(), alpha, beta, M_SIZE, K_SIZE, N_SIZE);
    }

    auto end = std::chrono::high_resolution_clock::now();
    double duration_ms = std::chrono::duration<double, std::milli>(end - start).count();

    double ops_per_iter = 2.0 * M_SIZE * N_SIZE * K_SIZE;
    double total_ops = ops_per_iter * NUM_ITERATIONS;
    double gflops = (total_ops / 1e9) / (duration_ms / 1000.0);

    double bytes_per_iter = (M_SIZE * K_SIZE + K_SIZE * N_SIZE + 2 * M_SIZE * N_SIZE) * sizeof(float);
    double total_bytes = bytes_per_iter * NUM_ITERATIONS;
    double bandwidth_gbps = (total_bytes / 1e9) / (duration_ms / 1000.0);

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "\nCPU Performance Metrics:\n";
    std::cout << "---------------------------------------------\n";
    std::cout << "Matrix size: A(" << M_SIZE << "x" << K_SIZE << ") * B(" << K_SIZE << "x" << N_SIZE << ")\n";
    std::cout << "Iterations: " << NUM_ITERATIONS << "\n";
    std::cout << "Total time: " << duration_ms << " ms\n";
    std::cout << "Avg time per iteration: " << duration_ms / NUM_ITERATIONS << " ms\n";
    std::cout << "GFLOPS: " << gflops << "\n";
    std::cout << "Bandwidth: " << bandwidth_gbps << " GB/s\n";
    std::cout << "---------------------------------------------\n";

    return 0;
}
