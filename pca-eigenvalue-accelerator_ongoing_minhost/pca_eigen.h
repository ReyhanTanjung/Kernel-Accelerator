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

#ifndef PCA_EIGEN_H
#define PCA_EIGEN_H

// Maximum dimensions for the input matrices
#define MAX_DIM 16
#define MAX_ITER 100  // Maximum number of iterations for convergence

// Memory depth macros - critical for correct memory interface configuration
#define MAX_DATA_SIZE (MAX_DIM * MAX_DIM)
#define MAX_VECTOR_SIZE (MAX_DIM)
#define MAX_MATRIX_SIZE (MAX_DIM * MAX_DIM)

// The main PCA kernel function
// data: input data matrix (rows = samples, columns = features)
// mean: output mean vector (length = features)
// covariance: output covariance matrix (size = features x features)
// eigenvalues: output eigenvalues (length = features)
// eigenvectors: output eigenvectors (size = features x features)
// rows: number of samples in the input data
// cols: number of features in the input data
void pca_eigen_kernel(
    float* data,           // Input: data matrix
    float* mean,           // Output: mean vector
    float* covariance,     // Output: covariance matrix
    float* eigenvalues,    // Output: eigenvalues
    float* eigenvectors,   // Output: eigenvectors matrix
    int rows,              // Number of samples/rows
    int cols               // Number of features/columns
);

#endif // PCA_EIGEN_H