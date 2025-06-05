/**
* Copyright (C) 2025
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/

#include "pca_eigen.h"
#include <math.h>

// TRIPCOUNT identifiers for optimization
const unsigned int c_dim = MAX_DIM;
const unsigned int c_iter = MAX_ITER;

// Helper function to compute matrix multiplication: C = A * B
void matrix_multiply(float A[MAX_DIM][MAX_DIM], float B[MAX_DIM][MAX_DIM], float C[MAX_DIM][MAX_DIM], int n) {
#pragma HLS INLINE
    // Initialize C to zeros
    for (int i = 0; i < n; i++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
        for (int j = 0; j < n; j++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
#pragma HLS PIPELINE II=1
            C[i][j] = 0.0f;
        }
    }

    // Perform matrix multiplication
    for (int i = 0; i < n; i++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
        for (int j = 0; j < n; j++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
#pragma HLS PIPELINE II=1
            float sum = 0.0f;
            for (int k = 0; k < n; k++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
                sum += A[i][k] * B[k][j];
            }
            C[i][j] = sum;
        }
    }
}

// Helper function to transpose a matrix: B = A^T
void matrix_transpose(float A[MAX_DIM][MAX_DIM], float B[MAX_DIM][MAX_DIM], int n) {
#pragma HLS INLINE
    for (int i = 0; i < n; i++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
        for (int j = 0; j < n; j++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
#pragma HLS PIPELINE II=1
            B[j][i] = A[i][j];
        }
    }
}

// Helper function for matrix-vector multiplication: y = A * x
void matrix_vector_multiply(float A[MAX_DIM][MAX_DIM], float x[MAX_DIM], float y[MAX_DIM], int n) {
#pragma HLS INLINE
    for (int i = 0; i < n; i++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
#pragma HLS PIPELINE II=1
        float sum = 0.0f;
        for (int j = 0; j < n; j++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
            sum += A[i][j] * x[j];
        }
        y[i] = sum;
    }
}

// Helper function to normalize a vector to unit length
void normalize_vector(float v[MAX_DIM], int n) {
#pragma HLS INLINE
    // Calculate the vector norm
    float norm = 0.0f;
    for (int i = 0; i < n; i++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
#pragma HLS PIPELINE II=1
        norm += v[i] * v[i];
    }
    norm = sqrt(norm);

    // Normalize the vector
    for (int i = 0; i < n; i++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
#pragma HLS PIPELINE II=1
        v[i] /= norm;
    }
}

// Helper function to compute dot product of two vectors
float vector_dot_product(float v1[MAX_DIM], float v2[MAX_DIM], int n) {
#pragma HLS INLINE
    float result = 0.0f;
    for (int i = 0; i < n; i++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
#pragma HLS PIPELINE II=1
        result += v1[i] * v2[i];
    }
    return result;
}

// Power iteration method to find the largest eigenvalue and its eigenvector
void power_iteration(float A[MAX_DIM][MAX_DIM], float eigenvalue[1], float eigenvector[MAX_DIM], int n) {
#pragma HLS INLINE
    float v[MAX_DIM];           // Current vector
    float Av[MAX_DIM];          // Result of A*v
    float prev_lambda = 0.0f;   // Previous eigenvalue
    float lambda = 0.0f;        // Current eigenvalue
    float diff = 1.0f;          // Convergence measure
    
    // Initialize v with random-like values to avoid getting stuck in bad directions
    for (int i = 0; i < n; i++) {
#pragma HLS PIPELINE II=1
        // Use a simple pattern that avoids zero vectors and creates variation
        v[i] = (i % 3 == 0) ? 1.0f : ((i % 3 == 1) ? -0.5f : 0.8f);
    }
    
    // Initial normalization
    normalize_vector(v, n);
    
    // Power iteration with more iterations for better convergence
    int iter = 0;
    while (diff > 0.00001f && iter < MAX_ITER) {
#pragma HLS LOOP_TRIPCOUNT min = c_iter max = c_iter
        // Compute A*v
        matrix_vector_multiply(A, v, Av, n);
        
        // Normalize result to prevent overflow/underflow
        normalize_vector(Av, n);
        
        // Update v for next iteration
        for (int i = 0; i < n; i++) {
#pragma HLS PIPELINE II=1
            v[i] = Av[i];
        }
        
        // Compute eigenvalue using Rayleigh quotient
        matrix_vector_multiply(A, v, Av, n);
        lambda = vector_dot_product(v, Av, n);
        
        // Check convergence
        diff = fabs(lambda - prev_lambda);
        prev_lambda = lambda;
        
        iter++;
    }
    
    // Ensure final normalization
    normalize_vector(v, n);
    
    // Store results
    eigenvalue[0] = lambda;
    for (int i = 0; i < n; i++) {
#pragma HLS PIPELINE II=1
        eigenvector[i] = v[i];
    }
}

// Deflation method to find multiple eigenvalues and eigenvectors
void deflation(float A[MAX_DIM][MAX_DIM], float eigenvalues[MAX_DIM], float eigenvectors[MAX_DIM][MAX_DIM], int n) {
#pragma HLS INLINE
    float current_A[MAX_DIM][MAX_DIM];
    float v[MAX_DIM];
    float vvT[MAX_DIM][MAX_DIM];  // outer product v*v^T
    float scaled_vvT[MAX_DIM][MAX_DIM];
    
    // Copy A to current_A
    for (int i = 0; i < n; i++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
        for (int j = 0; j < n; j++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
#pragma HLS PIPELINE II=1
            current_A[i][j] = A[i][j];
        }
    }
    
    // Find eigenvalues and eigenvectors one by one
    for (int k = 0; k < n; k++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
        // Find largest eigenvalue and its eigenvector
        power_iteration(current_A, &eigenvalues[k], v, n);
        
        // Store eigenvector
        for (int i = 0; i < n; i++) {
#pragma HLS PIPELINE II=1
            eigenvectors[i][k] = v[i];
        }
        
        // Compute v*v^T
        for (int i = 0; i < n; i++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
            for (int j = 0; j < n; j++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
#pragma HLS PIPELINE II=1
                vvT[i][j] = v[i] * v[j];
            }
        }
        
        // Scale vvT by eigenvalue
        for (int i = 0; i < n; i++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
            for (int j = 0; j < n; j++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
#pragma HLS PIPELINE II=1
                scaled_vvT[i][j] = eigenvalues[k] * vvT[i][j];
            }
        }
        
        // Deflate the matrix: A = A - lambda * v*v^T
        for (int i = 0; i < n; i++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
            for (int j = 0; j < n; j++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
#pragma HLS PIPELINE II=1
                current_A[i][j] -= scaled_vvT[i][j];
            }
        }
        
        // Extra step: Apply Gram-Schmidt orthogonalization on eigenvectors
        // This ensures better orthogonality among eigenvectors
        if (k > 0) {
            for (int p = 0; p < k; p++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
                // Calculate the dot product
                float dp = 0.0f;
                for (int i = 0; i < n; i++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
#pragma HLS PIPELINE II=1
                    dp += eigenvectors[i][k] * eigenvectors[i][p];
                }
                
                // Subtract the projection
                for (int i = 0; i < n; i++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
#pragma HLS PIPELINE II=1
                    eigenvectors[i][k] -= dp * eigenvectors[i][p];
                }
            }
            
            // Re-normalize the eigenvector
            float norm = 0.0f;
            for (int i = 0; i < n; i++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
#pragma HLS PIPELINE II=1
                norm += eigenvectors[i][k] * eigenvectors[i][k];
            }
            norm = sqrt(norm);
            
            for (int i = 0; i < n; i++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
#pragma HLS PIPELINE II=1
                eigenvectors[i][k] /= norm;
            }
        }
    }
}

// Main PCA kernel function
void pca_eigen_kernel(
    float* data,           // Input: data matrix
    float* mean,           // Output: mean vector
    float* covariance,     // Output: covariance matrix
    float* eigenvalues,    // Output: eigenvalues
    float* eigenvectors,   // Output: eigenvectors matrix
    int rows,              // Number of samples/rows
    int cols               // Number of features/columns
) {
    // Interface pragmas with explicit depth information
#pragma HLS INTERFACE m_axi port=data offset=slave bundle=gmem0 depth=MAX_DATA_SIZE
#pragma HLS INTERFACE m_axi port=mean offset=slave bundle=gmem1 depth=MAX_VECTOR_SIZE
#pragma HLS INTERFACE m_axi port=covariance offset=slave bundle=gmem1 depth=MAX_MATRIX_SIZE
#pragma HLS INTERFACE m_axi port=eigenvalues offset=slave bundle=gmem2 depth=MAX_VECTOR_SIZE
#pragma HLS INTERFACE m_axi port=eigenvectors offset=slave bundle=gmem2 depth=MAX_MATRIX_SIZE
    
    // All s_axilite interfaces must use the same bundle name
#pragma HLS INTERFACE s_axilite port=rows bundle=control
#pragma HLS INTERFACE s_axilite port=cols bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control
#pragma HLS INTERFACE s_axilite port=data bundle=control
#pragma HLS INTERFACE s_axilite port=mean bundle=control
#pragma HLS INTERFACE s_axilite port=covariance bundle=control
#pragma HLS INTERFACE s_axilite port=eigenvalues bundle=control
#pragma HLS INTERFACE s_axilite port=eigenvectors bundle=control

    // Local arrays
    float local_data[MAX_DIM][MAX_DIM];
    float local_mean[MAX_DIM];
    float local_covariance[MAX_DIM][MAX_DIM];
    float local_eigenvalues[MAX_DIM];
    float local_eigenvectors[MAX_DIM][MAX_DIM];
    float centered_data[MAX_DIM][MAX_DIM];
    float transposed_data[MAX_DIM][MAX_DIM];

    // Partitioning for better performance
#pragma HLS ARRAY_PARTITION variable=local_data complete dim=2
#pragma HLS ARRAY_PARTITION variable=local_mean complete dim=1
#pragma HLS ARRAY_PARTITION variable=local_covariance complete dim=2
#pragma HLS ARRAY_PARTITION variable=local_eigenvalues complete dim=1
#pragma HLS ARRAY_PARTITION variable=local_eigenvectors complete dim=2
#pragma HLS ARRAY_PARTITION variable=centered_data complete dim=2
#pragma HLS ARRAY_PARTITION variable=transposed_data complete dim=2

    // Ensure dimensions are within bounds
    int n_rows = (rows > MAX_DIM) ? MAX_DIM : rows;
    int n_cols = (cols > MAX_DIM) ? MAX_DIM : cols;

    // Read input data to local memory
read_data:
    for (int i = 0; i < n_rows; i++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
        for (int j = 0; j < n_cols; j++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
#pragma HLS PIPELINE II=1
            local_data[i][j] = data[i * cols + j];
        }
    }

    // Step 1: Compute mean vector
compute_mean:
    for (int j = 0; j < n_cols; j++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
#pragma HLS PIPELINE II=1
        float sum = 0.0f;
        for (int i = 0; i < n_rows; i++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
            sum += local_data[i][j];
        }
        local_mean[j] = sum / n_rows;
    }

    // Step 2: Center the data (subtract mean)
center_data:
    for (int i = 0; i < n_rows; i++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
        for (int j = 0; j < n_cols; j++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
#pragma HLS PIPELINE II=1
            centered_data[i][j] = local_data[i][j] - local_mean[j];
        }
    }

    // Step 3: Transpose centered data
    matrix_transpose(centered_data, transposed_data, n_cols);

    // Step 4: Compute covariance matrix: (1/n) * X^T * X
compute_covariance:
    for (int i = 0; i < n_cols; i++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
        for (int j = 0; j < n_cols; j++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
#pragma HLS PIPELINE II=1
            float sum = 0.0f;
            for (int k = 0; k < n_rows; k++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
                sum += centered_data[k][i] * centered_data[k][j];
            }
            local_covariance[i][j] = sum / (n_rows - 1);
        }
    }

    // Step 5: Compute eigenvalues and eigenvectors using deflation method
    deflation(local_covariance, local_eigenvalues, local_eigenvectors, n_cols);

    // Write results back to global memory
    // Write mean vector
write_mean:
    for (int j = 0; j < n_cols; j++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
#pragma HLS PIPELINE II=1
        mean[j] = local_mean[j];
    }

    // Write covariance matrix
write_covariance:
    for (int i = 0; i < n_cols; i++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
        for (int j = 0; j < n_cols; j++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
#pragma HLS PIPELINE II=1
            covariance[i * n_cols + j] = local_covariance[i][j];
        }
    }

    // Write eigenvalues
write_eigenvalues:
    for (int i = 0; i < n_cols; i++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
#pragma HLS PIPELINE II=1
        eigenvalues[i] = local_eigenvalues[i];
    }

    // Write eigenvectors (column-wise)
write_eigenvectors:
    for (int i = 0; i < n_cols; i++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
        for (int j = 0; j < n_cols; j++) {
#pragma HLS LOOP_TRIPCOUNT min = c_dim max = c_dim
#pragma HLS PIPELINE II=1
            eigenvectors[i * n_cols + j] = local_eigenvectors[i][j];
        }
    }
}