# Blowfish Encryption HLS Synthesis Script
open_project -reset blowfish_project
set_top blowfish_encrypt
add_files blowfish.cpp
add_files -cflags "-std=c++11" blowfish.h
# Open solution and set part
open_solution "solution1" -reset
set_part {xcu250-figd2104-2L-e}
create_clock -period 3.3 -name default
# Synthesis configuration for optimal pipelining
config_compile -pipeline_loops 64
# Add testbench for simulation
add_files -tb blowfish_tb.cpp
# Run C simulation
puts "=== Running C Simulation ==="
csim_design
# Run C synthesis
puts "=== Running C Synthesis ==="
csynth_design
# Run co-simulation
puts "=== Running COSIM ==="
cosim_design
# Optional: Export RTL
# export_design -format ip_catalog
exit