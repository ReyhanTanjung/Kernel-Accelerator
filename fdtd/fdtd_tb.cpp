#include <iostream>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include "fdtd.h"

// Very small test for co-simulation debugging
#define SMALL_GRID_SIZE 8
#define SMALL_TIME_STEPS 2

int main() {
    const int total_elements = SMALL_GRID_SIZE * SMALL_GRID_SIZE;
    
    // Use simple arrays instead of dynamic allocation
    float grid_current[64];   // 8x8 = 64 elements max
    float grid_previous[64];
    float grid_next[64];
    float grid_expected[64];
    
    // Initialize with simple pattern
    for (int i = 0; i < total_elements; i++) {
        grid_current[i] = 0.0f;
        grid_previous[i] = 0.0f;
        grid_next[i] = 0.0f;
        grid_expected[i] = 0.0f;
    }
    
    // Set a simple initial condition - single point source
    int center_idx = (SMALL_GRID_SIZE/2) * SMALL_GRID_SIZE + (SMALL_GRID_SIZE/2);
    grid_current[center_idx] = 1.0f;
    grid_previous[center_idx] = 0.8f;
    
    std::cout << "=== Small FDTD Test for Co-simulation ===" << std::endl;
    std::cout << "Grid Size: " << SMALL_GRID_SIZE << "x" << SMALL_GRID_SIZE << std::endl;
    std::cout << "Total Elements: " << total_elements << std::endl;
    
    // Print initial state
    std::cout << "\nInitial grid_current:" << std::endl;
    for (int i = 0; i < SMALL_GRID_SIZE; i++) {
        for (int j = 0; j < SMALL_GRID_SIZE; j++) {
            std::cout << std::fixed << std::setprecision(2) 
                      << grid_current[i * SMALL_GRID_SIZE + j] << " ";
        }
        std::cout << std::endl;
    }
    
    // Run simulation for small number of steps
    for (int t = 0; t < SMALL_TIME_STEPS; t++) {
        std::cout << "\n--- Time Step " << t + 1 << " ---" << std::endl;
        
        // Call the FDTD function
        fdtd_wave_propagation(grid_current, grid_previous, grid_next, 
                             SMALL_GRID_SIZE, t, C_CONSTANT);
        
        // Calculate expected values for verification
        // Interior points only
        for (int i = 1; i < SMALL_GRID_SIZE - 1; i++) {
            for (int j = 1; j < SMALL_GRID_SIZE - 1; j++) {
                int idx = i * SMALL_GRID_SIZE + j;
                int north_idx = (i-1) * SMALL_GRID_SIZE + j;
                int south_idx = (i+1) * SMALL_GRID_SIZE + j;
                int west_idx = i * SMALL_GRID_SIZE + (j-1);
                int east_idx = i * SMALL_GRID_SIZE + (j+1);
                
                float laplacian = grid_current[north_idx] + grid_current[south_idx] + 
                                grid_current[west_idx] + grid_current[east_idx] - 
                                4.0f * grid_current[idx];
                
                grid_expected[idx] = 2.0f * grid_current[idx] - grid_previous[idx] + 
                                   C_CONSTANT * C_CONSTANT * laplacian;
            }
        }
        
        // Boundary conditions for expected
        for (int i = 0; i < SMALL_GRID_SIZE; i++) {
            grid_expected[i * SMALL_GRID_SIZE + 0] = 0.0f;                          // Left
            grid_expected[i * SMALL_GRID_SIZE + (SMALL_GRID_SIZE-1)] = 0.0f;         // Right
            grid_expected[0 * SMALL_GRID_SIZE + i] = 0.0f;                          // Top
            grid_expected[(SMALL_GRID_SIZE-1) * SMALL_GRID_SIZE + i] = 0.0f;         // Bottom
        }
        
        // Print results
        std::cout << "FPGA Result:" << std::endl;
        for (int i = 0; i < SMALL_GRID_SIZE; i++) {
            for (int j = 0; j < SMALL_GRID_SIZE; j++) {
                std::cout << std::fixed << std::setprecision(4) 
                          << grid_next[i * SMALL_GRID_SIZE + j] << " ";
            }
            std::cout << std::endl;
        }
        
        std::cout << "Expected Result:" << std::endl;
        for (int i = 0; i < SMALL_GRID_SIZE; i++) {
            for (int j = 0; j < SMALL_GRID_SIZE; j++) {
                std::cout << std::fixed << std::setprecision(4) 
                          << grid_expected[i * SMALL_GRID_SIZE + j] << " ";
            }
            std::cout << std::endl;
        }
        
        // Verify results
        bool test_passed = true;
        float max_error = 0.0f;
        const float tolerance = 1e-5f;
        
        for (int i = 0; i < total_elements; i++) {
            float error = fabs(grid_next[i] - grid_expected[i]);
            if (error > tolerance) {
                test_passed = false;
                int row = i / SMALL_GRID_SIZE;
                int col = i % SMALL_GRID_SIZE;
                std::cout << "ERROR at [" << row << "][" << col << "]: "
                          << "got " << grid_next[i] 
                          << ", expected " << grid_expected[i]
                          << ", error " << error << std::endl;
            }
            if (error > max_error) {
                max_error = error;
            }
        }
        
        std::cout << "Max Error: " << std::scientific << max_error << std::endl;
        
        if (test_passed) {
            std::cout << "✓ Time step " << t + 1 << " PASSED" << std::endl;
        } else {
            std::cout << "✗ Time step " << t + 1 << " FAILED" << std::endl;
            return 1;
        }
        
        // Update grids for next iteration
        for (int i = 0; i < total_elements; i++) {
            grid_previous[i] = grid_current[i];
            grid_current[i] = grid_next[i];
        }
    }
    
    std::cout << "\n=== Small Test Complete ===" << std::endl;
    return 0;
}