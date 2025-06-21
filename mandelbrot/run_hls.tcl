# Fractal HLS Project TCL Script
open_project -reset fractal_project
set_top fractal_kernel
add_files fractal.cpp
add_files -cflags "-std=c++11" fractal.h
open_solution "solution1" -reset
set_part {xcu250-figd2104-2L-e}
create_clock -period 3.3 -name default

# Configure synthesis options for better performance
config_compile -name_max_length 50
config_interface -m_axi_addr64
config_rtl -register_reset_num 3

# Run C synthesis
csynth_design

# Add testbench and run simulation
add_files -tb fractal_tb.cpp
csim_design

# Run co-simulation
cosim_design -rtl verilog -trace_level all

# Export for Vivado (optional)
# export_design -format ip_catalog

exit