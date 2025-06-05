# Create a Vitis HLS project
open_project -reset rsa_encryption_project

# Set the top level function
set_top rsa_encrypt

# Add design files
add_files rsa.cpp
add_files -cflags "-std=c++11 -DAP_INT_MAX_W=8192" rsa.h

# Add testbench files
add_files -tb rsa_tb.cpp

# Create a solution
open_solution solution1

# Target the Alveo U250
set_part xcu250-figd2104-2L-e

# Set the target clock period (300 MHz)
create_clock -period 3.3 -name default

# Run C simulation
csim_design

# Run C synthesis
csynth_design

# Run RTL co-simulation
cosim_design

exit