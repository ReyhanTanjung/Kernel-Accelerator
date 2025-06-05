// host.cpp
#include <iostream>
#include <vector>
#include <chrono>
#include <xrt.h>
#include <experimental/xrt_kernel.h>

typedef float data_t;

#define MAX_FEATURES 32
#define MAX_SUPPORT_VECTORS 128

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <xclbin>" << std::endl;
        return 1;
    }

    // Parameters for this test
    const int n_features = 16;
    const int n_sv = 64;
    const data_t gamma = 0.1f;
    const data_t bias = -0.5f;

    try {
        // Initialize XRT and load XCLBIN
        std::cout << "Initializing XRT and loading XCLBIN..." << std::endl;
        auto device = xrt::device(0);
        auto uuid = device.load_xclbin(argv[1]);
        auto kernel = xrt::kernel(device, uuid, "svm_rbf_kernel", xrt::kernel::cu_access_mode::exclusive);

        // Prepare input data
        std::vector<data_t> x_test(n_features);
        std::vector<data_t> support_vectors(n_sv * n_features);
        std::vector<data_t> alphas(n_sv);
        data_t decision_value = 0.0f;

        // Initialize with some data
        for (int i = 0; i < n_features; i++) {
            x_test[i] = (data_t)(i % 10) * 0.1f;
        }

        for (int i = 0; i < n_sv; i++) {
            for (int j = 0; j < n_features; j++) {
                support_vectors[i * n_features + j] = (data_t)((i + j) % 10) * 0.1f;
            }
            alphas[i] = (i % 2 == 0) ? 1.0f : -1.0f;  // Alternate signs
        }

        // Create device buffers
        std::cout << "Creating device buffers..." << std::endl;
        auto x_test_buf = xrt::bo(device, x_test.size() * sizeof(data_t), kernel.group_id(0));
        auto support_vectors_buf = xrt::bo(device, support_vectors.size() * sizeof(data_t), kernel.group_id(1));
        auto alphas_buf = xrt::bo(device, alphas.size() * sizeof(data_t), kernel.group_id(2));
        auto result_buf = xrt::bo(device, sizeof(data_t), kernel.group_id(3));

        // Map the buffers for host access
        auto x_test_map = x_test_buf.map<data_t*>();
        auto support_vectors_map = support_vectors_buf.map<data_t*>();
        auto alphas_map = alphas_buf.map<data_t*>();
        auto result_map = result_buf.map<data_t*>();

        // Copy data to mapped memory
        for (int i = 0; i < n_features; i++) {
            x_test_map[i] = x_test[i];
        }

        for (int i = 0; i < n_sv * n_features; i++) {
            support_vectors_map[i] = support_vectors[i];
        }

        for (int i = 0; i < n_sv; i++) {
            alphas_map[i] = alphas[i];
        }

        // Synchronize buffers to device memory
        std::cout << "Syncing input buffers to device memory..." << std::endl;
        x_test_buf.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        support_vectors_buf.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        alphas_buf.sync(XCL_BO_SYNC_BO_TO_DEVICE);

        // Launch the kernel
        std::cout << "Starting kernel execution..." << std::endl;
        auto start = std::chrono::high_resolution_clock::now();
        
        auto run = kernel(x_test_buf, support_vectors_buf, alphas_buf, gamma, bias, n_features, n_sv, result_buf);
        run.wait();
        
        auto end = std::chrono::high_resolution_clock::now();
        double duration_ms = std::chrono::duration<double, std::milli>(end - start).count();

        // Get the result
        std::cout << "Getting results from device..." << std::endl;
        result_buf.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        decision_value = result_map[0];

        // Print results
        std::cout << "SVM Decision Value: " << decision_value << std::endl;
        std::cout << "Kernel execution time: " << duration_ms << " ms" << std::endl;

        // Calculate performance metrics
        double operations = n_sv * n_features * 2.0;  // approx ops per SVM evaluation
        double operations_per_second = operations / (duration_ms / 1000.0);
        std::cout << "Approximate GFLOPS: " << operations_per_second / 1e9 << std::endl;

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }
}