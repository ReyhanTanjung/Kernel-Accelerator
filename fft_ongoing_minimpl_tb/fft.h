#ifndef FFT_H_
#define FFT_H_
#include <complex>
#include <ap_fixed.h>

// Define data types for better precision and hardware efficiency
typedef ap_fixed<16, 8> data_t;  // 16-bit fixed point with 8 integer bits
typedef std::complex<data_t> complex_t;

// Maximum FFT size - power of 2 for efficient implementation
#define MAX_FFT_SIZE 4096

// FFT Interface
void fft(const complex_t* input, complex_t* output, int size, bool inverse);

#endif // FFT_H_