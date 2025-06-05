open_project matrix_mult_hls
set_top matrix_mult

add_files src/matrix_mult.cpp -cflags "-I./src"
add_files -tb src/matrix_mult_test.cpp -cflags "-I./src"

open_solution "solution1"
set_part xcu250-figd2104-2L-e

create_clock -period 2.5

csim_design
csynth_design
cosim_design
export_design -format ip_catalog

exit
