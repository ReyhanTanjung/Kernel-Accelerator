#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <xrt.h>
#include <experimental/xrt_kernel.h>
#include "kmeans.h"

// Helper to generate random data with clusters
void generate_data(float* points, int num_points, int num_clusters, int dimensions) {
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Generate cluster centers
    std::vector<std::vector<float>> centers(num_clusters, std::vector<float>(dimensions));
    std::uniform_real_distribution<> center_dis(0.0, 100.0);
    
    for (int c = 0; c < num_clusters; c++) {
        for (int d = 0; d < dimensions; d++) {
            centers[c][d] = center_dis(gen);
        }
    }
    
    // Generate points around centers
    std::normal_distribution<> point_dis(0.0, 5.0);
    
    for (int p = 0; p < num_points; p++) {
        int cluster = p % num_clusters;
        
        for (int d = 0; d < dimensions; d++) {
            points[p * MAX_DIM + d] = centers[cluster][d] + point_dis(gen);
        }
        
        // Zero out unused dimensions
        for (int d = dimensions; d < MAX_DIM; d++) {
            points[p * MAX_DIM + d] = 0.0f;
        }
    }
}

// Calculate inertia (sum of squared distances)
float calculate_inertia(const float* points, const float* centroids, 
                       const int* assignments, int num_points, 
                       int num_clusters, int dimensions) {
    float inertia = 0.0f;
    
    for (int p = 0; p < num_points; p++) {
        int c = assignments[p];
        float dist_sq = 0.0f;
        
        for (int d = 0; d < dimensions; d++) {
            float diff = points[p * MAX_DIM + d] - centroids[c * MAX_DIM + d];
            dist_sq += diff * diff;
        }
        
        inertia += dist_sq;
    }
    
    return inertia;
}

