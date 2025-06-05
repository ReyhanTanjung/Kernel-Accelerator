#ifndef _BATCHNORM_H_
#define _BATCHNORM_H_

#define N 1024

extern "C" {
    // Batch Normalization function
    // input: input data to normalize
    // gamma: scaling parameter
    // beta: shifting parameter
    // mean: pre-computed mean for each channel
    // variance: pre-computed variance for each channel
    // output: normalized output
    // batch_size: number of elements in each batch
    // epsilon: small value to avoid division by zero
    void batchnorm(const float *input, const float *gamma, const float *beta, 
                  const float *mean, const float *variance, float *output, 
                  int batch_size, float epsilon);
}

#endif