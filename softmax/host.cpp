#include <iostream>
#include <vector>
#include <xrt.h>
#include <experimental/xrt_kernel.h>
#include <chrono>
#include <cmath>

#define SIZE 1024  // Jumlah elemen

float softmax_cpu(const std::vector<float>& input, std::vector<float>& output) {
    // Cari nilai maksimum untuk stabilitas numerik
    float max_val = *std::max_element(input.begin(), input.end());
    
    // Hitung eksponensial dan jumlahnya
    float sum = 0.0f;
    std::vector<float> exp_values(input.size());
    
    for (size_t i = 0; i < input.size(); i++) {
        exp_values[i] = std::exp(input[i] - max_val);
        sum += exp_values[i];
    }
    
    // Normalisasi
    for (size_t i = 0; i < input.size(); i++) {
        output[i] = exp_values[i] / sum;
    }
    
    return sum;
}

int main() {
    // Inisialisasi data
    std::vector<float> input(SIZE), output_hw(SIZE), output_sw(SIZE);

    // Buat data input acak
    std::cout << "Generating random input data...\n";
    for (int i = 0; i < SIZE; i++) {
        input[i] = static_cast<float>(rand() % 100) / 10.0f;
    }

    // Jalankan Softmax di CPU untuk verifikasi
    std::cout << "Running softmax on CPU for verification...\n";
    softmax_cpu(input, output_sw);

    // Setup XRT device dan kernel
    std::cout << "Setting up FPGA device and softmax kernel...\n";
    auto device = xrt::device(0);
    auto uuid = device.load_xclbin("softmax_hw.xclbin");
    auto kernel = xrt::kernel(device, uuid, "softmax", xrt::kernel::cu_access_mode::exclusive);

    // Alokasi buffer
    auto input_buf = xrt::bo(device, SIZE * sizeof(float), kernel.group_id(0));
    auto output_buf = xrt::bo(device, SIZE * sizeof(float), kernel.group_id(1));

    // Map buffer ke memori host
    auto input_map = input_buf.map<float*>();
    auto output_map = output_buf.map<float*>();

    // Salin data ke buffer input
    for (int i = 0; i < SIZE; i++) {
        input_map[i] = input[i];
    }

    // Sinkronisasi buffer input ke device memory
    std::cout << "Syncing input buffer to device memory...\n";
    input_buf.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    // Eksekusi kernel
    std::cout << "Starting kernel execution...\n";
    auto start = std::chrono::high_resolution_clock::now();

    auto run = kernel(input_buf, output_buf, SIZE);
    run.wait();

    auto end = std::chrono::high_resolution_clock::now();
    double duration_ms = std::chrono::duration<double, std::milli>(end - start).count();

    // Ambil hasil
    std::cout << "Getting results from device...\n";
    output_buf.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

    // Simpan hasil ke vektor output_hw
    for (int i = 0; i < SIZE; i++) {
        output_hw[i] = output_map[i];
    }

    // Verifikasi hasil
    bool error = false;
    float max_diff = 0.0f;
    for (int i = 0; i < SIZE; i++) {
        float diff = std::abs(output_hw[i] - output_sw[i]);
        max_diff = std::max(max_diff, diff);
        if (diff > 1e-4) {
            std::cout << "Error at index " << i << ": ";
            std::cout << "HW = " << output_hw[i] << ", ";
            std::cout << "SW = " << output_sw[i] << ", ";
            std::cout << "diff = " << diff << std::endl;
            error = true;
            if (i > 10) break; // Tampilkan hanya beberapa error pertama
        }
    }

    // Verifikasi jumlah probabilitas = 1
    float hw_sum = 0.0f, sw_sum = 0.0f;
    for (int i = 0; i < SIZE; i++) {
        hw_sum += output_hw[i];
        sw_sum += output_sw[i];
    }

    if (!error) {
        std::cout << "Verification PASSED!\n";
        std::cout << "Maximum difference: " << max_diff << std::endl;
    } else {
        std::cout << "Verification FAILED!\n";
    }

    std::cout << "HW probabilities sum: " << hw_sum << std::endl;
    std::cout << "SW probabilities sum: " << sw_sum << std::endl;
    std::cout << "Kernel execution time: " << duration_ms << " ms\n";

    return 0;
}