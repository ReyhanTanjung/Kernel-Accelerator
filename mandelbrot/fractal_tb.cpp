#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cmath>
#include "fractal.h"

// Simple PPM image writer for visualization
void write_ppm(const char* filename, unsigned char* image, int width, int height) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cout << "Error: Could not open file " << filename << std::endl;
        return;
    }
    
    // PPM header
    file << "P2\n";
    file << width << " " << height << "\n";
    file << "255\n";
    
    // Write pixel data
    for (int i = 0; i < width * height; i++) {
        file << (int)image[i] << " ";
        if ((i + 1) % width == 0) file << "\n";
    }
    
    file.close();
    std::cout << "Image saved to " << filename << std::endl;
}

// Reference implementation for verification
int mandelbrot_ref(double x0, double y0, int max_iter) {
    double x = 0.0, y = 0.0;
    int iter = 0;
    
    while (iter < max_iter && (x*x + y*y) <= 4.0) {
        double x_temp = x*x - y*y + x0;
        y = 2*x*y + y0;
        x = x_temp;
        iter++;
    }
    
    return iter;
}

int main() {
    const int test_width = WIDTH;    // Use defined constants
    const int test_height = HEIGHT;
    
    // Allocate memory for output - static array for co-simulation
    static unsigned char hw_result[WIDTH * HEIGHT];
    static unsigned char sw_result[WIDTH * HEIGHT];
    
    // Set up fractal parameters - pass by value now
    fractal_params params;
    
    // Test 1: Mandelbrot set (classic view)
    std::cout << "Testing Mandelbrot set..." << std::endl;
    params.x_min = -2.5;
    params.x_max = 1.5;
    params.y_min = -2.0;
    params.y_max = 2.0;
    params.fractal_type = 0;  // Mandelbrot
    params.max_iterations = MAX_ITER;
    
    // Call hardware function - pass struct by value
    fractal_kernel(hw_result, params, test_width, test_height);
    
    // Generate reference result for a small region (for verification)
    double dx = (1.5 - (-2.5)) / test_width;
    double dy = (2.0 - (-2.0)) / test_height;
    
    int errors = 0;
    const int check_points = 100;  // Check only a subset for speed
    
    for (int i = 0; i < check_points; i++) {
        int row = rand() % test_height;
        int col = rand() % test_width;
        int idx = row * test_width + col;
        
        double x = -2.5 + col * dx;
        double y = -2.0 + row * dy;
        
        int ref_iter = mandelbrot_ref(x, y, MAX_ITER);
        unsigned char ref_color = (ref_iter >= MAX_ITER) ? 0 : (unsigned char)((ref_iter * 255) / MAX_ITER);
        
        // Allow small differences due to fixed-point precision
        if (abs((int)hw_result[idx] - (int)ref_color) > 5) {
            errors++;
            if (errors <= 5) {  // Show first few errors
                std::cout << "Error at (" << row << "," << col << "): "
                         << "HW=" << (int)hw_result[idx] 
                         << ", SW=" << (int)ref_color << std::endl;
            }
        }
    }
    
    if (errors == 0) {
        std::cout << "Mandelbrot test PASSED!" << std::endl;
    } else {
        std::cout << "Mandelbrot test FAILED with " << errors << " errors out of " 
                  << check_points << " checked points." << std::endl;
    }
    
    // Save Mandelbrot image
    write_ppm("mandelbrot_hw.ppm", hw_result, test_width, test_height);
    
    // Test 2: Julia set
    std::cout << "\nTesting Julia set..." << std::endl;
    params.x_min = -2.0;
    params.x_max = 2.0;
    params.y_min = -2.0;
    params.y_max = 2.0;
    params.julia_cx = -0.7;
    params.julia_cy = 0.27015;
    params.fractal_type = 1;  // Julia
    params.max_iterations = MAX_ITER;
    
    // Call hardware function
    fractal_kernel(hw_result, params, test_width, test_height);
    
    // Save Julia image
    write_ppm("julia_hw.ppm", hw_result, test_width, test_height);
    std::cout << "Julia set test completed (visual verification required)." << std::endl;
    
    // Test 3: Edge case - very small region
    std::cout << "\nTesting edge case (small region)..." << std::endl;
    params.x_min = -0.1;
    params.x_max = 0.1;
    params.y_min = -0.1;
    params.y_max = 0.1;
    params.fractal_type = 0;  // Mandelbrot
    params.max_iterations = MAX_ITER;
    
    fractal_kernel(hw_result, params, test_width, test_height);
    write_ppm("mandelbrot_zoom.ppm", hw_result, test_width, test_height);
    
    std::cout << "\nAll tests completed!" << std::endl;
    std::cout << "Generated images:" << std::endl;
    std::cout << "  - mandelbrot_hw.ppm (Mandelbrot set)" << std::endl;
    std::cout << "  - julia_hw.ppm (Julia set)" << std::endl;
    std::cout << "  - mandelbrot_zoom.ppm (Mandelbrot zoom)" << std::endl;
    
    return (errors == 0) ? 0 : 1;
}