#include "kmeans.h"
#include <hls_math.h>
#include <float.h>

// Simplified K-means kernel with static arrays
extern "C" {
void kmeans_kernel(
    const float points[MAX_POINTS*MAX_DIM],  // Input data points
    float centroids[MAX_CLUSTERS*MAX_DIM],   // Centroids (in/out)
    int assignments[MAX_POINTS],             // Cluster assignments
    int num_points,                          // Number of data points
    int num_clusters,                        // Number of clusters (k)
    int dimensions,                          // Number of dimensions
    int max_iterations                       // Maximum iterations
) {
    // Interface pragmas for AXI
#pragma HLS INTERFACE m_axi port=points bundle=gmem0 depth=MAX_POINTS*MAX_DIM
#pragma HLS INTERFACE m_axi port=centroids bundle=gmem1 depth=MAX_CLUSTERS*MAX_DIM
#pragma HLS INTERFACE m_axi port=assignments bundle=gmem2 depth=MAX_POINTS
#pragma HLS INTERFACE s_axilite port=num_points bundle=control
#pragma HLS INTERFACE s_axilite port=num_clusters bundle=control
#pragma HLS INTERFACE s_axilite port=dimensions bundle=control
#pragma HLS INTERFACE s_axilite port=max_iterations bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

    // Use local static arrays for FPGA BRAM
    float local_centroids[MAX_CLUSTERS][MAX_DIM];
    float new_centroids[MAX_CLUSTERS][MAX_DIM];
    int cluster_sizes[MAX_CLUSTERS];
    
    // Copy centroids to local memory
    for (int c = 0; c < num_clusters; c++) {
        for (int d = 0; d < dimensions; d++) {
            local_centroids[c][d] = centroids[c * MAX_DIM + d];
        }
    }
    
    // Main iteration loop
    for (int iter = 0; iter < max_iterations; iter++) {
        // Reset accumulators
        for (int c = 0; c < num_clusters; c++) {
            cluster_sizes[c] = 0;
            for (int d = 0; d < dimensions; d++) {
                new_centroids[c][d] = 0.0f;
            }
        }
        
        // Process each point
        for (int p = 0; p < num_points; p++) {
            // Find closest centroid
            float min_dist = FLT_MAX;
            int closest = 0;
            
            for (int c = 0; c < num_clusters; c++) {
                float dist = 0.0f;
                
                // Calculate Euclidean distance squared
                for (int d = 0; d < dimensions; d++) {
                    float diff = points[p * MAX_DIM + d] - local_centroids[c][d];
                    dist += diff * diff;
                }
                
                // Update closest centroid if needed
                if (dist < min_dist) {
                    min_dist = dist;
                    closest = c;
                }
            }
            
            // Store assignment
            assignments[p] = closest;
            
            // Update sum and count
            cluster_sizes[closest]++;
            for (int d = 0; d < dimensions; d++) {
                new_centroids[closest][d] += points[p * MAX_DIM + d];
            }
        }
        
        // Calculate new centroids
        for (int c = 0; c < num_clusters; c++) {
            if (cluster_sizes[c] > 0) {
                for (int d = 0; d < dimensions; d++) {
                    local_centroids[c][d] = new_centroids[c][d] / cluster_sizes[c];
                }
            }
        }
    }
    
    // Copy final centroids back to global memory
    for (int c = 0; c < num_clusters; c++) {
        for (int d = 0; d < dimensions; d++) {
            centroids[c * MAX_DIM + d] = local_centroids[c][d];
        }
    }
}
}