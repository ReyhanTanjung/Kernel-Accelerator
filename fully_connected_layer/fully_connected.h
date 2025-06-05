#ifndef _FULLY_CONNECTED_H_
#define _FULLY_CONNECTED_H_

// Define maximum buffer sizes
#define MAX_INPUT_SIZE 1024
#define MAX_OUTPUT_SIZE 1024

extern "C" {
    void fully_connected(const float *input, const float *weights, float *output, int input_size, int output_size);
}

#endif