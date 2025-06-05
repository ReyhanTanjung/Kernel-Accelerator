#ifndef _SOFTMAX_H_
#define _SOFTMAX_H_

#define MAX_SIZE 1024

extern "C" {
    void softmax(const float *input, float *output, int size);
}

#endif