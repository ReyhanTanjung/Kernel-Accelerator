# Create a Vitis HLS project
open_project -reset svm_rbf_project

# Set the top level function
set_top svm_rbf_kernel

# Add design files
add_files svm_rbf.cpp
add_files -cflags "-std=c++11" svm_rbf.h

# Add testbench files
add_files -tb svm_rbf_tb.cpp

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

# Run RTL co-simulation (optional - uncomment if needed)
cosim_design

exit
