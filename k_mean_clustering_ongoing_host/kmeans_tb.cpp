#include <iostream>
#include <cstdlib>
#include <cstring>
#include "kmeans.h"

// Very simple, safe testbench for K-means
int main() {
    std::cout << "=== K-means Clustering Testbench (Minimal) ===" << std::endl;
    
    // Define very small test parameters
    const int num_points = 4;
    const int num_clusters = 2;
    const int dimensions = 2;
    const int max_iterations = 10;
    
    // Allocate data with static arrays
    float points[MAX_POINTS*MAX_DIM] = {0};
    float centroids[MAX_CLUSTERS*MAX_DIM] = {0};
    int assignments[MAX_POINTS] = {0};
    
    // Initialize simple test data
    // Point 0: (1.0, 1.0)
    points[0*MAX_DIM + 0] = 1.0f;
    points[0*MAX_DIM + 1] = 1.0f;
    
    // Point 1: (1.5, 2.0)
    points[1*MAX_DIM + 0] = 1.5f;
    points[1*MAX_DIM + 1] = 2.0f;
    
    // Point 2: (5.0, 7.0)
    points[2*MAX_DIM + 0] = 5.0f;
    points[2*MAX_DIM + 1] = 7.0f;
    
    // Point 3: (6.0, 8.0)
    points[3*MAX_DIM + 0] = 6.0f;
    points[3*MAX_DIM + 1] = 8.0f;
    
    // Initial centroids
    centroids[0*MAX_DIM + 0] = 1.0f;
    centroids[0*MAX_DIM + 1] = 1.0f;
    centroids[1*MAX_DIM + 0] = 5.0f; 
    centroids[1*MAX_DIM + 1] = 7.0f;
    
    std::cout << "Starting kernel execution..." << std::endl;
    
    // Run K-means kernel
    kmeans_kernel(points, centroids, assignments, 
                num_points, num_clusters, dimensions, max_iterations);
    
    std::cout << "Kernel execution completed" << std::endl;
    
    // Print results
    std::cout << "Final cluster assignments:" << std::endl;
    for (int i = 0; i < num_points; i++) {
        std::cout << "Point " << i << " -> Cluster " << assignments[i] << std::endl;
    }
    
    std::cout << "Final centroids:" << std::endl;
    for (int c = 0; c < num_clusters; c++) {
        std::cout << "Centroid " << c << ": (";
        for (int d = 0; d < dimensions; d++) {
            std::cout << centroids[c * MAX_DIM + d];
            if (d < dimensions - 1) {
                std::cout << ", ";
            }
        }
        std::cout << ")" << std::endl;
    }
    
    std::cout << "Test finished successfully" << std::endl;
    return 0;
}