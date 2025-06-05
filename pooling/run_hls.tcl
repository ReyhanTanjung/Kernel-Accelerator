open_project -reset pooling_project
set_top pooling
add_files pooling.cpp
add_files -cflags "-std=c++11" pooling.h
open_solution "solution1" -reset
set_part {xcu250-figd2104-2L-e}
create_clock -period 3.3 -name default
csynth_design
add_files -tb pooling_tb.cpp
csim_design
cosim_design
exit