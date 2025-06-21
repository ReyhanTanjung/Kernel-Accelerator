#include "prefix_sum.h"

void prefix_sum(const int *input, int *output, int size) {
    // Interface pragmas for AXI memory-mapped interfaces
    #pragma HLS INTERFACE m_axi port=input depth=1024 offset=slave bundle=gmem
    #pragma HLS INTERFACE m_axi port=output depth=1024 offset=slave bundle=gmem
    
    // Control interface pragmas
    #pragma HLS INTERFACE s_axilite port=input bundle=control
    #pragma HLS INTERFACE s_axilite port=output bundle=control
    #pragma HLS INTERFACE s_axilite port=size bundle=control
    #pragma HLS INTERFACE s_axilite port=return bundle=control
    
    // Perform prefix sum computation directly without large local arrays
    // First element is always the same
    output[0] = input[0];
    
    // Sequential prefix sum - read previous output value for accumulation
    COMPUTE_PREFIX: for (int i = 1; i < size; i++) {
        #pragma HLS PIPELINE II=2
        #pragma HLS LOOP_TRIPCOUNT min=1023 max=1023 avg=1023
        output[i] = output[i-1] + input[i];
    }
}