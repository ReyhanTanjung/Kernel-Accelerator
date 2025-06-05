open_project -reset conv2d_project
set_top conv2d
add_files conv2d.cpp
add_files -cflags "-std=c++11" conv2d.h
open_solution "solution1" -reset
set_part {xcu250-figd2104-2L-e}
create_clock -period 3.3 -name default
csynth_design
add_files -tb conv2d_tb.cpp
csim_design
cosim_design
exit