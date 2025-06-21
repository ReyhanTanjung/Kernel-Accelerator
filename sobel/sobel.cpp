#include "sobel.h"

void sobel_filter(const pixel_t *input, gradient_t *output, int width, int height) {
#pragma HLS INTERFACE m_axi port=input depth=1048576 offset=slave bundle=gmem0 max_read_burst_length=256
#pragma HLS INTERFACE m_axi port=output depth=1048576 offset=slave bundle=gmem1 max_write_burst_length=256
#pragma HLS INTERFACE s_axilite port=input bundle=control
#pragma HLS INTERFACE s_axilite port=output bundle=control
#pragma HLS INTERFACE s_axilite port=width bundle=control
#pragma HLS INTERFACE s_axilite port=height bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

    // Simple approach: directly access input array for 3x3 neighborhood
    // This is less optimal for very large images but much more reliable
    
    ROW_LOOP: for (int row = 0; row < height; row++) {
#pragma HLS LOOP_TRIPCOUNT min=64 max=1024 avg=256
        
        COL_LOOP: for (int col = 0; col < width; col++) {
#pragma HLS LOOP_TRIPCOUNT min=64 max=1024 avg=256
#pragma HLS PIPELINE II=1
            
            gradient_t result = 0;
            
            // Only compute for interior pixels (not on borders)
            if (row > 0 && row < height-1 && col > 0 && col < width-1) {
                
                // Get 3x3 neighborhood
                pixel_t p00 = input[(row-1) * width + (col-1)];
                pixel_t p01 = input[(row-1) * width + col];
                pixel_t p02 = input[(row-1) * width + (col+1)];
                pixel_t p10 = input[row * width + (col-1)];
                pixel_t p11 = input[row * width + col];
                pixel_t p12 = input[row * width + (col+1)];
                pixel_t p20 = input[(row+1) * width + (col-1)];
                pixel_t p21 = input[(row+1) * width + col];
                pixel_t p22 = input[(row+1) * width + (col+1)];
                
                // Apply Sobel X kernel: [[-1,0,1],[-2,0,2],[-1,0,1]]
                int grad_x = -p00 + p02 - 2*p10 + 2*p12 - p20 + p22;
                
                // Apply Sobel Y kernel: [[-1,-2,-1],[0,0,0],[1,2,1]]
                int grad_y = -p00 - 2*p01 - p02 + p20 + 2*p21 + p22;
                
                // Compute magnitude (sum of absolute values)
                grad_x = (grad_x < 0) ? -grad_x : grad_x;
                grad_y = (grad_y < 0) ? -grad_y : grad_y;
                result = grad_x + grad_y;
                
                // Clamp to prevent overflow
                if (result > 255) result = 255;
            }
            
            output[row * width + col] = result;
        }
    }
}