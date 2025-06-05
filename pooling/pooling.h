#ifndef _POOLING_H_
#define _POOLING_H_

// Define constants for the pooling kernel
#define MAX_HEIGHT 224
#define MAX_WIDTH 224
#define MAX_CHANNELS 64
#define MAX_OUTPUT_HEIGHT ((MAX_HEIGHT-2)/2+1)  // For 2x2 pool with stride 2
#define MAX_OUTPUT_WIDTH ((MAX_WIDTH-2)/2+1)    // For 2x2 pool with stride 2
#define POOL_SIZE 2
#define POOL_STRIDE 2

// Maximum buffer sizes
#define INPUT_BUFFER_SIZE (MAX_CHANNELS * MAX_HEIGHT * MAX_WIDTH)
#define OUTPUT_BUFFER_SIZE (MAX_CHANNELS * MAX_OUTPUT_HEIGHT * MAX_OUTPUT_WIDTH)

typedef enum {POOL_MAX, POOL_AVG} pool_type;

extern "C" {
    // Main pooling function
    void pooling(
        const float input[INPUT_BUFFER_SIZE],  // Input feature map with explicit size
        float output[OUTPUT_BUFFER_SIZE],      // Output pooled feature map with explicit size
        int height,                            // Height of input feature map
        int width,                             // Width of input feature map
        int channels,                          // Number of channels
        int pool_size,                         // Size of pooling window
        int pool_stride,                       // Stride of pooling operation
        pool_type type                         // Type of pooling (MAX or AVG)
    );
}

#endif // _POOLING_H_