#include "conv2d.h"

void conv2d(
    const float input[MAX_INPUT_SIZE],
    const float kernel[MAX_KERNEL_SIZE_SQ],
    float output[MAX_OUTPUT_SIZE],
    int height,
    int width,
    int kernel_size
) {
    // Interface pragmas for AXI4 memory mapped interface
#pragma HLS INTERFACE m_axi port=input offset=slave bundle=gmem0 depth=MAX_INPUT_SIZE
#pragma HLS INTERFACE m_axi port=kernel offset=slave bundle=gmem1 depth=MAX_KERNEL_SIZE_SQ
#pragma HLS INTERFACE m_axi port=output offset=slave bundle=gmem2 depth=MAX_OUTPUT_SIZE
    
    // Control interface
#pragma HLS INTERFACE s_axilite port=input bundle=control
#pragma HLS INTERFACE s_axilite port=kernel bundle=control
#pragma HLS INTERFACE s_axilite port=output bundle=control
#pragma HLS INTERFACE s_axilite port=height bundle=control
#pragma HLS INTERFACE s_axilite port=width bundle=control
#pragma HLS INTERFACE s_axilite port=kernel_size bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

    // Local buffers for input and kernel
    float local_input[MAX_IMAGE_HEIGHT][MAX_IMAGE_WIDTH];
#pragma HLS ARRAY_PARTITION variable=local_input cyclic factor=2 dim=2
    
    float local_kernel[MAX_KERNEL_SIZE][MAX_KERNEL_SIZE];
#pragma HLS ARRAY_PARTITION variable=local_kernel complete dim=0
    
    // Copy input to local memory
    input_copy_y: for (int y = 0; y < height; y++) {
#pragma HLS LOOP_TRIPCOUNT min=1 max=MAX_IMAGE_HEIGHT
        input_copy_x: for (int x = 0; x < width; x++) {
#pragma HLS LOOP_TRIPCOUNT min=1 max=MAX_IMAGE_WIDTH
#pragma HLS PIPELINE
            local_input[y][x] = input[y * width + x];
        }
    }
    
    // Copy kernel to local memory
    kernel_copy_y: for (int ky = 0; ky < kernel_size; ky++) {
#pragma HLS LOOP_TRIPCOUNT min=1 max=MAX_KERNEL_SIZE
        kernel_copy_x: for (int kx = 0; kx < kernel_size; kx++) {
#pragma HLS LOOP_TRIPCOUNT min=1 max=MAX_KERNEL_SIZE
#pragma HLS PIPELINE
            local_kernel[ky][kx] = kernel[ky * kernel_size + kx];
        }
    }
    
    // Calculate output dimensions
    int output_height = height - kernel_size + 1;
    int output_width = width - kernel_size + 1;
    
    // Main convolution loops
    // y and x iterate over output pixels
    row_loop: for (int y = 0; y < output_height; y++) {
#pragma HLS LOOP_TRIPCOUNT min=1 max=MAX_IMAGE_HEIGHT
        col_loop: for (int x = 0; x < output_width; x++) {
#pragma HLS LOOP_TRIPCOUNT min=1 max=MAX_IMAGE_WIDTH
#pragma HLS PIPELINE II=1
            
            // Initialize accumulator for convolution at this output position
            float sum = 0.0f;
            
            // Convolution kernel application
            kernel_y: for (int ky = 0; ky < kernel_size; ky++) {
#pragma HLS LOOP_TRIPCOUNT min=1 max=MAX_KERNEL_SIZE
                kernel_x: for (int kx = 0; kx < kernel_size; kx++) {
#pragma HLS LOOP_TRIPCOUNT min=1 max=MAX_KERNEL_SIZE
#pragma HLS UNROLL factor=4
                    // Multiply and accumulate
                    sum += local_input[y + ky][x + kx] * local_kernel[ky][kx];
                }
            }
            
            // Store result to output
            output[y * output_width + x] = sum;
        }
    }
}