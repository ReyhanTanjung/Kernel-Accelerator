#ifndef _FDTD_H_
#define _FDTD_H_

#define GRID_SIZE 256
#define TIME_STEPS 100
#define C_CONSTANT 0.5f  // Wave propagation constant

typedef struct {
    float current[GRID_SIZE][GRID_SIZE];
    float previous[GRID_SIZE][GRID_SIZE];
    float next[GRID_SIZE][GRID_SIZE];
} wave_grid_t;

extern "C" {
    void fdtd_wave_propagation(
        const float *grid_current,
        const float *grid_previous, 
        float *grid_next,
        int grid_size,
        int time_step,
        float c_constant
    );
}

#endif