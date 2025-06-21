#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>

// Ukuran data untuk pengujian performa
#define TEST_HEIGHT 64
#define TEST_WIDTH 64
#define TEST_KERNEL_SIZE 3

// Konvolusi 2D referensi pada CPU
void conv2d_cpu(
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

int main() {
    std::cout << "================================\n";
    std::cout << "Benchmark Konvolusi 2D di CPU\n";
    std::cout << "================================\n";

    // Inisialisasi data
    const int input_size = TEST_HEIGHT * TEST_WIDTH;
    const int kernel_size_sq = TEST_KERNEL_SIZE * TEST_KERNEL_SIZE;
    const int output_height = TEST_HEIGHT - TEST_KERNEL_SIZE + 1;
    const int output_width = TEST_WIDTH - TEST_KERNEL_SIZE + 1;
    const int output_size = output_height * output_width;

    std::vector<float> input(input_size);
    std::vector<float> kernel(kernel_size_sq);
    std::vector<float> output(output_size);

    // Inisialisasi input
    for (int i = 0; i < input_size; i++) {
        input[i] = static_cast<float>(i % 256) / 256.0f;
    }

    // Kernel Gaussian-like 3x3
    kernel[0] = 1.0f/16.0f; kernel[1] = 2.0f/16.0f; kernel[2] = 1.0f/16.0f;
    kernel[3] = 2.0f/16.0f; kernel[4] = 4.0f/16.0f; kernel[5] = 2.0f/16.0f;
    kernel[6] = 1.0f/16.0f; kernel[7] = 2.0f/16.0f; kernel[8] = 1.0f/16.0f;

    std::cout << "[CPU] Menjalankan konvolusi 2D...\n";
    auto start = std::chrono::high_resolution_clock::now();

    conv2d_cpu(input.data(), kernel.data(), output.data(), TEST_HEIGHT, TEST_WIDTH, TEST_KERNEL_SIZE);

    auto end = std::chrono::high_resolution_clock::now();
    double cpu_duration_ms = std::chrono::duration<double, std::milli>(end - start).count();

    // Hitung total operasi: 2 operasi (perkalian + penjumlahan) per elemen kernel untuk setiap output pixel
    double total_ops = 2.0 * output_height * output_width * TEST_KERNEL_SIZE * TEST_KERNEL_SIZE;
    double cpu_ops_per_second = total_ops / (cpu_duration_ms / 1000.0);
    double cpu_gops = cpu_ops_per_second / 1e9;

    std::cout << "\n===== Hasil Performa CPU =====\n";
    std::cout << "Ukuran input: " << TEST_HEIGHT << "x" << TEST_WIDTH << std::endl;
    std::cout << "Ukuran kernel: " << TEST_KERNEL_SIZE << "x" << TEST_KERNEL_SIZE << std::endl;
    std::cout << "Ukuran output: " << output_height << "x" << output_width << std::endl;
    std::cout << "Total operasi: " << total_ops << " (multiply-accumulate)\n";
    std::cout << "CPU: " << cpu_gops << " GOPS (Giga Operations Per Second)\n";
    std::cout << "Waktu eksekusi CPU: " << cpu_duration_ms << " ms\n";

    return 0;
}
