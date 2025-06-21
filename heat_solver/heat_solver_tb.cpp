#include <iostream>
#include <fstream>
#include <cmath>
#include <cstring>
#include <chrono>
#include "heat_solver.h"

using namespace std;
using namespace std::chrono;

// Reference software implementation for verification
void heat_solver_sw(
    data_t* grid_in,
    data_t* grid_out,
    const data_t* boundary,
    const index_t width,
    const index_t height,
    const index_t iterations
) {
    data_t* current = new data_t[width * height];
    data_t* next = new data_t[width * height];
    
    // Initialize
    memcpy(current, grid_in, width * height * sizeof(data_t));
    
    for(index_t t = 0; t < iterations; t++) {
        // Interior points
        for(index_t i = 1; i < height-1; i++) {
            for(index_t j = 1; j < width-1; j++) {
                data_t center = current[i * width + j];
                data_t north = current[(i-1) * width + j];
                data_t south = current[(i+1) * width + j];
                data_t east = current[i * width + (j+1)];
                data_t west = current[i * width + (j-1)];
                
                data_t laplacian = (north + south + east + west - 4.0f * center);
                next[i * width + j] = center + ALPHA * laplacian;
            }
        }
        
        // Boundary conditions
        for(index_t j = 0; j < width; j++) {
            next[j] = boundary[j]; // Top
            next[(height-1) * width + j] = boundary[width + j]; // Bottom
        }
        for(index_t i = 1; i < height-1; i++) {
            next[i * width] = boundary[2*width + i]; // Left
            next[i * width + (width-1)] = boundary[2*width + height + i]; // Right
        }
        
        // Swap buffers
        data_t* temp = current;
        current = next;
        next = temp;
    }
    
    memcpy(grid_out, current, width * height * sizeof(data_t));
    
    delete[] current;
    delete[] next;
}

// Initialize test data
void initialize_test_data(
    data_t* grid,
    data_t* boundary,
    index_t width,
    index_t height
) {
    // Initialize grid with a hot spot in the center
    for(index_t i = 0; i < height; i++) {
        for(index_t j = 0; j < width; j++) {
            data_t x = (data_t)j / width;
            data_t y = (data_t)i / height;
            
            // Hot spot at center
            data_t dx = x - 0.5f;
            data_t dy = y - 0.5f;
            data_t r2 = dx*dx + dy*dy;
            
            if(r2 < 0.01f) {
                grid[i * width + j] = 100.0f; // Hot spot
            } else {
                grid[i * width + j] = 20.0f; // Ambient temperature
            }
        }
    }
    
    // Boundary conditions - fixed temperature boundaries
    for(index_t j = 0; j < width; j++) {
        boundary[j] = 20.0f; // Top boundary
        boundary[width + j] = 20.0f; // Bottom boundary
    }
    for(index_t i = 0; i < height; i++) {
        boundary[2*width + i] = 20.0f; // Left boundary
        boundary[2*width + height + i] = 20.0f; // Right boundary
    }
}

// Save results to file for visualization
void save_results(const char* filename, data_t* grid, index_t width, index_t height) {
    ofstream file(filename);
    if(file.is_open()) {
        for(index_t i = 0; i < height; i++) {
            for(index_t j = 0; j < width; j++) {
                file << grid[i * width + j];
                if(j < width-1) file << ",";
            }
            file << "\n";
        }
        file.close();
        cout << "Results saved to " << filename << endl;
    }
}

