# BLAKE2s HLS Synthesis Script
open_project -reset blake2s_project
set_top blake2s_hash

# Add source files
add_files blake2s.cpp
add_files -cflags "-std=c++11" blake2s.h

# Open solution
open_solution "solution1" -reset
set_part {xcu250-figd2104-2L-e}

# Create clock constraint (3.33ns = 300MHz)
create_clock -period 3.33 -name default

# Set optimization directives
config_compile -name_max_length 50
config_rtl -reset control
config_interface -m_axi_latency 64
config_interface -m_axi_alignment_byte_size 64

# Run C synthesis
csynth_design

# Add testbench
add_files -tb blake2s_tb.cpp

# Run C simulation
csim_design

# Run C/RTL co-simulation
cosim_design -rtl verilog -trace_level all

# Export design
# export_design -format ip_catalog -output blake2s_ip

exit