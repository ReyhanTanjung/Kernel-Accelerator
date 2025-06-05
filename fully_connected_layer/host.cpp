#include <iostream>
#include <vector>
#include <xrt/xrt_device.h>
#include <xrt/xrt_kernel.h>
#include <chrono>
#include <iomanip> // for std::setprecision

#define MAX_INPUT_SIZE 1024
#define MAX_OUTPUT_SIZE 512

int main() {
    const int input_size = 128;
    const int output_size = 64;

    std::vector<float> input(input_size, 0.5f);
    std::vector<float> weights(input_size * output_size, 0.1f);
    std::vector<float> output(output_size);
    std::vector<float> golden(output_size);

    // Calculate golden result for verification
    for (int o = 0; o < output_size; o++) {
        float sum = 0.0f;
        for (int i = 0; i < input_size; i++) {
            sum += input[i] * weights[o * input_size + i];
        }
        golden[o] = sum;
    }

    // Load device and kernel
    auto device = xrt::device(0);
    auto uuid = device.load_xclbin("fully_connected.xclbin");
    auto kernel = xrt::kernel(device, uuid, "fully_connected");

    // Allocate buffers
    auto input_bo = xrt::bo(device, input_size * sizeof(float), kernel.group_id(0));
    auto weights_bo = xrt::bo(device, input_size * output_size * sizeof(float), kernel.group_id(1));
    auto output_bo = xrt::bo(device, output_size * sizeof(float), kernel.group_id(2));

    // Map buffers to host
    auto input_map = input_bo.map<float*>();
    auto weights_map = weights_bo.map<float*>();
    auto output_map = output_bo.map<float*>();

    // Copy data into mapped buffers
    std::copy(input.begin(), input.end(), input_map);
    std::copy(weights.begin(), weights.end(), weights_map);

    // Sync input buffers to device
    input_bo.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    weights_bo.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    std::cout << "Running fully_connected kernel...\n";
    auto start = std::chrono::high_resolution_clock::now();

    // Launch kernel
    auto run = kernel(input_bo, weights_bo, output_bo, input_size, output_size);
    run.wait();

    auto end = std::chrono::high_resolution_clock::now();
    double exec_ms = std::chrono::duration<double, std::milli>(end - start).count();

    // Sync output buffer from device
    output_bo.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

    // Copy results into host output vector
    std::copy(output_map, output_map + output_size, output.begin());

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

    // Throughput: total data read + write
    double total_bytes = (input_size + input_size * output_size + output_size) * sizeof(float);
    double gb = total_bytes / (1024.0 * 1024.0 * 1024.0);
    double throughput = gb / (exec_ms / 1000.0);

    std::cout << "Data size: " << gb << " GB\n";
    std::cout << "Throughput: " << throughput << " GB/s\n";

    return 0;
}
