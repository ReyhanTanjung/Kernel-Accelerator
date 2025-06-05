#include <iostream>
#include <vector>
#include <xrt.h>
#include <experimental/xrt_kernel.h>
#include <chrono>
#include <cmath>
#include "conv2d.h"

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
    std::cout << "Perbandingan Performa Konvolusi 2D: CPU vs FPGA\n";
    std::cout << "================================\n";
    
    // Inisialisasi data
    const int input_size = TEST_HEIGHT * TEST_WIDTH;
    const int kernel_size_sq = TEST_KERNEL_SIZE * TEST_KERNEL_SIZE;
    const int output_height = TEST_HEIGHT - TEST_KERNEL_SIZE + 1;
    const int output_width = TEST_WIDTH - TEST_KERNEL_SIZE + 1;
    const int output_size = output_height * output_width;
    
    std::vector<float> input(input_size);
    std::vector<float> kernel(kernel_size_sq);
    std::vector<float> output_fpga(output_size);
    std::vector<float> output_cpu(output_size);
    
    // Inisialisasi data input
    for (int i = 0; i < input_size; i++) {
        input[i] = static_cast<float>(i % 256) / 256.0f;
    }
    
    // Inisialisasi kernel (Gaussian-like)
    kernel[0] = 1.0f/16.0f; kernel[1] = 2.0f/16.0f; kernel[2] = 1.0f/16.0f;
    kernel[3] = 2.0f/16.0f; kernel[4] = 4.0f/16.0f; kernel[5] = 2.0f/16.0f;
    kernel[6] = 1.0f/16.0f; kernel[7] = 2.0f/16.0f; kernel[8] = 1.0f/16.0f;
    
    // ===== Pengujian performa CPU =====
    std::cout << "\n[CPU] Menjalankan konvolusi 2D pada CPU...\n";
    auto cpu_start = std::chrono::high_resolution_clock::now();
    
    // Eksekusi konvolusi pada CPU
    conv2d_cpu(input.data(), kernel.data(), output_cpu.data(), TEST_HEIGHT, TEST_WIDTH, TEST_KERNEL_SIZE);
    
    auto cpu_end = std::chrono::high_resolution_clock::now();
    double cpu_duration_ms = std::chrono::duration<double, std::milli>(cpu_end - cpu_start).count();
    
    std::cout << "[CPU] Waktu eksekusi: " << cpu_duration_ms << " ms\n";
    
    // ===== Pengujian performa FPGA =====
    std::cout << "\n[FPGA] Melakukan setup XRT...\n";
    
    try {
        // Setup XRT device dan kernel
        auto device = xrt::device(0);
        auto uuid = device.load_xclbin("conv2d.xclbin");
        auto kernel_conv2d = xrt::kernel(device, uuid, "conv2d", xrt::kernel::cu_access_mode::exclusive);
        
        // Alokasi buffer untuk input, kernel, dan output
        auto input_buf = xrt::bo(device, input.size() * sizeof(float), kernel_conv2d.group_id(0));
        auto kernel_buf = xrt::bo(device, kernel.size() * sizeof(float), kernel_conv2d.group_id(1));
        auto output_buf = xrt::bo(device, output_fpga.size() * sizeof(float), kernel_conv2d.group_id(2));
        
        // Map buffer objects ke host memory
        auto input_map = input_buf.map<float*>();
        auto kernel_map = kernel_buf.map<float*>();
        auto output_map = output_buf.map<float*>();
        
        // Salin data ke mapped memory
        for (size_t i = 0; i < input.size(); i++) {
            input_map[i] = input[i];
        }
        
        for (size_t i = 0; i < kernel.size(); i++) {
            kernel_map[i] = kernel[i];
        }
        
        // Sinkronisasi buffer dengan device memory
        std::cout << "[FPGA] Sinkronisasi buffer ke device memory...\n";
        input_buf.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        kernel_buf.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        
        std::cout << "[FPGA] Menjalankan kernel pada FPGA...\n";
        auto fpga_start = std::chrono::high_resolution_clock::now();
        
        // Eksekusi kernel FPGA
        auto run = kernel_conv2d(input_buf, kernel_buf, output_buf, TEST_HEIGHT, TEST_WIDTH, TEST_KERNEL_SIZE);
        run.wait();
        
        auto fpga_end = std::chrono::high_resolution_clock::now();
        double fpga_duration_ms = std::chrono::duration<double, std::milli>(fpga_end - fpga_start).count();
        
        // Ambil data output
        std::cout << "[FPGA] Mengambil hasil dari device...\n";
        output_buf.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        
        // Salin hasil ke vector output_fpga
        for (size_t i = 0; i < output_fpga.size(); i++) {
            output_fpga[i] = output_map[i];
        }
        
        // Verifikasi hasil
        bool error = false;
        float max_diff = 0.0f;
        int error_idx = -1;
        
        for (size_t i = 0; i < output_size; i++) {
            float diff = std::fabs(output_fpga[i] - output_cpu[i]);
            if (diff > max_diff) {
                max_diff = diff;
                error_idx = i;
            }
            
            if (diff > 1e-5) {
                error = true;
                if (i < 5) {  // Tampilkan beberapa error pertama saja
                    std::cout << "Error pada indeks " << i << ": " 
                              << output_fpga[i] << " (FPGA) vs " 
                              << output_cpu[i] << " (CPU), "
                              << "diff = " << diff << std::endl;
                }
            }
        }
        
        if (!error) {
            std::cout << "[FPGA] Verifikasi BERHASIL! Hasil konvolusi FPGA sesuai dengan CPU.\n";
        } else {
            std::cout << "[FPGA] Verifikasi GAGAL! Perbedaan maksimum = " << max_diff 
                      << " pada indeks " << error_idx << std::endl;
        }
        
        std::cout << "[FPGA] Waktu eksekusi: " << fpga_duration_ms << " ms\n";
        
        // ===== Analisis Performa =====
        
        // Hitung jumlah operasi
        // Untuk setiap piksel output, kita melakukan kernel_size^2 perkalian dan akumulasi
        double total_ops = 2.0 * output_height * output_width * TEST_KERNEL_SIZE * TEST_KERNEL_SIZE;
        
        double cpu_ops_per_second = total_ops / (cpu_duration_ms / 1000.0);
        double fpga_ops_per_second = total_ops / (fpga_duration_ms / 1000.0);
        
        double cpu_gops = cpu_ops_per_second / 1e9;
        double fpga_gops = fpga_ops_per_second / 1e9;
        
        std::cout << "\n===== Perbandingan Performa =====\n";
        std::cout << "Ukuran input: " << TEST_HEIGHT << "x" << TEST_WIDTH << std::endl;
        std::cout << "Ukuran kernel: " << TEST_KERNEL_SIZE << "x" << TEST_KERNEL_SIZE << std::endl;
        std::cout << "Ukuran output: " << output_height << "x" << output_width << std::endl;
        std::cout << "Total operasi: " << total_ops << " (multiply-accumulate)\n";
        std::cout << "CPU: " << cpu_gops << " GOPS (Giga Operations Per Second)\n";
        std::cout << "FPGA: " << fpga_gops << " GOPS (Giga Operations Per Second)\n";
        std::cout << "Speedup FPGA vs CPU: " << (fpga_gops / cpu_gops) << "x\n";
        
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        std::cerr << "Kemungkinan bitstream FPGA belum tersedia atau setup XRT belum tepat.\n";
        std::cerr << "Menjalankan pengujian CPU saja.\n";
        
        // Hitung performa CPU saja
        double total_ops = 2.0 * output_height * output_width * TEST_KERNEL_SIZE * TEST_KERNEL_SIZE;
        double cpu_ops_per_second = total_ops / (cpu_duration_ms / 1000.0);
        double cpu_gops = cpu_ops_per_second / 1e9;
        
        std::cout << "\n===== Hasil Performa CPU =====\n";
        std::cout << "Ukuran input: " << TEST_HEIGHT << "x" << TEST_WIDTH << std::endl;
        std::cout << "Ukuran kernel: " << TEST_KERNEL_SIZE << "x" << TEST_KERNEL_SIZE << std::endl;
        std::cout << "Ukuran output: " << output_height << "x" << output_width << std::endl;
        std::cout << "Total operasi: " << total_ops << " (multiply-accumulate)\n";
        std::cout << "CPU: " << cpu_gops << " GOPS (Giga Operations Per Second)\n";
    }
    
    return 0;
}