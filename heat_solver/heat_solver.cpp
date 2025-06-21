#include "heat_solver.h"
#include <hls_math.h>

// Main heat equation solver function
void heat_solver_2d(
    data_t* grid_in,           // Input temperature grid
    data_t* grid_out,          // Output temperature grid  
    const data_t* boundary,    // Boundary conditions
    const index_t width,       // Grid width
    const index_t height,      // Grid height
    const index_t iterations   // Number of time steps
) {
    // HLS Interface pragmas for AXI with depth specification
    #pragma HLS INTERFACE m_axi port=grid_in offset=slave bundle=gmem0 depth=262144
    #pragma HLS INTERFACE m_axi port=grid_out offset=slave bundle=gmem1 depth=262144
    #pragma HLS INTERFACE m_axi port=boundary offset=slave bundle=gmem2 depth=4096
    #pragma HLS INTERFACE s_axilite port=width bundle=control
    #pragma HLS INTERFACE s_axilite port=height bundle=control
    #pragma HLS INTERFACE s_axilite port=iterations bundle=control
    #pragma HLS INTERFACE s_axilite port=return bundle=control

    // Local memory buffers - ping-pong buffers for optimization
    static data_t grid_buffer_0[GRID_SIZE][GRID_SIZE];
    static data_t grid_buffer_1[GRID_SIZE][GRID_SIZE];
    
    #pragma HLS ARRAY_PARTITION variable=grid_buffer_0 cyclic factor=4 dim=2
    #pragma HLS ARRAY_PARTITION variable=grid_buffer_1 cyclic factor=4 dim=2

    // Initialize first buffer from input
    INIT_LOOP_I: for(index_t i = 0; i < height; i++) {
        #pragma HLS LOOP_TRIPCOUNT min=256 max=512
        INIT_LOOP_J: for(index_t j = 0; j < width; j++) {
            #pragma HLS LOOP_TRIPCOUNT min=256 max=512
            #pragma HLS PIPELINE II=1
            grid_buffer_0[i][j] = grid_in[i * width + j];
        }
    }

    // Time stepping loop
    TIME_LOOP: for(index_t t = 0; t < iterations; t++) {
        #pragma HLS LOOP_TRIPCOUNT min=100 max=1000
        
        if (t % 2 == 0) {
            // Even iterations: buffer_0 -> buffer_1
            heat_iteration_2d(grid_buffer_0, grid_buffer_1, boundary, width, height);
        } else {
            // Odd iterations: buffer_1 -> buffer_0  
            heat_iteration_2d(grid_buffer_1, grid_buffer_0, boundary, width, height);
        }
    }

    // Copy final result to output
    data_t (*final_buffer)[GRID_SIZE] = (iterations % 2 == 0) ? grid_buffer_0 : grid_buffer_1;
    
    OUTPUT_LOOP_I: for(index_t i = 0; i < height; i++) {
        #pragma HLS LOOP_TRIPCOUNT min=256 max=512
        OUTPUT_LOOP_J: for(index_t j = 0; j < width; j++) {
            #pragma HLS LOOP_TRIPCOUNT min=256 max=512
            #pragma HLS PIPELINE II=1
            grid_out[i * width + j] = final_buffer[i][j];
        }
    }
}

// Single iteration of heat equation using finite difference
void heat_iteration_2d(
    data_t grid_in[GRID_SIZE][GRID_SIZE],
    data_t grid_out[GRID_SIZE][GRID_SIZE],
    const data_t* boundary,
    const index_t width,
    const index_t height
) {
    #pragma HLS INLINE off

    // Stencil computation with 5-point finite difference
    STENCIL_LOOP_I: for(index_t i = 1; i < height-1; i++) {
        #pragma HLS LOOP_TRIPCOUNT min=254 max=510
        
        STENCIL_LOOP_J: for(index_t j = 1; j < width-1; j++) {
            #pragma HLS LOOP_TRIPCOUNT min=254 max=510
            #pragma HLS PIPELINE II=1
            #pragma HLS DEPENDENCE variable=grid_in inter false
            #pragma HLS DEPENDENCE variable=grid_out intra false
            
            // 5-point stencil: center, north, south, east, west
            data_t center = grid_in[i][j];
            data_t north  = grid_in[i-1][j];
            data_t south  = grid_in[i+1][j];
            data_t east   = grid_in[i][j+1];
            data_t west   = grid_in[i][j-1];
            
            // Finite difference formula for 2D heat equation
            // dT/dt = α * (d²T/dx² + d²T/dy²)
            // T_new = T_old + α*dt * [(T_i+1,j + T_i-1,j - 2*T_i,j)/dx² + 
            //                        (T_i,j+1 + T_i,j-1 - 2*T_i,j)/dy²]
            
            data_t laplacian = (north + south + east + west - 4.0f * center);
            grid_out[i][j] = center + ALPHA * laplacian;
        }
    }
    
    // Handle boundary conditions - copy boundaries
    BOUNDARY_TOP: for(index_t j = 0; j < width; j++) {
        #pragma HLS LOOP_TRIPCOUNT min=256 max=512
        #pragma HLS PIPELINE II=1
        grid_out[0][j] = boundary[j]; // Top boundary
    }
    
    BOUNDARY_BOTTOM: for(index_t j = 0; j < width; j++) {
        #pragma HLS LOOP_TRIPCOUNT min=256 max=512
        #pragma HLS PIPELINE II=1
        grid_out[height-1][j] = boundary[width + j]; // Bottom boundary
    }
    
    BOUNDARY_LEFT: for(index_t i = 1; i < height-1; i++) {
        #pragma HLS LOOP_TRIPCOUNT min=254 max=510
        #pragma HLS PIPELINE II=1
        grid_out[i][0] = boundary[2*width + i]; // Left boundary
    }
    
    BOUNDARY_RIGHT: for(index_t i = 1; i < height-1; i++) {
        #pragma HLS LOOP_TRIPCOUNT min=254 max=510
        #pragma HLS PIPELINE II=1
        grid_out[i][width-1] = boundary[2*width + height + i]; // Right boundary
    }
}