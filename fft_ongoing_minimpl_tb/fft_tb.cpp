#include "fft.h"
#include <cmath>
#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <vector>

// Reference FFT implementation for result verification
void reference_fft(complex_t* data, int size, bool inverse) {
    // Bit-reversal permutation
    int bits = 0;
    for (int temp = size; temp > 1; temp >>= 1) {
        bits++;
    }
    
    std::vector<int> rev(size);
    for (int i = 0; i < size; i++) {
        rev[i] = 0;
        for (int j = 0; j < bits; j++) {
            rev[i] = (rev[i] << 1) | ((i >> j) & 1);
        }
    }
    
    for (int i = 0; i < size; i++) {
        if (i < rev[i]) {
            std::swap(data[i], data[rev[i]]);
        }
    }
    
    // Cooley-Tukey FFT implementation
    double angle_norm = inverse ? 2.0 * M_PI / size : -2.0 * M_PI / size;
    
    for (int step = 2; step <= size; step <<= 1) {
        int half_step = step / 2;
        
        for (int i = 0; i < size; i += step) {
            for (int j = 0; j < half_step; j++) {
                int twiddle_idx = j * (size / step);
                double angle = angle_norm * twiddle_idx;
                complex_t twiddle(cos(angle), sin(angle));
                
                int idx1 = i + j;
                int idx2 = i + j + half_step;
                
                complex_t temp = data[idx1];
                complex_t temp2 = data[idx2] * twiddle;
                
                data[idx1] = temp + temp2;
                data[idx2] = temp - temp2;
            }
        }
    }
    
    // Scale the outputs for IFFT
    if (inverse) {
        double scale = 1.0 / size;
        for (int i = 0; i < size; i++) {
            data[i] *= scale;
        }
    }
}

// Function to generate random test data
void generate_test_data(complex_t* data, int size) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(-1.0, 1.0);
    
    for (int i = 0; i < size; i++) {
        data_t real_part = data_t(dist(gen));
        data_t imag_part = data_t(dist(gen));
        data[i] = complex_t(real_part, imag_part);
    }
}

// Function to generate sinusoidal test data for better verification
void generate_sinusoidal_data(complex_t* data, int size) {
    const int num_frequencies = 3;
    const double frequencies[num_frequencies] = {1.0, 5.0, 20.0};
    const double amplitudes[num_frequencies] = {1.0, 0.5, 0.25};
    
    for (int i = 0; i < size; i++) {
        double real_part = 0.0;
        for (int f = 0; f < num_frequencies; f++) {
            real_part += amplitudes[f] * cos(2 * M_PI * frequencies[f] * i / size);
        }
        data[i] = complex_t(data_t(real_part), data_t(0.0));
    }
}

// Function to compare results between HLS and reference implementation
bool compare_results(const complex_t* hls_result, const complex_t* ref_result, int size) {
    double max_error_real = 0.0;
    double max_error_imag = 0.0;
    double sum_error_real = 0.0;
    double sum_error_imag = 0.0;
    
    for (int i = 0; i < size; i++) {
        double error_real = std::abs(hls_result[i].real() - ref_result[i].real());
        double error_imag = std::abs(hls_result[i].imag() - ref_result[i].imag());
        
        max_error_real = std::max(max_error_real, error_real);
        max_error_imag = std::max(max_error_imag, error_imag);
        
        sum_error_real += error_real;
        sum_error_imag += error_imag;
    }
    
    double avg_error_real = sum_error_real / size;
    double avg_error_imag = sum_error_imag / size;
    
    std::cout << "Maximum error (real part): " << max_error_real << std::endl;
    std::cout << "Maximum error (imag part): " << max_error_imag << std::endl;
    std::cout << "Average error (real part): " << avg_error_real << std::endl;
    std::cout << "Average error (imag part): " << avg_error_imag << std::endl;
    
    // Define a threshold for acceptable error (adjust as needed)
    const double threshold = 1e-3;
    return (max_error_real < threshold && max_error_imag < threshold);
}

// Function to save results to file for further analysis
void save_results_to_file(const complex_t* data, int size, const std::string& filename) {
    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for writing." << std::endl;
        return;
    }
    
    outfile << "Index,Real,Imaginary,Magnitude\n";
    for (int i = 0; i < size; i++) {
        double real = data[i].real();
        double imag = data[i].imag();
        double mag = std::sqrt(real*real + imag*imag);
        outfile << i << "," << real << "," << imag << "," << mag << "\n";
    }
    
    outfile.close();
}

