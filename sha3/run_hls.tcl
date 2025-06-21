# SHA3-256 HLS Synthesis Script
open_project -reset sha3_project
set_top sha3_256

# Add source files
add_files sha3.cpp
add_files -cflags "-std=c++11" sha3.h

# Open solution
open_solution "solution1" -reset
set_part {xcu250-figd2104-2L-e}

# Set clock period (300 MHz = 3.33ns)
create_clock -period 3.33 -name default

# Set optimization directives
config_compile -name_max_length 50
config_schedule -enable_dsp_full_reg

# Run C synthesis
csynth_design

# Add testbench
add_files -tb sha3_tb.cpp

# Run C simulation
csim_design

# Run C/RTL co-simulation
cosim_design -trace_level all

# Export RTL for implementation
# export_design -format ip_catalog -description "SHA3-256 Hash Accelerator" -vendor "user" -library "hls" -version "1.0"

exit