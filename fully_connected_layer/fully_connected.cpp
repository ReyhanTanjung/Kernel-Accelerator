#include "fully_connected.h"

void fully_connected(const float *input, const float *weights, float *output, int input_size, int output_size) {
// Memory interface - IMPORTANT: use same bundle name for parameters
#pragma HLS INTERFACE mode=m_axi port=input bundle=gmem offset=slave depth=MAX_INPUT_SIZE
#pragma HLS INTERFACE mode=m_axi port=weights bundle=gmem offset=slave depth=MAX_INPUT_SIZE*MAX_OUTPUT_SIZE
#pragma HLS INTERFACE mode=m_axi port=output bundle=gmem offset=slave depth=MAX_OUTPUT_SIZE

// Control interface - IMPORTANT: use same bundle name for all s_axilite
#pragma HLS INTERFACE mode=s_axilite port=input bundle=control
#pragma HLS INTERFACE mode=s_axilite port=weights bundle=control
#pragma HLS INTERFACE mode=s_axilite port=output bundle=control
#pragma HLS INTERFACE mode=s_axilite port=input_size bundle=control
#pragma HLS INTERFACE mode=s_axilite port=output_size bundle=control
#pragma HLS INTERFACE mode=s_axilite port=return bundle=control

    // Ensure we don't exceed max buffer sizes
    int actual_input_size = (input_size > MAX_INPUT_SIZE) ? MAX_INPUT_SIZE : input_size;
    int actual_output_size = (output_size > MAX_OUTPUT_SIZE) ? MAX_OUTPUT_SIZE : output_size;
    
    // Process each output neuron one at a time
    for (int o = 0; o < actual_output_size; o++) {
        float sum = 0.0f;
        
        // Compute dot product for this output neuron
        for (int i = 0; i < actual_input_size; i++) {
#pragma HLS PIPELINE II=1
            sum += input[i] * weights[o * actual_input_size + i];
        }
        
        // Write result to output
        output[o] = sum;
    }
}