#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <iomanip> // for std::setprecision

#define MAX_INPUT_SIZE 1024
#define MAX_OUTPUT_SIZE 512

int main() {
    const int input_size = 128;
    const int output_size = 64;

    std::vector<float> input(input_size, 0.5f);
    std::vector<float> weights(input_size * output_size, 0.1f);
    std::vector<float> output(output_size, 0.0f);
    std::vector<float> golden(output_size);

    // Calculate golden reference
    for (int o = 0; o < output_size; o++) {
        float sum = 0.0f;
        for (int i = 0; i < input_size; i++) {
            sum += input[i] * weights[o * input_size + i];
        }
        golden[o] = sum;
    }

    std::cout << "Running fully_connected on CPU...\n";
    auto start = std::chrono::high_resolution_clock::now();

    // CPU-based matrix-vector multiplication
    for (int o = 0; o < output_size; o++) {
        float sum = 0.0f;
        for (int i = 0; i < input_size; i++) {
            sum += input[i] * weights[o * input_size + i];
        }
        output[o] = sum;
    }

    auto end = std::chrono::high_resolution_clock::now();
    double exec_ms = std::chrono::duration<double, std::milli>(end - start).count();

    // Verification
    bool match = true;
    std::cout << std::fixed << std::setprecision(6);
    for (int i = 0; i < output_size; i++) {
        std::cout << "Output[" << i << "] = " << output[i]
                  << ", Golden[" << i << "] = " << golden[i] << "\n";
        if (std::abs(output[i] - golden[i]) > 1e-3f) {
            std::cout << "  >> Mismatch at index " << i << "\n";
            match = false;
        }
    }

    if (match)
        std::cout << "Verification PASSED.\n";
    else
        std::cout << "Verification FAILED.\n";

    std::cout << "Execution time: " << exec_ms << " ms\n";

    // Throughput (same formula as FPGA version)
    double total_bytes = (input_size + input_size * output_size + output_size) * sizeof(float);
    double gb = total_bytes / (1024.0 * 1024.0 * 1024.0);
    double throughput = gb / (exec_ms / 1000.0);

    std::cout << "Data size: " << gb << " GB\n";
    std::cout << "Throughput: " << throughput << " GB/s\n";

    return 0;
}
