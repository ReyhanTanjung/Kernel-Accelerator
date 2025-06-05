# HLS script for Fast Fourier Transform accelerator

# Create a new project
open_project -reset fft_project

# Set the top-level function
set_top fft

# Add source files
add_files fft.cpp
add_files -cflags "-std=c++11" fft.h

# Add testbench files
add_files -tb fft_tb.cpp

# Create solution
open_solution "solution1" -reset

# Set the target device (Xilinx Alveo U250)
set_part {xcu250-figd2104-2L-e}

# Set clock period (300 MHz = 3.3ns)
create_clock -period 3.3 -name default

# Configure HLS optimization directives
config_compile -pipeline_loops 64
config_schedule -enable_dsp_full_reg=true
config_interface -m_axi_addr64

# Synthesis and analysis of the design
csynth_design

# Run C/RTL co-simulation to verify the design
cosim_design -trace_level port

# Export the RTL as an IP for later integration
# export_design -format ip_catalog

# Exit the HLS tool
exit