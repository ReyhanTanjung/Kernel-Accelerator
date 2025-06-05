// #include "gemm.h"

// void gemm(const float *A, const float *B, float *C, 
//           float alpha, float beta, 
//           int m, int k, int n) {
// // Interface pragmas untuk AXI interfaces
// #pragma HLS INTERFACE m_axi port=A depth=1024 offset=slave bundle=gmem0
// #pragma HLS INTERFACE m_axi port=B depth=1024 offset=slave bundle=gmem1
// #pragma HLS INTERFACE m_axi port=C depth=1024 offset=slave bundle=gmem2
// #pragma HLS INTERFACE s_axilite port=A bundle=control
// #pragma HLS INTERFACE s_axilite port=B bundle=control
// #pragma HLS INTERFACE s_axilite port=C bundle=control
// #pragma HLS INTERFACE s_axilite port=alpha bundle=control
// #pragma HLS INTERFACE s_axilite port=beta bundle=control
// #pragma HLS INTERFACE s_axilite port=m bundle=control
// #pragma HLS INTERFACE s_axilite port=k bundle=control
// #pragma HLS INTERFACE s_axilite port=n bundle=control
// #pragma HLS INTERFACE s_axilite port=return bundle=control

//     // Local buffers untuk menyimpan potongan matriks untuk meningkatkan performa
//     float A_local[M][K];
//     float B_local[K][N];
//     float C_local[M][N];

// // Menyalin data dari memori global ke memori lokal
// load_A:
//     for (int i = 0; i < m; i++) {
//         for (int j = 0; j < k; j++) {
// #pragma HLS PIPELINE II=1
//             A_local[i][j] = A[i * k + j];
//         }
//     }

// load_B:
//     for (int i = 0; i < k; i++) {
//         for (int j = 0; j < n; j++) {
// #pragma HLS PIPELINE II=1
//             B_local[i][j] = B[i * n + j];
//         }
//     }

// load_C:
//     for (int i = 0; i < m; i++) {
//         for (int j = 0; j < n; j++) {
// #pragma HLS PIPELINE II=1
//             C_local[i][j] = C[i * n + j];
//         }
//     }

// // Perform matrix multiplication: C = alpha*A*B + beta*C
// compute_gemm:
//     for (int i = 0; i < m; i++) {
// #pragma HLS LOOP_TRIPCOUNT min=M max=M
//         for (int j = 0; j < n; j++) {
// #pragma HLS LOOP_TRIPCOUNT min=N max=N
// #pragma HLS PIPELINE II=1
//             float sum = 0.0f;
            
//             // Akumulasi dot product baris A dengan kolom B
//             for (int l = 0; l < k; l++) {
// #pragma HLS LOOP_TRIPCOUNT min=K max=K
// #pragma HLS UNROLL factor=8
//                 sum += A_local[i][l] * B_local[l][j];
//             }
            
//             // Terapkan alpha dan beta
//             C_local[i][j] = alpha * sum + beta * C_local[i][j];
//         }
//     }

// // Menyalin hasil kembali ke memori global
// store_C:
//     for (int i = 0; i < m; i++) {
//         for (int j = 0; j < n; j++) {
// #pragma HLS PIPELINE II=1
//             C[i * n + j] = C_local[i][j];
//         }
//     }
// }

#include "gemm.h"

void gemm(const float *A, const float *B, float *C, 
          float alpha, float beta, 
          int m, int k, int n) {
// Interface pragmas for AXI interfaces
#pragma HLS INTERFACE m_axi port=A depth=1024 offset=slave bundle=gmem0 max_read_burst_length=256
#pragma HLS INTERFACE m_axi port=B depth=1024 offset=slave bundle=gmem1 max_read_burst_length=256
#pragma HLS INTERFACE m_axi port=C depth=1024 offset=slave bundle=gmem2 max_read_burst_length=256 max_write_burst_length=256
#pragma HLS INTERFACE s_axilite port=A bundle=control
#pragma HLS INTERFACE s_axilite port=B bundle=control
#pragma HLS INTERFACE s_axilite port=C bundle=control
#pragma HLS INTERFACE s_axilite port=alpha bundle=control
#pragma HLS INTERFACE s_axilite port=beta bundle=control
#pragma HLS INTERFACE s_axilite port=m bundle=control
#pragma HLS INTERFACE s_axilite port=k bundle=control
#pragma HLS INTERFACE s_axilite port=n bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

    // Local buffers for matrices
    float A_local[M][K];
#pragma HLS ARRAY_PARTITION variable=A_local cyclic factor=8 dim=2
    
    float B_local[K][N];
#pragma HLS ARRAY_PARTITION variable=B_local cyclic factor=8 dim=1
    
    float C_local[M][N];
#pragma HLS ARRAY_PARTITION variable=C_local cyclic factor=8 dim=2
    
    // Copy input matrices from global memory to local buffers
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < k; j++) {
#pragma HLS PIPELINE II=1
            A_local[i][j] = A[i * k + j];
        }
    }
    
    for (int i = 0; i < k; i++) {
        for (int j = 0; j < n; j++) {
#pragma HLS PIPELINE II=1
            B_local[i][j] = B[i * n + j];
        }
    }
    
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
#pragma HLS PIPELINE II=1
            C_local[i][j] = C[i * n + j];
        }
    }
    
    // Compute GEMM: C = alpha * A * B + beta * C
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
#pragma HLS PIPELINE II=1
            float sum = 0.0f;
            
            // Unroll the innermost loop for better performance
#pragma HLS UNROLL factor=8
            for (int l = 0; l < k; l++) {
                sum += A_local[i][l] * B_local[l][j];
            }
            
            // Apply alpha and beta
            C_local[i][j] = alpha * sum + beta * C_local[i][j];
        }
    }
    
    // Copy result back to global memory
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
#pragma HLS PIPELINE II=1
            C[i * n + j] = C_local[i][j];
        }
    }
}