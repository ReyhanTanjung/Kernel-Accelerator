open_project -reset softmax_project
set_top softmax
add_files softmax.cpp
add_files -cflags "-std=c++11" softmax.h
open_solution "solution1" -reset
set_part {xcu250-figd2104-2L-e}
create_clock -period 3.3 -name default
csynth_design
add_files -tb softmax_tb.cpp
csim_design
cosim_design
exit