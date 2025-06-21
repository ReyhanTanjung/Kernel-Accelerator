# Vitis HLS script for Heat Equation Solver - Fixed version
# Based on the original vadd example structure with cosimulation fixes

# Create and reset project
open_project -reset heat_solver_project

# Set top function
set_top heat_solver_2d

# Add source files
add_files heat_solver.cpp
add_files -cflags "-std=c++11 -O3" heat_solver.h

# Open solution
open_solution "solution1" -reset

# Set target device (Alveo U250)
set_part {xcu250-figd2104-2L-e}

# Create clock constraint (3.3ns = ~300MHz)
create_clock -period 3.3 -name default

# Configure solution settings for better performance
config_interface -m_axi_latency 64
config_interface -m_axi_alignment_byte_size 64
config_interface -m_axi_max_widen_bitwidth 512

# Additional optimization settings
config_schedule -enable_dsp_full_reg=false
config_compile -name_max_length 80

# Run C synthesis
csynth_design

# Add testbench and run C simulation
add_files -tb heat_solver_tb.cpp
csim_design -clean

# Run cosimulation with proper settings for large arrays
# Note: Using smaller test size for cosimulation to reduce runtime
cosim_design

# Export design for implementation
# export_design -format ip_catalog -description "Heat Equation Solver using Finite Difference Method" -vendor "user" -library "hls" -version "1.0"

exit