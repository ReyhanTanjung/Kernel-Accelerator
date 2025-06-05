#ifndef _GEMM_H_
#define _GEMM_H_

// Mendefinisikan dimensi matriks (M x K) * (K x N) = (M x N)
#define M 32
#define K 32
#define N 32

extern "C" {
    // Fungsi GEMM: C = alpha*A*B + beta*C
    void gemm(const float *A, const float *B, float *C, 
              float alpha, float beta, 
              int m, int k, int n);
}

#endif