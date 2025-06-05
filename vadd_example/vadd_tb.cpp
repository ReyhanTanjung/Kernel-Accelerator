#include <iostream>
#include <cstdlib>
#include "vadd.h"

#define SIZE 1024

int main() {
    int a[SIZE], b[SIZE], c[SIZE];

    // Inisialisasi input
    for (int i = 0; i < SIZE; i++) {
        a[i] = i;
        b[i] = SIZE - i;
    }

    // Panggil fungsi yang akan disintesis
    vadd(a, b, c, SIZE);

    // Verifikasi hasil
    bool pass = true;
    for (int i = 0; i < SIZE; i++) {
        if (c[i] != a[i] + b[i]) {
            std::cout << "ERROR: c[" << i << "] = " << c[i]
                      << ", expected " << a[i] + b[i] << std::endl;
            pass = false;
            break;
        }
    }

    if (pass) {
        std::cout << "Test passed successfully.\n";
        return 0;
    } else {
        std::cout << "Test failed.\n";
        return 1;
    }
}
