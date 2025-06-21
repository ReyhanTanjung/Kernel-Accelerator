#include "fdtd.h"
#include <cmath>

void fdtd_wave_propagation(
    const float *grid_current,
    const float *grid_previous, 
    float *grid_next,
    int grid_size,
    int time_step,
    float c_constant
) {
    // HLS Interface pragmas untuk optimasi memori dan kontrol
    // Set depth to accommodate maximum expected grid size
    #pragma HLS INTERFACE m_axi port=grid_current depth=65536 offset=slave bundle=gmem0
    #pragma HLS INTERFACE m_axi port=grid_previous depth=65536 offset=slave bundle=gmem1  
    #pragma HLS INTERFACE m_axi port=grid_next depth=65536 offset=slave bundle=gmem2
    #pragma HLS INTERFACE s_axilite port=grid_current bundle=control
    #pragma HLS INTERFACE s_axilite port=grid_previous bundle=control
    #pragma HLS INTERFACE s_axilite port=grid_next bundle=control
    #pragma HLS INTERFACE s_axilite port=grid_size bundle=control
    #pragma HLS INTERFACE s_axilite port=time_step bundle=control
    #pragma HLS INTERFACE s_axilite port=c_constant bundle=control
    #pragma HLS INTERFACE s_axilite port=return bundle=control

    // Calculate c^2 once
    float c_squared = c_constant * c_constant;

    // FDTD Wave Propagation Computation
    // Process interior points only (exclude boundaries)
    main_loop_i: for (int i = 1; i < grid_size - 1; i++) {
        main_loop_j: for (int j = 1; j < grid_size - 1; j++) {
            #pragma HLS PIPELINE II=1
            #pragma HLS LOOP_FLATTEN
            
            // Calculate current index and neighbor indices
            int idx = i * grid_size + j;
            int north_idx = (i - 1) * grid_size + j;
            int south_idx = (i + 1) * grid_size + j;
            int west_idx = i * grid_size + (j - 1);
            int east_idx = i * grid_size + (j + 1);
            
            // Read values from memory
            float center = grid_current[idx];
            float north = grid_current[north_idx];
            float south = grid_current[south_idx];
            float west = grid_current[west_idx];
            float east = grid_current[east_idx];
            float previous = grid_previous[idx];
            
            // Calculate Laplacian (discrete second derivative)
            float laplacian = north + south + west + east - 4.0f * center;
            
            // FDTD update formula: u_new = 2*u_current - u_previous + c^2 * laplacian
            float next_value = 2.0f * center - previous + c_squared * laplacian;
            
            // Write result
            grid_next[idx] = next_value;
        }
    }

    // Handle boundary conditions - set boundaries to zero
    // Top and bottom boundaries
    boundary_horizontal: for (int j = 0; j < grid_size; j++) {
        #pragma HLS PIPELINE II=1
        grid_next[0 * grid_size + j] = 0.0f;                    // Top row
        grid_next[(grid_size-1) * grid_size + j] = 0.0f;        // Bottom row
    }
    
    // Left and right boundaries  
    boundary_vertical: for (int i = 0; i < grid_size; i++) {
        #pragma HLS PIPELINE II=1
        grid_next[i * grid_size + 0] = 0.0f;                    // Left column
        grid_next[i * grid_size + (grid_size-1)] = 0.0f;        // Right column
    }
}