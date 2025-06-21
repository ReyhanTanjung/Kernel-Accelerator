#ifndef HEAT_SOLVER_H
#define HEAT_SOLVER_H

#include <ap_int.h>
#include <hls_stream.h>

// Grid dimensions - dapat disesuaikan berdasarkan kebutuhan
#define GRID_SIZE 512
#define MAX_ITERATIONS 1000

// Data types
typedef float data_t;
typedef ap_uint<32> index_t;

// Thermal properties constants
#define THERMAL_DIFFUSIVITY 0.1f
#define DX 0.01f
#define DY 0.01f
#define DT 0.0001f

// Stability condition: dt <= dx²/(4*alpha)
#define ALPHA (THERMAL_DIFFUSIVITY * DT / (DX * DX))

// Function declarations
void heat_solver_2d(
    data_t* grid_in,           // Input temperature grid
    data_t* grid_out,          // Output temperature grid  
    const data_t* boundary,    // Boundary conditions
    const index_t width,       // Grid width
    const index_t height,      // Grid height
    const index_t iterations   // Number of time steps
);

// Helper function for single iteration
void heat_iteration_2d(
    data_t grid_in[GRID_SIZE][GRID_SIZE],
    data_t grid_out[GRID_SIZE][GRID_SIZE],
    const data_t* boundary,
    const index_t width,
    const index_t height
);

#endif // HEAT_SOLVER_H