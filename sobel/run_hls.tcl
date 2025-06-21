# Sobel Filter HLS Project Script
open_project -reset sobel_project
set_top sobel_filter
add_files sobel.cpp
add_files -cflags "-std=c++11" sobel.h
open_solution "solution1" -reset
set_part {xcu250-figd2104-2L-e}
create_clock -period 3.3 -name default

# Run C synthesis
csynth_design

# Add testbench and run simulation
add_files -tb sobel_tb.cpp
csim_design

# Run co-simulation
cosim_design

# Export RTL for Vivado
# export_design -format ip_catalog

exit