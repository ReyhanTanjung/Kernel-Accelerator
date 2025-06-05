# AES Encryption HLS Synthesis Script - Safe Version (No Co-simulation)
open_project -reset aes_project
set_top aes_encrypt
add_files aes.cpp
add_files -cflags "-std=c++11" aes.h

# Open solution and set part
open_solution "solution1" -reset
set_part {xcu250-figd2104-2L-e}
create_clock -period 3.3 -name default

# Synthesis configuration for optimal pipelining
config_compile -pipeline_loops 64

# Add testbench for simulation
add_files -tb aes_tb.cpp

# Run C simulation first
puts "=== Running C Simulation ==="
csim_design

# Run C synthesis
puts "=== Running C Synthesis ==="
csynth_design

puts "=== Running COSIM ==="
cosim_design

# Optional: Export RTL (uncomment if needed)
# export_design -format ip_catalog

exit