// Function to print results for debugging
void print_results(const complex_t* data, int size, const std::string& label) {
    std::cout << label << " (showing first 16 elements):" << std::endl;
    for (int i = 0; i < std::min(16, size); i++) {
        std::cout << i << ": " << data[i].real() << " + " << data[i].imag() << "i" << std::endl;
    }
    std::cout << std::endl;
}

// Main testbench function
int main() {
    // Test parameters
    const int test_sizes[] = {16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
    const int num_test_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);
    
    // Arrays for input and output data
    complex_t* input = new complex_t[MAX_FFT_SIZE];
    complex_t* output_hls = new complex_t[MAX_FFT_SIZE];
    complex_t* output_ref = new complex_t[MAX_FFT_SIZE];
    
    bool all_passed = true;
    
    // Run tests for different FFT sizes
    for (int t = 0; t < num_test_sizes; t++) {
        int size = test_sizes[t];
        std::cout << "=======================================" << std::endl;
        std::cout << "Testing FFT with size = " << size << std::endl;
        
        // Generate test data
        generate_sinusoidal_data(input, size);
        
        // Save input data for reference
        save_results_to_file(input, size, "input_data_" + std::to_string(size) + ".csv");
        
        // Make a copy for reference implementation
        for (int i = 0; i < size; i++) {
            output_ref[i] = input[i];
        }
        
        // Run HLS implementation
        fft(input, output_hls, size, false);
        
        // Run reference implementation
        reference_fft(output_ref, size, false);
        
        // Save output data for reference
        save_results_to_file(output_hls, size, "output_hls_" + std::to_string(size) + ".csv");
        save_results_to_file(output_ref, size, "output_ref_" + std::to_string(size) + ".csv");
        
        // Print some results for debugging
        print_results(input, size, "Input Data");
        print_results(output_hls, size, "HLS FFT Output");
        print_results(output_ref, size, "Reference FFT Output");
        
        // Compare results
        std::cout << "Comparing HLS and reference implementation results:" << std::endl;
        bool test_passed = compare_results(output_hls, output_ref, size);
        if (test_passed) {
            std::cout << "TEST PASSED for size " << size << std::endl;
        } else {
            std::cout << "TEST FAILED for size " << size << std::endl;
            all_passed = false;
        }
        
        // Now test IFFT (inverse FFT)
        std::cout << "\nTesting IFFT with size = " << size << std::endl;
        
        // Use FFT output as IFFT input
        complex_t* ifft_input = output_hls;
        complex_t* ifft_output_hls = new complex_t[MAX_FFT_SIZE];
        complex_t* ifft_output_ref = new complex_t[MAX_FFT_SIZE];
        
        // Make a copy for reference implementation
        for (int i = 0; i < size; i++) {
            ifft_output_ref[i] = ifft_input[i];
        }
        
        // Run HLS IFFT implementation
        fft(ifft_input, ifft_output_hls, size, true);
        
        // Run reference IFFT implementation
        reference_fft(ifft_output_ref, size, true);
        
        // Save IFFT output data for reference
        save_results_to_file(ifft_output_hls, size, "ifft_output_hls_" + std::to_string(size) + ".csv");
        
        // Print some results for debugging
        print_results(ifft_output_hls, size, "HLS IFFT Output");
        print_results(ifft_output_ref, size, "Reference IFFT Output");
        
        // Compare IFFT results with original input
        std::cout << "Comparing IFFT output with original input:" << std::endl;
        bool ifft_test_passed = compare_results(ifft_output_hls, input, size);
        if (ifft_test_passed) {
            std::cout << "IFFT TEST PASSED for size " << size << std::endl;
        } else {
            std::cout << "IFFT TEST FAILED for size " << size << std::endl;
            all_passed = false;
        }
        
        delete[] ifft_output_hls;
        delete[] ifft_output_ref;
    }
    
    // Clean up
    delete[] input;
    delete[] output_hls;
    delete[] output_ref;
    
    // Final result
    if (all_passed) {
        std::cout << "\nALL TESTS PASSED!" << std::endl;
        return 0;
    } else {
        std::cout << "\nSOME TESTS FAILED!" << std::endl;
        return 1;
    }
}