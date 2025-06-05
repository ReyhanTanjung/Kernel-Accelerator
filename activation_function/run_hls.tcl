open_project -reset activation_project
set_top activation_kernel
add_files activation.cpp
add_files -cflags "-std=c++11" activation.h
open_solution "solution1" -reset
set_part {xcu250-figd2104-2L-e}
create_clock -period 3.3 -name default
config_compile -pipeline_loops 64
config_interface -m_axi_alignment_byte_size 64 -m_axi_max_widen_bitwidth 512
csynth_design
add_files -tb activation_tb.cpp
csim_design
cosim_design
export_design -format ip_catalog -rtl verilog
exit