#include <iostream>
#include <vector>
#include <chrono>

#define SIZE 1024 * 1024  // 1 million elements
#define ITERATIONS 1000   // Same as in original code

int main() {
    // Initialize data
    std::vector<int> a(SIZE), b(SIZE), c(SIZE);
    
    // Create test data - same as in FPGA version
    for (int i = 0; i < SIZE; i++) {
        a[i] = i;
        b[i] = i * 2;
    }
    
    // Warmup (to ensure fair CPU timing)
    for (int i = 0; i < SIZE; i++) {
        c[i] = a[i] + b[i];
    }
    
    // Start timing
    auto start = std::chrono::high_resolution_clock::now();
    
    // Run the same number of iterations as in the FPGA test
    for (int iter = 0; iter < ITERATIONS; iter++) {
        for (int i = 0; i < SIZE; i++) {
            c[i] = a[i] + b[i];
        }
    }
    
    // End timing
    auto end = std::chrono::high_resolution_clock::now();
    double duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    // Verify results
    bool error = false;
    for (int i = 0; i < 10; i++) {
        if (c[i] != a[i] + b[i]) {
            std::cout << "Error at " << i << ": " << c[i] << " != " << a[i] + b[i] << std::endl;
            error = true;
            break;
        }
    }
    
    if (!error) {
        std::cout << "CPU-only test passed after " << ITERATIONS << " iterations\n";
        std::cout << "Total compute time: " << duration_ms << " ms\n";
        
        // Calculate throughput
        double data_size_gb = (SIZE * sizeof(int) * 3 * ITERATIONS) / (1024.0 * 1024.0 * 1024.0); // in GB
        double throughput_gbps = data_size_gb / (duration_ms / 1000.0);
        
        std::cout << "Data processed: " << data_size_gb << " GB\n";
        std::cout << "Throughput: " << throughput_gbps << " GB/s\n";
    }
    
    return 0;
}
