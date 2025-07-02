# FPGA Kernel Accelerator - Vitis HLS Alveo U250

## Deskripsi

Dokumen ini bertujuan untuk melakukan profiling dan implementasi berbagai algoritma dengan tujuan akselerasi komputasi menggunakan FPGA Xilinx Alveo U250. Fokus utama adalah pemanfaatan High-Level Synthesis (HLS) dalam mengembangkan solusi hardware yang efisien untuk aplikasi di berbagai domain seperti Artificial Intelligence (AI), Science, Communication, Security, dan Finance. Dengan melakukan profiling, diharapkan dapat diidentifikasi bagian-bagian komputasi yang paling menuntut sumber daya serta cocok untuk diakselerasi pada FPGA, sehingga meningkatkan performa dan efisiensi sistem secara keseluruhan.

---

## Struktur

```
vitis_hls_project/
├── <nama_kernel_1>/
├── <nama_kernel_2>/
├── <nama_kernel_n>/
├── .gitignore
├── Guide.md
└── Readme.md
```

---

## Status Implementasi Kernel

### AI & Machine Learning

| Nama Algoritma                           | C-Sim | C-Synth | Co-Sim | Export IP | Catatan                                  |
|-----------------------------------------|:-----:|:-------:|:------:|:---------:|-------------------------------------------|
| `activation_function`                   | [x]   | [x]     | [x]    | [x]       |                                           |
| `batch_normalization`                   | [x]   | [x]     | [x]    | [x]       |                                           |
| `convolution_2d`                               | [x]   | [x]     | [x]    | [x]       |                                           |
| `fully_connected_layer`                 | [x]   | [x]     | [x]    | [x]       |                                           |
| `gemm`                                  | [x]   | [x]     | [x]    | [x]       |                                           |
| `k_mean_clustering`        | [x]   | [x]     | [x]    | [x]       | *Ongoing, host code*                      |
| `pca_eigenvalue` | [x] | [x]     | [x]    | [x]       | *Ongoing, min host, eigenvalue*           |
| `pooling`                               | [x]   | [x]     | [x]    | [x]       |                                           |
| `softmax`                               | [x]   | [x]     | [x]    | [x]       |                                           |
| `svm_rbf`              | [x]   | [x]     | [x]    | [x]       | *Ongoing, min host, RBF kernel*           |

### Science

| Nama Algoritma                          | C-Sim | C-Synth | Co-Sim | Export IP | Catatan                                  |
|----------------------------------------|:-----:|:-------:|:------:|:---------:|-------------------------------------------|
| `fdtd`              | [x]   | [x]     | [ ]    | [ ]       | *Segmentation violation in COSIM*            |
| `fft`    | [x]   | [x]     | []    | [ ]       | *Segmentation violation in COSIM*                    |
| `heat_equation_solver`              | [x]   | [x]     | [ ]    | [ ]       | *Segmentation vioation in COSIM*            |
| `mandelbrot`    | [x]   | [x]     | [x]    | [x]       |                     |
| `prefix_sum`              | [x]   | [x]     | [x]    | [ ]       | *IP Export*            |
| `sobel_edge_detection`    | [x]   | [x]     | []    | [ ]       | *Segmentation violation in COSIM*                    |

### Cryptography & Security

| Nama Algoritma                          | C-Sim | C-Synth | Co-Sim | Export IP | Catatan                                  |
|----------------------------------------|:-----:|:-------:|:------:|:---------:|-------------------------------------------|
| `aes`                           | [x]   | [x]     | [x]    | [x]       |                       |
| `blowfish`                             | [x]   | [x]     | [x]    | [ ]       |                                           *IP Export* |
| `blake2s`               | [x]   | [x]     | [x]    | [ ]       | *IP Export*                       |
| `chacha20`                           | [x]   | [x]     | [x]    | [x]       | *Comparison has not been done yet*                      |
| `rsa`                           | [x]   | [x]     | [x]    | []       | *IP Export*  
| `sha256`                           | [x]   | [x]     | [x]    | [x]       |   
| `keccak sha3`                           | [x]   | [x]     | [x]    | []       | *IP Export*  
### Example

| Nama Algoritma     | C-Sim | C-Synth | Co-Sim | Export IP | Catatan           |
|--------------------|:-----:|:-------:|:------:|:---------:|--------------------|
| `vadd_example`     | [x]   | [x]     | [x]    | [x]       |                    |
| `matrix_mult`     | [x]   | [x]     | [x]    | [x]       |                    |
| `median_filter`     | [x]   | [x]     | [x]    | [x]       |                    |

---

## Keterangan Kolom

- **C-Sim (C Simulation):** Simulasi fungsionalitas C/C++ sebelum sintesis.
- **C-Synth (C Synthesis):** Proses sintesis kode C/C++ menjadi RTL menggunakan Vitis HLS.
- **Co-Sim (Co-Simulation):** Simulasi RTL hasil sintesis terhadap testbench C/C++.
- **Export IP:** Ekspor RTL terverifikasi sebagai IP core untuk Vivado/Vitis.
