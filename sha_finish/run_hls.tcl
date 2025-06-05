# SHA-256 Hashing HLS Synthesis Script
open_project -reset sha256_project
set_top sha256_hash
add_files sha256.cpp
add_files -cflags "-std=c++11" sha256.h

# Open solution and set part
open_solution "solution1" -reset
set_part {xcu250-figd2104-2L-e}
create_clock -period 3.3 -name default

# Synthesis configuration for optimal pipelining
config_compile -pipeline_loops 64
config_dataflow -default_channel fifo -fifo_depth 2

# Add testbench for simulation
add_files -tb sha256_tb.cpp

# Run C simulation first
puts "=== Running C Simulation ==="
csim_design

# Run C synthesis
puts "=== Running C Synthesis ==="
csynth_design

# Run Co-simulation
puts "=== Running COSIM ==="
cosim_design -trace_level all

# Export RTL for integration
# export_design -format ip_catalog -output "./sha256_ip"

exit