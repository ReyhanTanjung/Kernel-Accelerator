# Create a Vitis HLS project
open_project -reset pca_eigen_project

# Set the top level function
set_top pca_eigen_kernel

# Add design files
add_files pca_eigen.cpp
add_files -cflags "-std=c++11" pca_eigen.h

# Add testbench files
add_files -tb pca_eigen_tb.cpp

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

# Export design (optional - uncomment if needed)
# export_design -format ip_catalog

exit