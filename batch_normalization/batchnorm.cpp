#include "batchnorm.h"
#include <cmath>  // Include for sqrt/sqrtf function

void batchnorm(const float *input, const float *gamma, const float *beta, 
              const float *mean, const float *variance, float *output, 
              int batch_size, float epsilon) {
#pragma HLS INTERFACE m_axi port=input depth=1024 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=gamma depth=1024 offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi port=beta depth=1024 offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi port=mean depth=1024 offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi port=variance depth=1024 offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi port=output depth=1024 offset=slave bundle=gmem0
#pragma HLS INTERFACE s_axilite port=input bundle=control
#pragma HLS INTERFACE s_axilite port=gamma bundle=control
#pragma HLS INTERFACE s_axilite port=beta bundle=control
#pragma HLS INTERFACE s_axilite port=mean bundle=control
#pragma HLS INTERFACE s_axilite port=variance bundle=control
#pragma HLS INTERFACE s_axilite port=output bundle=control
#pragma HLS INTERFACE s_axilite port=batch_size bundle=control
#pragma HLS INTERFACE s_axilite port=epsilon bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

    // Main computation loop
    // Each element is normalized using formula: y = gamma * (x - mean) / sqrt(variance + epsilon) + beta
    for (int i = 0; i < batch_size; i++) {
#pragma HLS PIPELINE II=1
        // Calculate normalized value
        float normalized = (input[i] - mean[i % N]) / sqrt(variance[i % N] + epsilon);
        
        // Scale and shift
        output[i] = gamma[i % N] * normalized + beta[i % N];
    }
}