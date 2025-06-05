open_project -reset gemm_project
set_top gemm
add_files gemm.cpp
add_files -cflags "-std=c++11" vadd.h
open_solution "solution1" -reset
set_part {xcu250-figd2104-2L-e}
create_clock -period 3.3 -name default
csynth_design
add_files -tb gemm_tb.cpp
csim_design
cosim_design
exit