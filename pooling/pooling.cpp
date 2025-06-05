#include "pooling.h"
#include <algorithm>

void pooling(
    const float input[INPUT_BUFFER_SIZE],
    float output[OUTPUT_BUFFER_SIZE],
    int height,
    int width,
    int channels,
    int pool_size,
    int pool_stride,
    pool_type type
) {
// Bundle all s_axilite interfaces into one bundle called 'control'
#pragma HLS INTERFACE s_axilite port=return bundle=control
#pragma HLS INTERFACE s_axilite port=height bundle=control
#pragma HLS INTERFACE s_axilite port=width bundle=control
#pragma HLS INTERFACE s_axilite port=channels bundle=control
#pragma HLS INTERFACE s_axilite port=pool_size bundle=control
#pragma HLS INTERFACE s_axilite port=pool_stride bundle=control
#pragma HLS INTERFACE s_axilite port=type bundle=control
// Use m_axi interfaces for high-bandwidth memory access
#pragma HLS INTERFACE m_axi port=input offset=slave bundle=gmem0 depth=INPUT_BUFFER_SIZE
#pragma HLS INTERFACE m_axi port=output offset=slave bundle=gmem1 depth=OUTPUT_BUFFER_SIZE
// Also include s_axilite for input and output in the same bundle
#pragma HLS INTERFACE s_axilite port=input bundle=control
#pragma HLS INTERFACE s_axilite port=output bundle=control

    // Calculate output dimensions
    int out_height = (height - pool_size) / pool_stride + 1;
    int out_width = (width - pool_size) / pool_stride + 1;

    // Input and output local caches
    float local_input[MAX_HEIGHT * MAX_WIDTH];
    float local_output[MAX_OUTPUT_HEIGHT * MAX_OUTPUT_WIDTH];

#pragma HLS ARRAY_PARTITION variable=local_input cyclic factor=POOL_SIZE*POOL_SIZE dim=1
#pragma HLS ARRAY_PARTITION variable=local_output cyclic factor=4 dim=1
    
    // Pooling implementation with loop optimizations
    channel_loop: for (int c = 0; c < channels; c++) {
#pragma HLS LOOP_TRIPCOUNT min=1 max=MAX_CHANNELS
        
        // Load channel data into local memory for faster access
        load_channel: for (int i = 0; i < height * width; i++) {
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=MAX_HEIGHT*MAX_WIDTH
            local_input[i] = input[c * height * width + i];
        }
        
        // Process each output row
        row_loop: for (int h = 0; h < out_height; h++) {
#pragma HLS LOOP_TRIPCOUNT min=1 max=MAX_OUTPUT_HEIGHT
            
            // Process each output column
            col_loop: for (int w = 0; w < out_width; w++) {
#pragma HLS LOOP_TRIPCOUNT min=1 max=MAX_OUTPUT_WIDTH
#pragma HLS PIPELINE II=1
                
                // Initialize for pooling
                float pool_result = (type == POOL_MAX) ? -3.40282e+38 : 0.0f;  // Min float value for max pooling
                
                // Inner loops for pooling window
                window_h: for (int ph = 0; ph < pool_size; ph++) {
#pragma HLS UNROLL
                    window_w: for (int pw = 0; pw < pool_size; pw++) {
#pragma HLS UNROLL
                        // Calculate input index
                        int in_row = h * pool_stride + ph;
                        int in_col = w * pool_stride + pw;
                        int in_idx = in_row * width + in_col;
                        
                        // Bounds checking
                        if (in_row < height && in_col < width) {
                            // Perform pooling based on type
                            if (type == POOL_MAX) {
                                pool_result = std::max(pool_result, local_input[in_idx]);
                            } else { // POOL_AVG
                                pool_result += local_input[in_idx] / (pool_size * pool_size);
                            }
                        }
                    }
                }
                
                // Store result to local buffer
                local_output[h * out_width + w] = pool_result;
            }
        }
        
        // Write channel results back to global memory
        store_channel: for (int i = 0; i < out_height * out_width; i++) {
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=MAX_OUTPUT_HEIGHT*MAX_OUTPUT_WIDTH
            output[c * out_height * out_width + i] = local_output[i];
        }
    }
}