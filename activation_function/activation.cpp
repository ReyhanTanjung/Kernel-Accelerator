#include "activation.h"
#include <hls_math.h>

// Pre-computed lookup tables for exponential function
static float exp_lut[LUT_SIZE];
static bool lut_initialized = false;

// Initialize LUT for exponential function
void init_exp_lut() {
#pragma HLS INLINE off
    if (!lut_initialized) {
        for (int i = 0; i < LUT_SIZE; i++) {
#pragma HLS PIPELINE
            // Range from -8 to 8 for efficient sigmoid/tanh coverage
            float x = (float)i * 16.0f / (float)LUT_SIZE - 8.0f;
            exp_lut[i] = hls::expf(x);
        }
        lut_initialized = true;
    }
}

// Get exponential value from LUT with linear interpolation
float exp_from_lut(float x) {
#pragma HLS INLINE
    // Clamp input to valid range [-8, 8]
    float clamped_x;
    if (x < -8.0f) {
        clamped_x = -8.0f;
    } else if (x > 8.0f) {
        clamped_x = 8.0f;
    } else {
        clamped_x = x;
    }
    
    // Scale to LUT index
    float scaled = (clamped_x + 8.0f) * (LUT_SIZE - 1) / 16.0f;
    int idx = (int)scaled;
    
    // Ensure valid index
    if (idx < 0) {
        idx = 0;
    } else if (idx >= LUT_SIZE - 1) {
        idx = LUT_SIZE - 2;
    }
    
    // Linear interpolation
    float frac = scaled - (float)idx;
    return exp_lut[idx] + frac * (exp_lut[idx + 1] - exp_lut[idx]);
}

// ReLU activation function
float relu(float x) {
#pragma HLS INLINE
    return (x > 0.0f) ? x : 0.0f;
}

// Sigmoid activation function using LUT
float sigmoid(float x) {
#pragma HLS INLINE
    float exp_neg_x = exp_from_lut(-x);
    return 1.0f / (1.0f + exp_neg_x);
}

// Tanh activation function using LUT
float tanh_activation(float x) {
#pragma HLS INLINE
    // tanh(x) = 2*sigmoid(2x) - 1
    float two_x = x * 2.0f;
    float exp_neg_2x = exp_from_lut(-two_x);
    float sig_2x = 1.0f / (1.0f + exp_neg_2x);
    return 2.0f * sig_2x - 1.0f;
}

// Main kernel function
void activation_kernel(const float *input, float *output, int size, int function_type) {
#pragma HLS INTERFACE m_axi port=input depth=1024 offset=slave bundle=gmem
#pragma HLS INTERFACE m_axi port=output depth=1024 offset=slave bundle=gmem
#pragma HLS INTERFACE s_axilite port=input bundle=control
#pragma HLS INTERFACE s_axilite port=output bundle=control
#pragma HLS INTERFACE s_axilite port=size bundle=control
#pragma HLS INTERFACE s_axilite port=function_type bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

    // Initialize lookup tables (only done once)
    init_exp_lut();
    
ACTIVATION_LOOP:
    for (int i = 0; i < size; i++) {
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=1024
        float val = input[i];
        
        // Apply the selected activation function
        switch (function_type) {
            case 0: // ReLU
                output[i] = relu(val);
                break;
            case 1: // Sigmoid
                output[i] = sigmoid(val);
                break;
            case 2: // Tanh
                output[i] = tanh_activation(val);
                break;
            default: // Default to identity function
                output[i] = val;
                break;
        }
    }
}