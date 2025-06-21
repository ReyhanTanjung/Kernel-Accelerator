#ifndef _FRACTAL_H_
#define _FRACTAL_H_

#include <ap_fixed.h>

// Fixed-point data types for better FPGA performance
typedef ap_fixed<32, 16> fixed_t;

// Image dimensions
#define WIDTH  64    // Smaller for co-simulation
#define HEIGHT 64
#define MAX_ITER 64  // Reduced iterations for faster simulation

// Fractal parameters structure
struct fractal_params {
    fixed_t x_min, x_max;
    fixed_t y_min, y_max;
    fixed_t julia_cx, julia_cy;  // Julia set parameters
    int fractal_type;            // 0 = Mandelbrot, 1 = Julia
    int max_iterations;
};

extern "C" {
    void fractal_kernel(
        unsigned char output[WIDTH * HEIGHT],
        fractal_params params_in,
        int width,
        int height
    );
}

#endif