int main(int argc, char* argv[]) {
    // Parameters (can be adjusted based on your needs)
    int num_points = 1024;
    int num_clusters = 8;
    int dimensions = 3;
    int max_iterations = 20;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "-p" && i+1 < argc) {
            num_points = std::stoi(argv[i+1]);
            i++;
        } else if (std::string(argv[i]) == "-k" && i+1 < argc) {
            num_clusters = std::stoi(argv[i+1]);
            i++;
        } else if (std::string(argv[i]) == "-d" && i+1 < argc) {
            dimensions = std::stoi(argv[i+1]);
            i++;
        } else if (std::string(argv[i]) == "-i" && i+1 < argc) {
            max_iterations = std::stoi(argv[i+1]);
            i++;
        }
    }
    
    // Validate parameters
    if (num_points > MAX_POINTS) {
        std::cout << "Warning: Reducing number of points to " << MAX_POINTS << std::endl;
        num_points = MAX_POINTS;
    }
    if (num_clusters > MAX_CLUSTERS) {
        std::cout << "Warning: Reducing number of clusters to " << MAX_CLUSTERS << std::endl;
        num_clusters = MAX_CLUSTERS;
    }
    if (dimensions > MAX_DIM) {
        std::cout << "Warning: Reducing number of dimensions to " << MAX_DIM << std::endl;
        dimensions = MAX_DIM;
    }
    
    std::cout << "K-means clustering with " << num_points << " points, " 
              << num_clusters << " clusters, " << dimensions << " dimensions" << std::endl;
    
    try {
        // Setup data
        float* points = new float[num_points * MAX_DIM];
        float* centroids = new float[num_clusters * MAX_DIM];
        int* assignments = new int[num_points];
        
        // Make copies for CPU verification
        float* cpu_centroids = new float[num_clusters * MAX_DIM];
        int* cpu_assignments = new int[num_points];
        
        // Generate data
        std::cout << "Generating data..." << std::endl;
        generate_data(points, num_points, num_clusters, dimensions);
        
        // Initialize centroids with the first K points
        for (int c = 0; c < num_clusters; c++) {
            for (int d = 0; d < dimensions; d++) {
                centroids[c * MAX_DIM + d] = points[c * MAX_DIM + d];
            }
        }
        
        // Copy centroids for CPU version
        std::memcpy(cpu_centroids, centroids, num_clusters * MAX_DIM * sizeof(float));
        
        // Setup XRT
        std::cout << "Setting up XRT..." << std::endl;
        auto device = xrt::device(0);
        auto uuid = device.load_xclbin("kmeans_kernel.xclbin");
        auto kernel = xrt::kernel(device, uuid, "kmeans_kernel");
        
        // Allocate device buffers
        std::cout << "Allocating device buffers..." << std::endl;
        auto points_buf = xrt::bo(device, points, num_points * MAX_DIM * sizeof(float), kernel.group_id(0));
        auto centroids_buf = xrt::bo(device, centroids, num_clusters * MAX_DIM * sizeof(float), kernel.group_id(1));
        auto assignments_buf = xrt::bo(device, assignments, num_points * sizeof(int), kernel.group_id(2));
        
        // Copy data to device
        std::cout << "Copying data to device..." << std::endl;
        points_buf#include <iostream>#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>
#include <limits>
#include <chrono>
#include <xrt.h>
#include <experimental/xrt_kernel.h>
#include "kmeans.h"

// Function to generate random centroids
void initialize_centroids(std::vector<float>& centroids, const std::vector<float>& points,
                         int num_points, int num_clusters, int dimensions) {
    // Use k-means++ initialization for better starting positions
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, num_points - 1);
    
    // Choose first centroid randomly
    int idx = dis(gen);
    for (int d = 0; d < dimensions; d++) {
        centroids[0 * MAX_DIM + d] = points[idx * MAX_DIM + d];
    }
    
    // Choose remaining centroids with probability proportional to distance
    std::vector<float> distances(num_points);
    
    for (int c = 1; c < num_clusters; c++) {
        // Calculate distances to closest existing centroid
        for (int p = 0; p < num_points; p++) {
            float min_dist = std::numeric_limits<float>::max();
            
            for (int prev_c = 0; prev_c < c; prev_c++) {
                float dist = 0.0f;
                for (int d = 0; d < dimensions; d++) {
                    float diff = points[p * MAX_DIM + d] - centroids[prev_c * MAX_DIM + d];
                    dist += diff * diff;
                }
                min_dist = std::min(min_dist, dist);
            }
            distances[p] = min_dist;
        }
        
        // Calculate sum of distances for probability
        float sum_distances = 0.0f;
        for (int p = 0; p < num_points; p++) {
            sum_distances += distances[p];
        }
        
        // Choose next centroid with probability proportional to squared distance
        float threshold = static_cast<float>(gen()) / static_cast<float>(gen.max()) * sum_distances;
        float cumsum = 0.0f;
        idx = 0;
        
        for (int p = 0; p < num_points; p++) {
            cumsum += distances[p];
            if (cumsum >= threshold) {
                idx = p;
                break;
            }
        }
        
        for (int d = 0; d < dimensions; d++) {
            centroids[c * MAX_DIM + d] = points[idx * MAX_DIM + d];
        }
    }
}

// Function to compute reference K-means clustering on CPU
void kmeans_reference(const std::vector<float>& points, std::vector<float>& centroids,
                     std::vector<int>& assignments, int num_points, int num_clusters, 
                     int dimensions, int max_iterations) {
    std::vector<int> cluster_sizes(num_clusters);
    std::vector<float> new_centroids(num_clusters * MAX_DIM);
    
    // Perform K-means iterations
    for (int iter = 0; iter < max_iterations; iter++) {
        // Reset cluster sizes and new centroids
        std::fill(cluster_sizes.begin(), cluster_sizes.end(), 0);
        std::fill(new_centroids.begin(), new_centroids.end(), 0.0f);
        
        // Assign points to closest centroid
        for (int p = 0; p < num_points; p++) {
            float min_dist = std::numeric_limits<float>::max();
            int closest = 0;
            
            // Find closest centroid
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
            
            // Update assignment
            assignments[p] = closest;
            cluster_sizes[closest]++;
            
            // Update sums for centroid calculation
            for (int d = 0; d < dimensions; d++) {
                new_centroids[closest * MAX_DIM + d] += points[p * MAX_DIM + d];
            }
        }
        
        // Calculate new centroids
        for (int c = 0; c < num_clusters; c++) {
            if (cluster_sizes[c] > 0) {
                for (int d = 0; d < dimensions; d++) {
                    centroids[c * MAX_DIM + d] = new_centroids[c * MAX_DIM + d] / cluster_sizes[c];
                }
            }
        }
    }
}

