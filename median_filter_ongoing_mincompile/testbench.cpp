#include <iostream>
#include <chrono>  // Untuk mengukur waktu eksekusi
#include "median_filter.h"

int main() {
    int input[ROWS][COLS] = {
        {10, 10, 80, 80, 10},
        {10, 50, 90, 70, 10},
        {30, 60, 100, 80, 20},
        {10, 40, 90, 60, 30},
        {10, 10, 70, 50, 10}
    };

    int output[ROWS][COLS] = {0};

    // Mulai pengukuran waktu
    auto start = std::chrono::high_resolution_clock::now();

    // Eksekusi fungsi filter median
    median_filter(input, output);

    // Selesai pengukuran waktu
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    // Cetak hasil filter median
    std::cout << "Hasil Filtering:\n";
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            std::cout << output[i][j] << " ";
        }
        std::cout << std::endl;
    }

    // Cetak waktu eksekusi
    std::cout << "Waktu eksekusi di CPU: " << duration.count() << " detik" << std::endl;

    return 0;
}
