#include "fft.h"
#include <cmath>
#include <hls_math.h>

// Twiddle factors for FFT calculation
complex_t W[MAX_FFT_SIZE/2];

// Pre-compute twiddle factors for the FFT
void init_twiddle_factors(int size, bool inverse) {
#pragma HLS INLINE off
    float angle_norm = inverse ? 2.0f * M_PI / size : -2.0f * M_PI / size;
    
    for (int i = 0; i < size/2; i++) {
#pragma HLS PIPELINE
        float angle = angle_norm * i;
        data_t cos_val = hls::cos(angle);
        data_t sin_val = hls::sin(angle);
        W[i] = complex_t(cos_val, sin_val);
    }
}

// Main FFT implementation using Cooley-Tukey algorithm (decimation in time)
void fft_dit_core(complex_t data[MAX_FFT_SIZE], int size, bool inverse) {
    // Bit-reversal permutation
    int bits = 0;
    // Calculate number of bits needed to represent indices
    for (int temp = size; temp > 1; temp >>= 1) {
        bits++;
    }
    
    // In-place bit reversal
    for (int i = 0; i < size; i++) {
#pragma HLS LOOP_TRIPCOUNT min=1024 max=4096
        int rev = 0;
        for (int j = 0; j < bits; j++) {
#pragma HLS PIPELINE
            rev = (rev << 1) | ((i >> j) & 1);
        }
        
        // Swap elements if needed
        if (i < rev) {
            complex_t temp = data[i];
            data[i] = data[rev];
            data[rev] = temp;
        }
    }
    
    // Cooley-Tukey FFT implementation
    for (int step = 2; step <= size; step <<= 1) {
#pragma HLS LOOP_TRIPCOUNT min=10 max=12  // Log2 of max FFT size
        int half_step = step / 2;
        
        for (int i = 0; i < size; i += step) {
#pragma HLS LOOP_TRIPCOUNT min=256 max=2048
            for (int j = 0; j < half_step; j++) {
#pragma HLS PIPELINE
#pragma HLS UNROLL factor=4
                int twiddle_idx = j * (size / step);
                complex_t twiddle = W[twiddle_idx];
                
                int idx1 = i + j;
                int idx2 = i + j + half_step;
                
                complex_t temp = data[idx1];
                complex_t temp2 = data[idx2] * twiddle;
                
                data[idx1] = temp + temp2;
                data[idx2] = temp - temp2;
            }
        }
    }
    
    // Scale the outputs for IFFT
    if (inverse) {
        data_t scale = 1.0 / size;
        for (int i = 0; i < size; i++) {
#pragma HLS PIPELINE
            data[i] *= scale;
        }
    }
}

// Main FFT function that interfaces with the outside world
void fft(const complex_t* input, complex_t* output, int size, bool inverse) {
// Memory interface optimizations
#pragma HLS INTERFACE m_axi port=input depth=4096 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=output depth=4096 offset=slave bundle=gmem1
#pragma HLS INTERFACE s_axilite port=input bundle=control
#pragma HLS INTERFACE s_axilite port=output bundle=control
#pragma HLS INTERFACE s_axilite port=size bundle=control
#pragma HLS INTERFACE s_axilite port=inverse bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

// Local storage for data processing
static complex_t data_local[MAX_FFT_SIZE];

// Memory access optimizations
data_copy_loop:
    for (int i = 0; i < size; i++) {
#pragma HLS PIPELINE
        data_local[i] = input[i];
    }
    
    // Initialize twiddle factors for this FFT size
    init_twiddle_factors(size, inverse);
    
    // Perform the FFT operation
    fft_dit_core(data_local, size, inverse);
    
    // Copy results back
result_copy_loop:
    for (int i = 0; i < size; i++) {
#pragma HLS PIPELINE
        output[i] = data_local[i];
    }
}