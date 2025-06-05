#ifndef _ACTIVATION_H_
#define _ACTIVATION_H_

#define N 1024
#define LUT_SIZE 1024  // Size of lookup table for sigmoid and tanh

extern "C" {
    // Main kernel function that applies activation function to an array
    void activation_kernel(const float *input, float *output, int size, int function_type);
}

#endif // _ACTIVATION_H_