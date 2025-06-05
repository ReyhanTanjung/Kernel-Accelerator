# Create Vitis HLS project
open_project kmeans_hls
set_top kmeans_kernel
add_files kmeans.cpp
add_files -tb kmeans_tb.cpp

# Configure solution
open_solution "solution1" -flow_target vitis
set_part {xcu250-figd2104-2L-e}
create_clock -period 10 -name default

# Run C simulation
csim_design

# Run synthesis
csynth_design

# Run C/RTL co-simulation with reduced points
cosim_design -trace_level all -rtl verilog -argv "-points 32 -clusters 2 -dimensions 2 -iterations 10"

# Export RTL
# export_design -format ip_catalog

exit