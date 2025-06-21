# ChaCha20 Encryption HLS Synthesis Script - Compatible Version
open_project -reset chacha20_project
set_top chacha20_encrypt
add_files chacha20.cpp
add_files -cflags "-std=c++11" chacha20.h

# Open solution and set part
open_solution "solution1" -reset
set_part {xcu250-figd2104-2L-e}
create_clock -period 3.3 -name default

# Basic synthesis configuration
config_compile -pipeline_loops 64

# Add testbench for simulation
add_files -tb chacha20_tb.cpp

# Run C simulation first
puts "=== Running ChaCha20 C Simulation ==="
csim_design

# Run C synthesis
puts "=== Running ChaCha20 C Synthesis ==="
csynth_design

# Run Co-simulation
puts "=== Running ChaCha20 COSIM ==="
cosim_design

# Optional: Export RTL IP (uncomment if needed for integration)
# export_design -format ip_catalog

puts "=== ChaCha20 HLS Synthesis Complete ==="

exit