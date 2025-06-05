#include "median_filter.h"

void median_filter(int input[ROWS][COLS], int output[ROWS][COLS]) {
    #pragma HLS PIPELINE
    int window[9];

    for (int i = 1; i < ROWS - 1; i++) {
        for (int j = 1; j < COLS - 1; j++) {
            #pragma HLS UNROLL factor=3

            // Ambil 9 nilai dari jendela 3x3
            int k = 0;
            for (int x = -1; x <= 1; x++) {
                for (int y = -1; y <= 1; y++) {
                    window[k++] = input[i + x][j + y];
                }
            }

            // Urutkan array (Bubble Sort)
            for (int m = 0; m < 9; m++) {
                for (int n = m + 1; n < 9; n++) {
                    if (window[m] > window[n]) {
                        int temp = window[m];
                        window[m] = window[n];
                        window[n] = temp;
                    }
                }
            }

            // Ambil median (nilai tengah)
            output[i][j] = window[4];
        }
    }
}
