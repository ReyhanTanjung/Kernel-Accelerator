#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <xrt.h>
#include <experimental/xrt_kernel.h>
#include <chrono>
#include <iomanip>

// Constants matching your kernel header
#define MAX_POINTS 16
#define MAX_CLUSTERS 4
#define MAX_DIM 4
#define MAX_ITERATIONS 20

// Helper function to generate random test data
void generate_test_data(std::vector<float>& points, int num_points, int dimensions) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 100.0f);
    
    for (int p = 0; p < num_points; p++) {
        for (int d = 0; d < dimensions; d++) {
            points[p * MAX_DIM + d] = dis(gen);
        }
    }
}

// Initialize centroids randomly from data points
void initialize_centroids(const std::vector<float>& points, std::vector<float>& centroids, 
                         int num_points, int num_clusters, int dimensions) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, num_points - 1);
    
    for (int c = 0; c < num_clusters; c++) {
        int random_point = dis(gen);
        for (int d = 0; d < dimensions; d++) {
            centroids[c * MAX_DIM + d] = points[random_point * MAX_DIM + d];
        }
    }
}

// CPU reference implementation for verification
void kmeans_cpu_reference(const std::vector<float>& points, std::vector<float>& centroids,
                         std::vector<int>& assignments, int num_points, int num_clusters,
                         int dimensions, int max_iterations) {
    std::vector<float> new_centroids(num_clusters * MAX_DIM, 0.0f);
    std::vector<int> cluster_sizes(num_clusters, 0);
    
    for (int iter = 0; iter < max_iterations; iter++) {
        // Reset accumulators
        std::fill(cluster_sizes.begin(), cluster_sizes.end(), 0);
        std::fill(new_centroids.begin(), new_centroids.end(), 0.0f);
        
        // Assign points to clusters
        for (int p = 0; p < num_points; p++) {
            float min_dist = std::numeric_limits<float>::max();
            int closest = 0;
            
            for (int c = 0; c < num_clusters; c++) {
                float dist = 0.0f;
                for (int d = 0; d < dimensions; d++) {
                    float diff = points[p * MAX_DIM + d] - centroids[c * MAX_DIM + d];
                    dist += diff * diff;
                }
                
                if (dist < min_dist) {
                    min_dist = dist;
                    closest = c;
                }
            }
            
            assignments[p] = closest;
            cluster_sizes[closest]++;
            
            for (int d = 0; d < dimensions; d++) {
                new_centroids[closest * MAX_DIM + d] += points[p * MAX_DIM + d];
            }
        }
        
        // Update centroids
        for (int c = 0; c < num_clusters; c++) {
            if (cluster_sizes[c] > 0) {
                for (int d = 0; d < dimensions; d++) {
                    centroids[c * MAX_DIM + d] = new_centroids[c * MAX_DIM + d] / cluster_sizes[c];
                }
            }
        }
    }
}

// Print results helper function
void print_results(const std::vector<float>& points, const std::vector<float>& centroids,
                  const std::vector<int>& assignments, int num_points, int num_clusters, int dimensions) {
    std::cout << "\n=== Results ===" << std::endl;
    
    // Print centroids
    std::cout << "Final Centroids:" << std::endl;
    for (int c = 0; c < num_clusters; c++) {
        std::cout << "Cluster " << c << ": [";
        for (int d = 0; d < dimensions; d++) {
            std::cout << std::fixed << std::setprecision(2) << centroids[c * MAX_DIM + d];
            if (d < dimensions - 1) std::cout << ", ";
        }
        std::cout << "]" << std::endl;
    }
    
    // Print point assignments
    std::cout << "\nPoint Assignments:" << std::endl;
    for (int p = 0; p < num_points; p++) {
        std::cout << "Point " << p << " [";
        for (int d = 0; d < dimensions; d++) {
            std::cout << std::fixed << std::setprecision(2) << points[p * MAX_DIM + d];
            if (d < dimensions - 1) std::cout << ", ";
        }
        std::cout << "] -> Cluster " << assignments[p] << std::endl;
    }
}

