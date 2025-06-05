#include "softmax.h"
#include <cmath>

void softmax(const float *input, float *output, int size) {
#pragma HLS INTERFACE m_axi port=input depth=1024 offset=slave bundle=gmem
#pragma HLS INTERFACE m_axi port=output depth=1024 offset=slave bundle=gmem
#pragma HLS INTERFACE s_axilite port=input bundle=control
#pragma HLS INTERFACE s_axilite port=output bundle=control
#pragma HLS INTERFACE s_axilite port=size bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

    // Pastikan size tidak melebihi MAX_SIZE
    int actual_size = (size > MAX_SIZE) ? MAX_SIZE : size;

    // Step 1: Find maximum value for numerical stability
    float max_val = input[0];
    for (int i = 1; i < actual_size; i++) {
#pragma HLS PIPELINE
        if (input[i] > max_val) {
            max_val = input[i];
        }
    }

    // Step 2: Compute exponentials and their sum
    float exp_sum = 0.0f;
    float exp_values[MAX_SIZE];

    for (int i = 0; i < actual_size; i++) {
#pragma HLS PIPELINE
        exp_values[i] = expf(input[i] - max_val);
        exp_sum += exp_values[i];
    }

    // Step 3: Normalize by dividing each exp value by the sum
    for (int i = 0; i < actual_size; i++) {
#pragma HLS PIPELINE
        output[i] = exp_values[i] / exp_sum;
    }
}