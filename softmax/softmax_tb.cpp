#include <iostream>
#include <cmath>
#include <cstdlib>
#include <vector>
#include "softmax.h"

#define SIZE 100
#define EPSILON 1e-6

// Softmax CPU implementation untuk verifikasi
void computeSoftmaxCPU(const float* input, float* output, int size) {
    // Cari nilai maksimum untuk stabilitas numerik
    float max_val = input[0];
    for (int i = 1; i < size; i++) {
        if (input[i] > max_val) {
            max_val = input[i];
        }
    }

    // Hitung eksponensial dan jumlahnya
    float sum = 0.0f;
    std::vector<float> exp_values(size);
    
    for (int i = 0; i < size; i++) {
        exp_values[i] = expf(input[i] - max_val);
        sum += exp_values[i];
    }
    
    // Normalisasi
    for (int i = 0; i < size; i++) {
        output[i] = exp_values[i] / sum;
    }
}

int main() {
    // Alokasi memori dinamis untuk array input dan output
    float *input = new float[SIZE];
    float *output = new float[SIZE];
    float *expected = new float[SIZE];

    // Inisialisasi input dengan nilai acak
    std::cout << "Initializing input data...\n";
    for (int i = 0; i < SIZE; i++) {
        input[i] = static_cast<float>(rand() % 100) / 10.0f;
    }

    // Hitung hasil yang diharapkan menggunakan implementasi CPU
    std::cout << "Computing expected results on CPU...\n";
    computeSoftmaxCPU(input, expected, SIZE);

    // Panggil fungsi hardware accelerated
    std::cout << "Calling hardware accelerated softmax...\n";
    softmax(input, output, SIZE);

    // Verifikasi hasil
    std::cout << "Verifying results...\n";
    bool pass = true;
    for (int i = 0; i < SIZE; i++) {
        float diff = std::abs(output[i] - expected[i]);
        if (diff > EPSILON) {
            std::cout << "ERROR at index " << i << ": ";
            std::cout << "output[" << i << "] = " << output[i] << ", ";
            std::cout << "expected = " << expected[i] << ", ";
            std::cout << "diff = " << diff << std::endl;
            pass = false;
        }
    }

    // Periksa apakah probabilitas berjumlah 1
    float sum = 0.0f;
    for (int i = 0; i < SIZE; i++) {
        sum += output[i];
    }
    
    std::cout << "Probability sum check: " << sum << std::endl;
    
    if (std::abs(sum - 1.0f) > EPSILON) {
        std::cout << "ERROR: Sum of probabilities = " << sum << ", expected 1.0" << std::endl;
        pass = false;
    }

    if (pass) {
        std::cout << "Test passed successfully!\n";
        std::cout << "Sum of probabilities: " << sum << std::endl;
    } else {
        std::cout << "Test failed.\n";
    }

    // Bebaskan memori yang dialokasikan
    delete[] input;
    delete[] output;
    delete[] expected;

    return pass ? 0 : 1;
}