// Verify FPGA results against CPU reference
bool verify_results(const std::vector<float>& fpga_centroids, const std::vector<int>& fpga_assignments,
                   const std::vector<float>& cpu_centroids, const std::vector<int>& cpu_assignments,
                   int num_clusters, int num_points, int dimensions, float tolerance = 0.01f) {
    bool centroids_match = true;
    bool assignments_match = true;
    
    // Check centroids (with tolerance for floating point differences)
    for (int c = 0; c < num_clusters; c++) {
        for (int d = 0; d < dimensions; d++) {
            float diff = std::abs(fpga_centroids[c * MAX_DIM + d] - cpu_centroids[c * MAX_DIM + d]);
            if (diff > tolerance) {
                std::cout << "Centroid mismatch at cluster " << c << ", dimension " << d 
                         << ": FPGA=" << fpga_centroids[c * MAX_DIM + d] 
                         << ", CPU=" << cpu_centroids[c * MAX_DIM + d] 
                         << ", diff=" << diff << std::endl;
                centroids_match = false;
            }
        }
    }
    
    // Check assignments
    for (int p = 0; p < num_points; p++) {
        if (fpga_assignments[p] != cpu_assignments[p]) {
            std::cout << "Assignment mismatch at point " << p 
                     << ": FPGA=" << fpga_assignments[p] 
                     << ", CPU=" << cpu_assignments[p] << std::endl;
            assignments_match = false;
        }
    }
    
    return centroids_match && assignments_match;
}

