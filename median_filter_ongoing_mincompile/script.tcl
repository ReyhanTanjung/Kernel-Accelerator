open_project median_project
set_top median_filter
add_files median_filter.cpp
add_files -tb testbench.cpp
open_solution "solution1"
set_part xczu9eg-ffvb1156-2-i
create_clock -period 10
csynth_design
exit
