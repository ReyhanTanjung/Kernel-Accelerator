open_project -reset batchnorm_project
set_top batchnorm
add_files batchnorm.cpp
add_files -cflags "-std=c++11" batchnorm.h
open_solution "solution1" -reset
set_part {xcu250-figd2104-2L-e}
create_clock -period 3.3 -name default
csynth_design
add_files -tb batchnorm_tb.cpp
csim_design
cosim_design
exit