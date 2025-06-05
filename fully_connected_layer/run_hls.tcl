open_project -reset fully_connected_project
set_top fully_connected
add_files fully_connected.cpp
add_files -cflags "-std=c++11" fully_connected.h
open_solution "solution1" -reset
set_part {xcu250-figd2104-2L-e}
create_clock -period 3.3 -name default
csynth_design
add_files -tb fully_connected_tb.cpp
csim_design
cosim_design
exit