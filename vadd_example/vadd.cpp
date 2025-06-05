#include "vadd.h"

void vadd(const int *a, const int *b, int *c, int size) {
#pragma HLS INTERFACE m_axi port=a depth=1024 offset=slave bundle=gmem
#pragma HLS INTERFACE m_axi port=b depth=1024 offset=slave bundle=gmem
#pragma HLS INTERFACE m_axi port=c depth=1024 offset=slave bundle=gmem
#pragma HLS INTERFACE s_axilite port=a bundle=control
#pragma HLS INTERFACE s_axilite port=b bundle=control
#pragma HLS INTERFACE s_axilite port=c bundle=control
#pragma HLS INTERFACE s_axilite port=size bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

    for (int i = 0; i < size; i++) {
#pragma HLS PIPELINE
        c[i] = a[i] + b[i];
    }
}