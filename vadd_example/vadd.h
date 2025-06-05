#ifndef _VADD_H_
#define _VADD_H_

#define N 1024

extern "C" {
    void vadd(const int *a, const int *b, int *c, int size);
}

#endif
