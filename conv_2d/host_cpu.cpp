#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <omp.h>

// Konstanta untuk dimensi maksimum
#define MAX_IMAGE_HEIGHT 64
#define MAX_IMAGE_WIDTH 64
#define MAX_KERNEL_SIZE 7

// Ukuran untuk pengujian
#define TEST_HEIGHT 64
#define TEST_WIDTH 64
#define TEST_KERNEL_SIZE 3

// Implementasi naif convolution 2D (single-threaded)
void conv2d_naive(
    const float* input,
    const float* kernel,
    float* output,
    int height,
    int width,
    int kernel_size
) {
    int output_height = height - kernel_size + 1;
    int output_width = width - kernel_size + 1;
    
    for (int y = 0; y < output_height; y++) {
        for (int x = 0; x < output_width; x++) {
            float sum = 0.0f;
            
            for (int ky = 0; ky < kernel_size; ky++) {
                for (int kx = 0; kx < kernel_size; kx++) {
                    int input_y = y + ky;
                    int input_x = x + kx;
                    sum += input[input_y * width + input_x] * kernel[ky * kernel_size + kx];
                }
            }
            
            output[y * output_width + x] = sum;
        }
    }
}

// Implementasi convolution 2D dengan optimasi memori (cache-friendly)
void conv2d_optimized(
    const float* input,
    const float* kernel,
    float* output,
    int height,
    int width,
    int kernel_size
) {
    int output_height = height - kernel_size + 1;
    int output_width = width - kernel_size + 1;
    
    // Salin kernel ke array lokal untuk akses memori yang lebih baik
    float local_kernel[MAX_KERNEL_SIZE * MAX_KERNEL_SIZE];
    for (int ky = 0; ky < kernel_size; ky++) {
        for (int kx = 0; kx < kernel_size; kx++) {
            local_kernel[ky * kernel_size + kx] = kernel[ky * kernel_size + kx];
        }
    }
    
    // Proses baris demi baris dengan akses memori yang terurut
    for (int y = 0; y < output_height; y++) {
        for (int x = 0; x < output_width; x++) {
            float sum = 0.0f;
            
            for (int ky = 0; ky < kernel_size; ky++) {
                const float* input_row = &input[(y + ky) * width + x];
                const float* kernel_row = &local_kernel[ky * kernel_size];
                
                // Optimasi inner loop dengan akses memori berurutan
                for (int kx = 0; kx < kernel_size; kx++) {
                    sum += input_row[kx] * kernel_row[kx];
                }
            }
            
            output[y * output_width + x] = sum;
        }
    }
}

// Implementasi convolution 2D dengan OpenMP (multithreaded)
void conv2d_openmp(
    const float* input,
    const float* kernel,
    float* output,
    int height,
    int width,
    int kernel_size
) {
    int output_height = height - kernel_size + 1;
    int output_width = width - kernel_size + 1;
    
    // Salin kernel ke array lokal untuk akses memori yang lebih baik
    float local_kernel[MAX_KERNEL_SIZE * MAX_KERNEL_SIZE];
    for (int ky = 0; ky < kernel_size; ky++) {
        for (int kx = 0; kx < kernel_size; kx++) {
            local_kernel[ky * kernel_size + kx] = kernel[ky * kernel_size + kx];
        }
    }
    
    // Paralelisasi dengan OpenMP pada loop luar
    #pragma omp parallel for collapse(2)
    for (int y = 0; y < output_height; y++) {
        for (int x = 0; x < output_width; x++) {
            float sum = 0.0f;
            
            for (int ky = 0; ky < kernel_size; ky++) {
                const float* input_row = &input[(y + ky) * width + x];
                const float* kernel_row = &local_kernel[ky * kernel_size];
                
                for (int kx = 0; kx < kernel_size; kx++) {
                    sum += input_row[kx] * kernel_row[kx];
                }
            }
            
            output[y * output_width + x] = sum;
        }
    }
}

