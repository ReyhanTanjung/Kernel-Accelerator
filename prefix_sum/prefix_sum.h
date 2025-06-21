#ifndef _PREFIX_SUM_H_
#define _PREFIX_SUM_H_

#define N 1024  // Default array size for testbench

extern "C" {
    void prefix_sum(const int *input, int *output, int size);
}

#endif