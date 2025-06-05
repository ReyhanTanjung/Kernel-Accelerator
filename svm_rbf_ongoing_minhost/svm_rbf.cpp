// svm_rbf.cpp
#include "svm_rbf.h"
#include <cmath>

void svm_rbf_kernel(
    const data_t *x_test,
    const data_t *support_vectors,
    const data_t *alphas,
    const data_t gamma,
    const data_t bias,
    const int n_features,
    const int n_sv,
    data_t *decision_value) {

// Interface pragmas
#pragma HLS INTERFACE m_axi port=x_test depth=MAX_FEATURES offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=support_vectors depth=MAX_SUPPORT_VECTORS*MAX_FEATURES offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi port=alphas depth=MAX_SUPPORT_VECTORS offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi port=decision_value depth=1 offset=slave bundle=gmem3

#pragma HLS INTERFACE s_axilite port=x_test bundle=control
#pragma HLS INTERFACE s_axilite port=support_vectors bundle=control
#pragma HLS INTERFACE s_axilite port=alphas bundle=control
#pragma HLS INTERFACE s_axilite port=gamma bundle=control
#pragma HLS INTERFACE s_axilite port=bias bundle=control
#pragma HLS INTERFACE s_axilite port=n_features bundle=control
#pragma HLS INTERFACE s_axilite port=n_sv bundle=control
#pragma HLS INTERFACE s_axilite port=decision_value bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

    // Local buffers for better performance
    data_t x_test_local[MAX_FEATURES];
    data_t sum = 0.0f;
    
    // Copy test vector to local memory
    COPY_X_TEST:
    for (int i = 0; i < n_features; i++) {
        #pragma HLS PIPELINE
        x_test_local[i] = x_test[i];
    }
    
    // Main computation loop
    SV_LOOP:
    for (int sv = 0; sv < n_sv; sv++) {
        data_t kernel_value = 0.0f;
        data_t squared_distance = 0.0f;
        
        // Compute squared Euclidean distance
        FEATURE_LOOP:
        for (int f = 0; f < n_features; f++) {
            #pragma HLS PIPELINE II=1
            data_t sv_feature = support_vectors[sv * n_features + f];
            data_t diff = x_test_local[f] - sv_feature;
            squared_distance += diff * diff;
        }
        
        // Compute RBF kernel: exp(-gamma * ||x-y||²)
        kernel_value = exp(-gamma * squared_distance);
        
        // Multiply by alpha and accumulate
        sum += alphas[sv] * kernel_value;
    }
    
    // Add bias term
    sum += bias;
    
    // Store the result
    *decision_value = sum;
}