// Fungsi untuk mencetak statistik performa
void print_performance_stats(const std::string& method_name, double duration_ms, int height, int width, int kernel_size) {
    std::cout << "----------------------------------------\n";
    std::cout << method_name << ":\n";
    std::cout << "Waktu eksekusi: " << duration_ms << " ms\n";
    
    // Hitung output dimensions
    int output_height = height - kernel_size + 1;
    int output_width = width - kernel_size + 1;
    
    // Hitung jumlah operasi
    // Untuk setiap piksel output, kita melakukan kernel_size * kernel_size kali multiply-accumulate
    double total_ops = 2.0 * output_height * output_width * kernel_size * kernel_size;
    double ops_per_second = total_ops / (duration_ms / 1000.0);
    double gops_per_second = ops_per_second / 1e9;
    
    // Data movement in bytes (input, kernel, output)
    double input_size_bytes = height * width * sizeof(float);
    double kernel_size_bytes = kernel_size * kernel_size * sizeof(float);
    double output_size_bytes = output_height * output_width * sizeof(float);
    double total_data_bytes = input_size_bytes + kernel_size_bytes + output_size_bytes;
    double total_data_gb = total_data_bytes / (1024 * 1024 * 1024);
    
    // Hitung bandwidth memory
    double bandwidth_gbps = total_data_gb / (duration_ms / 1000.0);
    
    std::cout << "Input dimensions: " << height << "x" << width << std::endl;
    std::cout << "Kernel size: " << kernel_size << "x" << kernel_size << std::endl;
    std::cout << "Output dimensions: " << output_height << "x" << output_width << std::endl;
    std::cout << "Total operasi: " << std::fixed << std::setprecision(0) << total_ops << " (multiply-accumulate)\n";
    std::cout << "Performa: " << std::fixed << std::setprecision(4) << gops_per_second << " GOPS (Giga Operations Per Second)\n";
    std::cout << "Data movement: " << std::fixed << std::setprecision(6) << total_data_gb << " GB\n";
    std::cout << "Memory bandwidth: " << std::fixed << std::setprecision(4) << bandwidth_gbps << " GB/s\n";
}

// Fungsi untuk memverifikasi dua output adalah sama
bool verify_outputs(const float* output1, const float* output2, int size, float epsilon = 1e-5) {
    for (int i = 0; i < size; i++) {
        if (std::fabs(output1[i] - output2[i]) > epsilon) {
            std::cout << "Perbedaan pada indeks " << i << ": " 
                      << output1[i] << " vs " << output2[i] 
                      << " (diff = " << std::fabs(output1[i] - output2[i]) << ")\n";
            return false;
        }
    }
    return true;
}

