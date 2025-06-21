# Create and reset project
open_project -reset fdtd_project
# Set top function
set_top fdtd_wave_propagation
# Add source files
add_files fdtd.cpp
add_files -cflags "-std=c++11 -I." fdtd.h
# Open solution and reset
open_solution "solution1" -reset
# Set target part (Alveo U250)
set_part {xcu250-figd2104-2L-e}
# Create clock constraint (3.3ns = ~303MHz)
create_clock -period 3.3 -name default
# Run C synthesis first
puts "=== Running C Synthesis ==="
csynth_design
# Add small testbench for co-simulation
add_files -tb fdtd_tb.cpp
# Run C simulation with small testbench
puts "=== Running C Simulation with Small Testbench ==="
csim_design
# Run co-simulation with minimal settings
puts "=== Running Co-simulation (Safe Mode) ==="
cosim_design -trace_level none -rtl verilog
# If successful, try with regular testbench
puts "=== Co-simulation completed successfully ==="
# Exit HLS
exit