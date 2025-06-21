#include <iostream>
#include <vector>
#include <xrt.h>
#include <experimental/xrt_kernel.h>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>

// Host-side parameter structure (without ap_fixed dependencies)
struct fractal_params_host {
    float x_min, x_max;
    float y_min, y_max;
    float julia_cx, julia_cy;  // Julia set parameters
    int fractal_type;          // 0 = Mandelbrot, 1 = Julia
    int max_iterations;
};

// Image dimensions - must match the kernel
#define WIDTH  64
#define HEIGHT 64
#define MAX_ITER 64

// CPU implementation for comparison
class FractalCPU {
public:
    static int mandelbrot_iterations(double x0, double y0, int max_iter) {
        double x = 0.0, y = 0.0;
        double x_temp;
        int iter = 0;
        
        for (iter = 0; iter < max_iter; iter++) {
            if ((x * x + y * y) > 4.0) {
                break;
            }
            x_temp = x * x - y * y + x0;
            y = 2 * x * y + y0;
            x = x_temp;
        }
        
        return iter;
    }
    
    static int julia_iterations(double x0, double y0, double cx, double cy, int max_iter) {
        double x = x0, y = y0;
        double x_temp;
        int iter = 0;
        
        for (iter = 0; iter < max_iter; iter++) {
            if ((x * x + y * y) > 4.0) {
                break;
            }
            x_temp = x * x - y * y + cx;
            y = 2 * x * y + cy;
            x = x_temp;
        }
        
        return iter;
    }
    
    static unsigned char iterations_to_color(int iterations, int max_iter) {
        if (iterations >= max_iter) {
            return 0;  // Black for points in the set
        } else {
            return (unsigned char)((iterations * 255) / max_iter);
        }
    }
    
    static void compute_fractal(unsigned char* output, const fractal_params_host& params, 
                               int width, int height) {
        double dx = (params.x_max - params.x_min) / width;
        double dy = (params.y_max - params.y_min) / height;
        
        for (int row = 0; row < height; row++) {
            for (int col = 0; col < width; col++) {
                double x = params.x_min + col * dx;
                double y = params.y_min + row * dy;
                
                int iterations;
                
                if (params.fractal_type == 0) {
                    // Mandelbrot set
                    iterations = mandelbrot_iterations(x, y, params.max_iterations);
                } else {
                    // Julia set
                    iterations = julia_iterations(x, y, params.julia_cx, 
                                                params.julia_cy, params.max_iterations);
                }
                
                unsigned char color = iterations_to_color(iterations, params.max_iterations);
                output[row * width + col] = color;
            }
        }
    }
};

// Utility function to save image as PGM format
void save_pgm(const std::string& filename, const unsigned char* data, int width, int height) {
    std::ofstream file(filename);
    if (file.is_open()) {
        file << "P2\n";
        file << width << " " << height << "\n";
        file << "255\n";
        
        for (int i = 0; i < width * height; i++) {
            file << (int)data[i] << " ";
            if ((i + 1) % width == 0) file << "\n";
        }
        file.close();
        std::cout << "Image saved as: " << filename << std::endl;
    } else {
        std::cout << "Error: Could not save image file: " << filename << std::endl;
    }
}

// Function to compare two images and report differences
bool compare_images(const unsigned char* img1, const unsigned char* img2, 
                   int width, int height, const std::string& name1, const std::string& name2) {
    int differences = 0;
    int max_diff = 0;
    double total_diff = 0.0;
    
    for (int i = 0; i < width * height; i++) {
        int diff = abs((int)img1[i] - (int)img2[i]);
        if (diff > 0) {
            differences++;
            total_diff += diff;
            if (diff > max_diff) max_diff = diff;
        }
    }
    
    std::cout << "\n=== Image Comparison: " << name1 << " vs " << name2 << " ===" << std::endl;
    std::cout << "Total pixels: " << width * height << std::endl;
    std::cout << "Different pixels: " << differences << std::endl;
    std::cout << "Difference percentage: " << std::fixed << std::setprecision(2) 
              << (double(differences) / (width * height)) * 100.0 << "%" << std::endl;
    
    if (differences > 0) {
        std::cout << "Maximum difference: " << max_diff << std::endl;
        std::cout << "Average difference: " << std::fixed << std::setprecision(2) 
                  << total_diff / differences << std::endl;
    }
    
    bool match = (differences == 0);
    std::cout << "Match: " << (match ? "PERFECT" : "DIFFERENT") << std::endl;
    
    return match;
}

