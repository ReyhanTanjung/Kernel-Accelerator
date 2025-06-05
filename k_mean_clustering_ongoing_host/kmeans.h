#ifndef _KMEANS_H_
#define _KMEANS_H_

// Define constants for K-means with smaller values for testing
#define MAX_POINTS 16      // Maximum number of data points
#define MAX_CLUSTERS 4     // Maximum number of clusters
#define MAX_DIM 4          // Maximum number of dimensions
#define MAX_ITERATIONS 20  // Maximum number of iterations

// Function declaration with extern "C" for C-style linkage
extern "C" {
void kmeans_kernel(
    const float points[MAX_POINTS*MAX_DIM],  // Input data points [num_points][dimensions]
    float centroids[MAX_CLUSTERS*MAX_DIM],   // Centroids [k][dimensions] (in/out)
    int assignments[MAX_POINTS],             // Cluster assignments for each point [num_points]
    int num_points,                          // Number of data points
    int num_clusters,                        // Number of clusters (k)
    int dimensions,                          // Number of dimensions
    int max_iterations                       // Maximum iterations
);
}

#endif // _KMEANS_H_