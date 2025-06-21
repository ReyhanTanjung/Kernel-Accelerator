# Vitis HLS TCL script for Prefix Sum implementation
# Target: Alveo U250 FPGA

# Create and reset project
open_project -reset prefix_sum_project

# Set top-level function
set_top prefix_sum

# Add source files
add_files prefix_sum.cpp
add_files -cflags "-std=c++11" prefix_sum.h

# Create solution and configure target
open_solution "solution1" -reset
set_part {xcu250-figd2104-2L-e}

# Set clock period (3.3ns = ~300MHz)
create_clock -period 3.3 -name default

# Run C synthesis
csynth_design

# Add testbench and run simulation
add_files -tb prefix_sum_tb.cpp

# Run C simulation (functional verification)
csim_design

# Run co-simulation (RTL verification)
cosim_design

# Export RTL design
# export_design -format xo

# Exit HLS
exit