// Fungsi untuk menjalankan benchmarking
void run_benchmark(int height, int width, int kernel_size, int num_iterations = 10, int warmup_iterations = 3) {
    // Alokasi memori untuk data uji
    std::vector<float> input(height * width);
    std::vector<float> kernel(kernel_size * kernel_size);
    
    // Hitung dimensi output
    int output_height = height - kernel_size + 1;
    int output_width = width - kernel_size + 1;
    int output_size = output_height * output_width;
    
    // Alokasi memori untuk output dari berbagai implementasi
    std::vector<float> output_naive(output_size);
    std::vector<float> output_optimized(output_size);
    std::vector<float> output_openmp(output_size);
    
    // Inisialisasi input dengan pola tes
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;
            input[idx] = static_cast<float>(idx % 16) / 16.0f;
        }
    }
    
    // Inisialisasi kernel dengan filter sederhana (Gaussian-like)
    kernel[0] = 1.0f/16.0f; kernel[1] = 2.0f/16.0f; kernel[2] = 1.0f/16.0f;
    kernel[3] = 2.0f/16.0f; kernel[4] = 4.0f/16.0f; kernel[5] = 2.0f/16.0f;
    kernel[6] = 1.0f/16.0f; kernel[7] = 2.0f/16.0f; kernel[8] = 1.0f/16.0f;
    
    // Pemanasan sistem
    std::cout << "Warming up...\n";
    for (int i = 0; i < warmup_iterations; i++) {
        conv2d_naive(input.data(), kernel.data(), output_naive.data(), height, width, kernel_size);
        conv2d_optimized(input.data(), kernel.data(), output_optimized.data(), height, width, kernel_size);
        conv2d_openmp(input.data(), kernel.data(), output_openmp.data(), height, width, kernel_size);
    }
    
    // Benchmarking Implementasi Naif
    std::cout << "Benchmarking naive implementation...\n";
    auto start_naive = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_iterations; i++) {
        conv2d_naive(input.data(), kernel.data(), output_naive.data(), height, width, kernel_size);
    }
    auto end_naive = std::chrono::high_resolution_clock::now();
    double duration_naive_ms = std::chrono::duration<double, std::milli>(end_naive - start_naive).count() / num_iterations;
    
    // Benchmarking Implementasi Optimasi Cache
    std::cout << "Benchmarking cache-optimized implementation...\n";
    auto start_optimized = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_iterations; i++) {
        conv2d_optimized(input.data(), kernel.data(), output_optimized.data(), height, width, kernel_size);
    }
    auto end_optimized = std::chrono::high_resolution_clock::now();
    double duration_optimized_ms = std::chrono::duration<double, std::milli>(end_optimized - start_optimized).count() / num_iterations;
    
    // Benchmarking Implementasi OpenMP
    std::cout << "Benchmarking OpenMP implementation...\n";
    auto start_openmp = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_iterations; i++) {
        conv2d_openmp(input.data(), kernel.data(), output_openmp.data(), height, width, kernel_size);
    }
    auto end_openmp = std::chrono::high_resolution_clock::now();
    double duration_openmp_ms = std::chrono::duration<double, std::milli>(end_openmp - start_openmp).count() / num_iterations;
    
    // Verifikasi hasil
    std::cout << "Verifying results...\n";
    bool naive_vs_optimized = verify_outputs(output_naive.data(), output_optimized.data(), output_size);
    bool naive_vs_openmp = verify_outputs(output_naive.data(), output_openmp.data(), output_size);
    
    if (naive_vs_optimized && naive_vs_openmp) {
        std::cout << "All implementations produce the same output.\n";
    } else {
        std::cout << "WARNING: Implementations produce different outputs!\n";
    }
    
    // Tampilkan statistik performa
    print_performance_stats("CPU (Naive Implementation)", duration_naive_ms, height, width, kernel_size);
    print_performance_stats("CPU (Cache-Optimized)", duration_optimized_ms, height, width, kernel_size);
    print_performance_stats("CPU (OpenMP Multi-threaded)", duration_openmp_ms, height, width, kernel_size);
    
    // Perkiraan performa FPGA (berdasarkan implementasi HLS)
    // Nilai ini harus diperbarui dengan hasil aktual dari benchmarking FPGA
    double estimated_fpga_duration_ms = 0.1 * duration_openmp_ms; // Asumsi FPGA 10x lebih cepat dari OpenMP
    std::cout << "\n----------------------------------------\n";
    std::cout << "Perbandingan dengan FPGA (estimasi):\n";
    std::cout << "Estimasi waktu FPGA: " << estimated_fpga_duration_ms << " ms\n";
    std::cout << "Estimasi speedup vs CPU (naive): " << duration_naive_ms / estimated_fpga_duration_ms << "x\n";
    std::cout << "Estimasi speedup vs CPU (optimized): " << duration_optimized_ms / estimated_fpga_duration_ms << "x\n";
    std::cout << "Estimasi speedup vs CPU (OpenMP): " << duration_openmp_ms / estimated_fpga_duration_ms << "x\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Catatan: Nilai FPGA di atas hanya estimasi.\n";
    std::cout << "Untuk hasil yang akurat, jalankan benchmark pada hardware FPGA yang sebenarnya.\n";
}

int main(int argc, char* argv[]) {
    // Default test parameters
    int height = TEST_HEIGHT;
    int width = TEST_WIDTH;
    int kernel_size = TEST_KERNEL_SIZE;
    int num_iterations = 100;
    
    // Parse command line arguments if provided
    if (argc > 1) height = std::stoi(argv[1]);
    if (argc > 2) width = std::stoi(argv[2]);
    if (argc > 3) kernel_size = std::stoi(argv[3]);
    if (argc > 4) num_iterations = std::stoi(argv[4]);
    
    // Validasi parameter
    if (height <= 0 || width <= 0 || kernel_size <= 0 || height > MAX_IMAGE_HEIGHT || width > MAX_IMAGE_WIDTH || kernel_size > MAX_KERNEL_SIZE) {
        std::cerr << "Invalid parameters. Please check your input values.\n";
        std::cerr << "Maximum allowed: height=" << MAX_IMAGE_HEIGHT << ", width=" << MAX_IMAGE_WIDTH << ", kernel_size=" << MAX_KERNEL_SIZE << "\n";
        return 1;
    }
    
    // Cetak informasi tentang sistem
    std::cout << "CPU Benchmark for 2D Convolution\n";
    std::cout << "================================\n";
    std::cout << "OpenMP max threads: " << omp_get_max_threads() << "\n";
    std::cout << "Running " << num_iterations << " iterations for each implementation\n\n";
    
    // Jalankan benchmark
    run_benchmark(height, width, kernel_size, num_iterations);
    
    return 0;
}