int main(int argc, char* argv[]) {
    // Test parameters - can be overridden by command line for cosim
    index_t width = 256;
    index_t height = 256;
    index_t iterations = 100;
    
    // Parse command line arguments for smaller cosim test
    if(argc >= 4) {
        width = atoi(argv[1]);
        height = atoi(argv[2]);
        iterations = atoi(argv[3]);
        cout << "Using command line parameters: " << width << "x" << height << ", " << iterations << " iterations" << endl;
    }
    const index_t grid_size = width * height;
    const index_t boundary_size = 2 * (width + height);
    
    cout << "=== Heat Equation Solver Test ===" << endl;
    cout << "Grid size: " << width << " x " << height << endl;
    cout << "Iterations: " << iterations << endl;
    cout << "Thermal diffusivity: " << THERMAL_DIFFUSIVITY << endl;
    cout << "Alpha (stability factor): " << ALPHA << endl;
    
    // Allocate memory
    data_t* grid_in = new data_t[grid_size];
    data_t* grid_out_hw = new data_t[grid_size];
    data_t* grid_out_sw = new data_t[grid_size];
    data_t* boundary = new data_t[boundary_size];
    
    // Initialize test data
    initialize_test_data(grid_in, boundary, width, height);
    
    cout << "\nInitial temperature range: ";
    data_t min_temp = grid_in[0], max_temp = grid_in[0];
    for(index_t i = 0; i < grid_size; i++) {
        min_temp = min(min_temp, grid_in[i]);
        max_temp = max(max_temp, grid_in[i]);
    }
    cout << min_temp << " to " << max_temp << " °C" << endl;
    
    // Run software reference
    cout << "\nRunning software reference..." << endl;
    auto start_sw = high_resolution_clock::now();
    heat_solver_sw(grid_in, grid_out_sw, boundary, width, height, iterations);
    auto end_sw = high_resolution_clock::now();
    auto duration_sw = duration_cast<milliseconds>(end_sw - start_sw);
    
    // Run hardware accelerated version
    cout << "Running hardware accelerated version..." << endl;
    auto start_hw = high_resolution_clock::now();
    heat_solver_2d(grid_in, grid_out_hw, boundary, width, height, iterations);
    auto end_hw = high_resolution_clock::now();
    auto duration_hw = duration_cast<milliseconds>(end_hw - start_hw);
    
    // Verify results
    cout << "\nVerifying results..." << endl;
    data_t max_error = 0.0f;
    data_t avg_error = 0.0f;
    index_t error_count = 0;
    
    for(index_t i = 0; i < grid_size; i++) {
        data_t error = abs(grid_out_hw[i] - grid_out_sw[i]);
        max_error = max(max_error, error);
        avg_error += error;
        if(error > 0.001f) error_count++;
    }
    avg_error /= grid_size;
    
    cout << "Max error: " << max_error << endl;
    cout << "Average error: " << avg_error << endl;
    cout << "Points with error > 0.001: " << error_count << endl;
    
    // Performance comparison
    cout << "\n=== Performance Results ===" << endl;
    cout << "Software time: " << duration_sw.count() << " ms" << endl;
    cout << "Hardware time: " << duration_hw.count() << " ms" << endl;
    if(duration_hw.count() > 0) {
        cout << "Speedup: " << (double)duration_sw.count() / duration_hw.count() << "x" << endl;
    }
    
    // Final temperature statistics
    min_temp = grid_out_hw[0]; max_temp = grid_out_hw[0];
    data_t avg_temp = 0.0f;
    for(index_t i = 0; i < grid_size; i++) {
        min_temp = min(min_temp, grid_out_hw[i]);
        max_temp = max(max_temp, grid_out_hw[i]);
        avg_temp += grid_out_hw[i];
    }
    avg_temp /= grid_size;
    
    cout << "\nFinal temperature range: " << min_temp << " to " << max_temp << " °C" << endl;
    cout << "Average temperature: " << avg_temp << " °C" << endl;
    
    // Save results for visualization
    save_results("initial_temperature.csv", grid_in, width, height);
    save_results("final_temperature_hw.csv", grid_out_hw, width, height);
    save_results("final_temperature_sw.csv", grid_out_sw, width, height);
    
    // Test result
    bool test_passed = (max_error < 0.01f) && (error_count < grid_size * 0.01f);
    cout << "\n=== Test Result ===" << endl;
    cout << (test_passed ? "PASSED" : "FAILED") << endl;
    
    // Cleanup
    delete[] grid_in;
    delete[] grid_out_hw;
    delete[] grid_out_sw;
    delete[] boundary;
    
    return test_passed ? 0 : 1;
}