int main() {
    // Test parameters
    int num_points = 12;      // Must be <= MAX_POINTS
    int num_clusters = 3;     // Must be <= MAX_CLUSTERS  
    int dimensions = 3;       // Must be <= MAX_DIM
    int max_iterations = 10;  // Must be <= MAX_ITERATIONS
    
    std::cout << "=== K-means FPGA Implementation Test ===" << std::endl;
    std::cout << "Points: " << num_points << ", Clusters: " << num_clusters 
              << ", Dimensions: " << dimensions << ", Iterations: " << max_iterations << std::endl;
    
    // Initialize data
    std::vector<float> points(MAX_POINTS * MAX_DIM, 0.0f);
    std::vector<float> fpga_centroids(MAX_CLUSTERS * MAX_DIM, 0.0f);
    std::vector<float> cpu_centroids(MAX_CLUSTERS * MAX_DIM, 0.0f);
    std::vector<int> fpga_assignments(MAX_POINTS, 0);
    std::vector<int> cpu_assignments(MAX_POINTS, 0);
    
    // Generate test data
    generate_test_data(points, num_points, dimensions);
    initialize_centroids(points, fpga_centroids, num_points, num_clusters, dimensions);
    
    // Copy initial centroids for CPU reference
    cpu_centroids = fpga_centroids;
    
    std::cout << "\n=== Initial Data ===" << std::endl;
    std::cout << "Points:" << std::endl;
    for (int p = 0; p < num_points; p++) {
        std::cout << "Point " << p << ": [";
        for (int d = 0; d < dimensions; d++) {
            std::cout << std::fixed << std::setprecision(2) << points[p * MAX_DIM + d];
            if (d < dimensions - 1) std::cout << ", ";
        }
        std::cout << "]" << std::endl;
    }
    
    std::cout << "\nInitial Centroids:" << std::endl;
    for (int c = 0; c < num_clusters; c++) {
        std::cout << "Cluster " << c << ": [";
        for (int d = 0; d < dimensions; d++) {
            std::cout << std::fixed << std::setprecision(2) << fpga_centroids[c * MAX_DIM + d];
            if (d < dimensions - 1) std::cout << ", ";
        }
        std::cout << "]" << std::endl;
    }
    
    // Run CPU reference implementation
    std::cout << "\n=== Running CPU Reference ===" << std::endl;
    auto cpu_start = std::chrono::high_resolution_clock::now();
    kmeans_cpu_reference(points, cpu_centroids, cpu_assignments, num_points, num_clusters, dimensions, max_iterations);
    auto cpu_end = std::chrono::high_resolution_clock::now();
    double cpu_time_ms = std::chrono::duration<double, std::milli>(cpu_end - cpu_start).count();
    std::cout << "CPU execution time: " << cpu_time_ms << " ms" << std::endl;
    
    // Setup XRT device and kernel
    std::cout << "\n=== Setting up FPGA ===" << std::endl;
    try {
        auto device = xrt::device(0);
        auto uuid = device.load_xclbin("kmeans.xclbin");  // Update with your .xclbin filename
        auto kernel = xrt::kernel(device, uuid, "kmeans_kernel", xrt::kernel::cu_access_mode::exclusive);
        
        std::cout << "FPGA setup successful!" << std::endl;
        
        // Create buffer objects
        auto points_buf = xrt::bo(device, MAX_POINTS * MAX_DIM * sizeof(float), kernel.group_id(0));
        auto centroids_buf = xrt::bo(device, MAX_CLUSTERS * MAX_DIM * sizeof(float), kernel.group_id(1));
        auto assignments_buf = xrt::bo(device, MAX_POINTS * sizeof(int), kernel.group_id(2));
        
        // Map buffers to host memory
        auto points_map = points_buf.map<float*>();
        auto centroids_map = centroids_buf.map<float*>();
        auto assignments_map = assignments_buf.map<int*>();
        
        // Copy data to mapped memory
        for (int i = 0; i < MAX_POINTS * MAX_DIM; i++) {
            points_map[i] = points[i];
        }
        for (int i = 0; i < MAX_CLUSTERS * MAX_DIM; i++) {
            centroids_map[i] = fpga_centroids[i];
        }
        
        // Synchronize input data to device
        std::cout << "Transferring data to FPGA..." << std::endl;
        points_buf.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        centroids_buf.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        
        // Execute kernel
        std::cout << "Executing K-means kernel..." << std::endl;
        auto fpga_start = std::chrono::high_resolution_clock::now();
        
        auto run = kernel(points_buf, centroids_buf, assignments_buf, 
                         num_points, num_clusters, dimensions, max_iterations);
        run.wait();
        
        auto fpga_end = std::chrono::high_resolution_clock::now();
        double fpga_time_ms = std::chrono::duration<double, std::milli>(fpga_end - fpga_start).count();
        
        // Get results from device
        std::cout << "Retrieving results from FPGA..." << std::endl;
        centroids_buf.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        assignments_buf.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        
        // Copy results back
        for (int i = 0; i < MAX_CLUSTERS * MAX_DIM; i++) {
            fpga_centroids[i] = centroids_map[i];
        }
        for (int i = 0; i < MAX_POINTS; i++) {
            fpga_assignments[i] = assignments_map[i];
        }
        
        std::cout << "FPGA execution time: " << fpga_time_ms << " ms" << std::endl;
        
        // Verify results
        std::cout << "\n=== Verification ===" << std::endl;
        bool results_match = verify_results(fpga_centroids, fpga_assignments, 
                                          cpu_centroids, cpu_assignments,
                                          num_clusters, num_points, dimensions);
        
        if (results_match) {
            std::cout << "✓ VERIFICATION PASSED! FPGA and CPU results match." << std::endl;
        } else {
            std::cout << "✗ VERIFICATION FAILED! Results do not match." << std::endl;
        }
        
        // Performance comparison
        std::cout << "\n=== Performance Comparison ===" << std::endl;
        std::cout << "CPU time: " << std::fixed << std::setprecision(3) << cpu_time_ms << " ms" << std::endl;
        std::cout << "FPGA time: " << std::fixed << std::setprecision(3) << fpga_time_ms << " ms" << std::endl;
        if (cpu_time_ms > 0) {
            double speedup = cpu_time_ms / fpga_time_ms;
            std::cout << "Speedup: " << std::fixed << std::setprecision(2) << speedup << "x" << std::endl;
        }
        
        // Print final results
        std::cout << "\n=== FPGA Results ===" << std::endl;
        print_results(points, fpga_centroids, fpga_assignments, num_points, num_clusters, dimensions);
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}