int main(int argc, char* argv[]) {
    std::cout << "=== Fractal Generator: FPGA vs CPU Comparison ===" << std::endl;
    
    // Parse command line arguments
    std::string xclbin_file = "fractal_hw.xclbin";
    if (argc > 1) {
        xclbin_file = argv[1];
    }
    
    // Image dimensions
    const int width = WIDTH;
    const int height = HEIGHT;
    const int image_size = width * height;
    
    std::cout << "Image dimensions: " << width << "x" << height << std::endl;
    std::cout << "Using xclbin file: " << xclbin_file << std::endl;
    
    // Test configurations
    std::vector<fractal_params_host> test_configs = {
        // Mandelbrot set - classic view
        {-2.5f, 1.0f, -1.25f, 1.25f, 0.0f, 0.0f, 0, MAX_ITER},
        
        // Mandelbrot set - zoomed in
        {-0.8f, -0.4f, -0.2f, 0.2f, 0.0f, 0.0f, 0, MAX_ITER},
        
        // Julia set - classic spiral
        {-1.5f, 1.5f, -1.5f, 1.5f, -0.7f, 0.27015f, 1, MAX_ITER},
        
        // Julia set - dragon fractal
        {-1.5f, 1.5f, -1.5f, 1.5f, -0.8f, 0.156f, 1, MAX_ITER}
    };
    
    std::vector<std::string> config_names = {
        "Mandelbrot_Classic",
        "Mandelbrot_Zoom",
        "Julia_Spiral", 
        "Julia_Dragon"
    };
    
    // Allocate memory for results
    std::vector<unsigned char> fpga_output(image_size);
    std::vector<unsigned char> cpu_output(image_size);
    
    try {
        // Setup XRT device and kernel
        std::cout << "\n=== Setting up FPGA ===" << std::endl;
        auto device = xrt::device(0);
        auto uuid = device.load_xclbin(xclbin_file);
        auto kernel = xrt::kernel(device, uuid, "fractal_kernel", 
                                 xrt::kernel::cu_access_mode::exclusive);
        
        // Create buffer for output
        auto output_buf = xrt::bo(device, image_size * sizeof(unsigned char), 
                                 kernel.group_id(0));
        auto output_map = output_buf.map<unsigned char*>();
        
        std::cout << "FPGA setup complete!" << std::endl;
        
        // Run tests for each configuration
        for (size_t config_idx = 0; config_idx < test_configs.size(); config_idx++) {
            const auto& params = test_configs[config_idx];
            const auto& config_name = config_names[config_idx];
            
            std::cout << "\n=== Testing Configuration: " << config_name << " ===" << std::endl;
            std::cout << "Fractal type: " << (params.fractal_type == 0 ? "Mandelbrot" : "Julia") << std::endl;
            std::cout << "X range: [" << params.x_min << ", " << params.x_max << "]" << std::endl;
            std::cout << "Y range: [" << params.y_min << ", " << params.y_max << "]" << std::endl;
            if (params.fractal_type == 1) {
                std::cout << "Julia constant: " << params.julia_cx << " + " 
                          << params.julia_cy << "i" << std::endl;
            }
            std::cout << "Max iterations: " << params.max_iterations << std::endl;
            
            // === CPU Computation ===
            std::cout << "\n--- CPU Computation ---" << std::endl;
            auto cpu_start = std::chrono::high_resolution_clock::now();
            
            FractalCPU::compute_fractal(cpu_output.data(), params, width, height);
            
            auto cpu_end = std::chrono::high_resolution_clock::now();
            double cpu_duration_ms = std::chrono::duration<double, std::milli>(cpu_end - cpu_start).count();
            
            std::cout << "CPU execution time: " << std::fixed << std::setprecision(2) 
                      << cpu_duration_ms << " ms" << std::endl;
            
            // === FPGA Computation ===
            std::cout << "\n--- FPGA Computation ---" << std::endl;
            
            // Clear output buffer
            std::fill(output_map, output_map + image_size, 0);
            output_buf.sync(XCL_BO_SYNC_BO_TO_DEVICE);
            
            auto fpga_start = std::chrono::high_resolution_clock::now();
            
            // Execute kernel - pass parameters as individual arguments
            // Note: The kernel expects fractal_params struct, but we need to pass 
            // the parameters in a way that matches the kernel interface
            auto run = kernel(output_buf, params, width, height);
            run.wait();
            
            auto fpga_end = std::chrono::high_resolution_clock::now();
            double fpga_duration_ms = std::chrono::duration<double, std::milli>(fpga_end - fpga_start).count();
            
            // Get results from FPGA
            output_buf.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
            std::copy(output_map, output_map + image_size, fpga_output.begin());
            
            std::cout << "FPGA execution time: " << std::fixed << std::setprecision(2) 
                      << fpga_duration_ms << " ms" << std::endl;
            
            // === Performance Analysis ===
            std::cout << "\n--- Performance Analysis ---" << std::endl;
            double speedup = cpu_duration_ms / fpga_duration_ms;
            std::cout << "Speedup (CPU/FPGA): " << std::fixed << std::setprecision(2) 
                      << speedup << "x" << std::endl;
            
            // Calculate throughput (pixels per second)
            double cpu_mpps = (image_size / 1000000.0) / (cpu_duration_ms / 1000.0); // Megapixels per second
            double fpga_mpps = (image_size / 1000000.0) / (fpga_duration_ms / 1000.0);
            
            std::cout << "CPU throughput: " << std::fixed << std::setprecision(2) 
                      << cpu_mpps << " Mpixels/s" << std::endl;
            std::cout << "FPGA throughput: " << std::fixed << std::setprecision(2) 
                      << fpga_mpps << " Mpixels/s" << std::endl;
            
            // === Verification ===
            bool images_match = compare_images(cpu_output.data(), fpga_output.data(), 
                                             width, height, "CPU", "FPGA");
            
            // === Save Images ===
            std::cout << "\n--- Saving Images ---" << std::endl;
            save_pgm(config_name + "_cpu.pgm", cpu_output.data(), width, height);
            save_pgm(config_name + "_fpga.pgm", fpga_output.data(), width, height);
            
            if (!images_match) {
                std::cout << "WARNING: CPU and FPGA results differ!" << std::endl;
            }
            
            std::cout << std::string(60, '=') << std::endl;
        }
        
        std::cout << "\n=== All Tests Completed Successfully ===" << std::endl;
        std::cout << "Generated images saved as .pgm files" << std::endl;
        std::cout << "You can convert to PNG using: convert filename.pgm filename.png" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}