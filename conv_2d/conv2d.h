#ifndef _CONV2D_H_
#define _CONV2D_H_

// Define constants for maximum dimensions
// Smaller test sizes to avoid memory issues
#define MAX_IMAGE_HEIGHT 64
#define MAX_IMAGE_WIDTH 64
#define MAX_KERNEL_SIZE 7

// Maximum buffer sizes
#define MAX_INPUT_SIZE (MAX_IMAGE_HEIGHT * MAX_IMAGE_WIDTH)
#define MAX_KERNEL_SIZE_SQ (MAX_KERNEL_SIZE * MAX_KERNEL_SIZE)
#define MAX_OUTPUT_SIZE (MAX_IMAGE_HEIGHT * MAX_IMAGE_WIDTH)

extern "C" {
    // 2D Convolution function
    // input: input image (H x W)
    // kernel: convolution kernel (K x K)
    // output: output image ((H-K+1) x (W-K+1))
    // height: height of input image
    // width: width of input image
    // kernel_size: size of the kernel (assuming square kernel K x K)
    void conv2d(
        const float input[MAX_INPUT_SIZE],
        const float kernel[MAX_KERNEL_SIZE_SQ],
        float output[MAX_OUTPUT_SIZE],
        int height,
        int width,
        int kernel_size
    );
}

#endif // _CONV2D_H_