// Function to calculate inertia (sum of squared distances to centroids)
float calculate_inertia(const std::vector<float>& points, const std::vector<float>& centroids,
                       const std::vector<int>& assignments, int num_points, int num_clusters, 
                       int dimensions) {
    float inertia = 0.0f;
    
    for (int p = 0; p < num_points; p++) {
        int c = assignments[p];
        float dist = 0.0f;
        
        for (int d = 0; d < dimensions; d++) {
            float diff = points[p * MAX_DIM + d] - centroids[c * MAX_DIM + d];
            dist += diff * diff;
        }
        
        inertia += dist;
    }
    
    return inertia;
}

int main(int argc, char* argv[]) {
    // Default parameters
    int num_points = 1000;
    int num_clusters = 5;
    int dimensions = 3;
    int max_iterations = 100;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (i + 1 < argc) {
            if (std::string(argv[i]) == "-p" || std::string(argv[i]) == "--points") {
                num_points = std::stoi(argv[i + 1]);
                i++;
            } else if (std::string(argv[i]) == "-k" || std::string(argv[i]) == "--clusters") {
                num_clusters = std::stoi(argv[i + 1]);
                i++;
            } else if (std::string(argv[i]) == "-d" || std::string(argv[i]) == "--dimensions") {
                dimensions = std::stoi(argv[i + 1]);
                i++;
            } else if (std::string(argv[i]) == "-i" || std::string(argv[i]) == "--iterations") {
                max_iterations = std::stoi(argv[i + 1]);
                i++;
            }
        }
    }
    
    // Check parameters
    if (num_points > MAX_POINTS) {
        std::cerr << "Error: Number of points cannot exceed " << MAX_POINTS << std::endl;
        return 1;
    }
    
    if (num_clusters > MAX_CLUSTERS) {
        std::cerr << "Error: Number of clusters cannot exceed " << MAX_CLUSTERS << std::endl;
        return 1;
    }
    
    if (dimensions > MAX_DIM) {
        std::cerr << "Error: Number of dimensions cannot exceed " << MAX_DIM << std::endl;
        return 1;
    }
    
    std::cout << "K-means Clustering with " << num_points << " points, " 
              << num_clusters << " clusters, " << dimensions << " dimensions, "
              << max_iterations << " iterations" << std::endl;
    
    // Generate random data points
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 100.0);
    
    std::vector<float> points(num_points * MAX_DIM);
    for (int i = 0; i < num_points; i++) {
        for (int d = 0; d < dimensions; d++) {
            points[i * MAX_DIM + d] = dis(gen);
        }
        // Zero out unused dimensions
        for (int d = dimensions; d < MAX_DIM; d++) {
            points[i * MAX_DIM + d] = 0.0f;
        }
    }
    
    // Initialize centroids
    std::vector<float> centroids(num_clusters * MAX_DIM);
    initialize_centroids(centroids, points, num_points, num_clusters, dimensions);
    
    // Create arrays for cluster assignments
    std::vector<int> assignments(num_points, 0);
    std::vector<int> ref_assignments(num_points, 0);
    std::vector<float> ref_centroids = centroids; // Copy centroids for reference calculation
    
    // Run reference K-means clustering on CPU
    std::cout << "Running reference K-means on CPU..." << std::endl;
    auto cpu_start = std::chrono::high_resolution_clock::now();
    kmeans_reference(points, ref_centroids, ref_assignments, num_points, num_clusters, 
                    dimensions, max_iterations);
    auto cpu_end = std::chrono::high_resolution_clock::now();
    double cpu_time_ms = std::chrono::duration<double, std::milli>(cpu_end - cpu_start).count();
    
    // Calculate reference inertia
    float ref_inertia = calculate_inertia(points, ref_centroids, ref_assignments, 
                                        num_points, num_clusters, dimensions);
    
    std::cout << "CPU execution time: " << cpu_time_ms << " ms" << std::endl;
    std::cout << "CPU inertia: " << ref_inertia << std::endl;
    
    // Set up XRT device and kernel
    try {
        std::cout << "Initializing XRT..." << std::endl;
        auto device = xrt::device(0);
        auto uuid = device.load_xclbin("kmeans_kernel.xclbin");
        auto kernel = xrt::kernel(device, uuid, "kmeans_kernel", xrt::kernel::cu_access_mode::exclusive);
        
        std::cout << "Allocating device buffers..." << std::endl;
        
        // Create buffer objects
        auto points_buf = xrt::bo(device, points.size() * sizeof(float), kernel.group_id(0));
        auto centroids_buf = xrt::bo(device, centroids.size() * sizeof(float), kernel.group_id(1));
        auto assignments_buf = xrt::bo(device, assignments.size() * sizeof(int), kernel.group_id(2));
        
        // Map buffers to host memory
        auto points_map = points_buf.map<float*>();
        auto centroids_map = centroids_buf.map<float*>();
        auto assignments_map = assignments_buf.map<int*>();
        
        // Copy data to mapped memory
        for (size_t i = 0; i < points.size(); i++) {
            points_map[i] = points[i];
        }
        
        for (size_t i = 0; i < centroids.size(); i++) {
            centroids_map[i] = centroids[i];
        }
        
        // Sync input buffers to device
        std::cout << "Syncing buffers to device..." << std::endl;
        points_buf.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        centroids_buf.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        
        // Execute kernel
        std::cout << "Executing FPGA kernel..." << std::endl;
        auto fpga_start = std::chrono::high_resolution_clock::now();
        
        auto run = kernel(points_buf, centroids_buf, assignments_buf, num_points, 
                       num_clusters, dimensions, max_iterations);
        run.wait();
        
        auto fpga_end = std::chrono::high_resolution_clock::now();
        double fpga_time_ms = std::chrono::duration<double, std::milli>(fpga_end - fpga_start).count();
        
        // Get results from device
        std::cout << "Getting results from device..." << std::endl;
        centroids_buf.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        assignments_buf.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        
        // Copy results back
        for (int i = 0; i < num_points; i++) {
            assignments[i] = assignments_map[i];
        }
        
        for (size_t i = 0; i < centroids.size(); i++) {
            centroids[i] = centroids_map[i];
        }
        
        // Calculate FPGA inertia
        float fpga_inertia = calculate_inertia(points, centroids, assignments, 
                                            num_points, num_clusters, dimensions);
        
        std::cout << "FPGA execution time: " << fpga_time_ms << " ms" << std::endl;
        std::cout << "FPGA inertia: " << fpga_inertia << std::endl;
        
        // Verify results
        float inertia_diff = std::abs(fpga_inertia - ref_inertia) / ref_inertia;
        std::cout << "Inertia difference: " << inertia_diff * 100.0f << "%" << std::endl;
        
        // Check if results are close enough (note: k-means may converge to different local minima)
        if (inertia_diff <= 0.05) { // 5% tolerance
            std::cout << "Verification PASSED: FPGA and CPU results are comparable." << std::endl;
        } else {
            std::cout << "Verification WARNING: FPGA and CPU results differ significantly." << std::endl;
            std::cout << "This could be due to different local minima or implementation differences." << std::endl;
        }
        
        // Calculate acceleration factor
        double speedup = cpu_time_ms / fpga_time_ms;
        std::cout << "Acceleration factor: " << speedup << "x" << std::endl;
        
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    
    return 0;
}