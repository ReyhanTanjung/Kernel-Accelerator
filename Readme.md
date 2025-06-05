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
| `conv_2d`                               | [x]   | [x]     | [x]    | [x]       |                                           |
| `fully_connected_layer`                 | [x]   | [x]     | [x]    | [x]       |                                           |
| `gemm`                                  | [x]   | [x]     | [x]    | [x]       |                                           |
| `k_mean_clustering_ongoing_host`        | [x]   | [x]     | [x]    | [x]       | *Ongoing, host code*                      |
| `pca_eigenvalue-accelerator_ongoing_minhost` | [x] | [x]     | [x]    | [x]       | *Ongoing, min host, eigenvalue*           |
| `pooling`                               | [x]   | [x]     | [x]    | [x]       |                                           |
| `softmax`                               | [x]   | [x]     | [x]    | [x]       |                                           |
| `svm_rbf_ongoing_minhost`              | [x]   | [x]     | [x]    | [x]       | *Ongoing, min host, RBF kernel*           |

### Signal Processing & Science

| Nama Algoritma                          | C-Sim | C-Synth | Co-Sim | Export IP | Catatan                                  |
|----------------------------------------|:-----:|:-------:|:------:|:---------:|-------------------------------------------|
| `fft_ongoing_minimpl_tb`              | []   | [ ]     | [ ]    | [ ]       | *Ongoing, min impl, testbench*            |
| `median_filter_ongoing_mincompile`    | [ ]   | [ ]     | [ ]    | [ ]       | *Ongoing, min compile*                    |

### Cryptography & Security

| Nama Algoritma                          | C-Sim | C-Synth | Co-Sim | Export IP | Catatan                                  |
|----------------------------------------|:-----:|:-------:|:------:|:---------:|-------------------------------------------|
| `aes_finish`                           | [x]   | [x]     | [x]    | [x]       | *Marked as 'finish'*                      |
| `blowfish`                             | [ ]   | [ ]     | [ ]    | [ ]       |                                           |
| `rsa_compile_to_xclbin`               | [x]   | [x]     | [x]    | [ ]       | *Compile to xclbin*                       |
| `sha_finish`                           | [x]   | [x]     | [x]    | [x]       | *Marked as 'finish'*                      |

### Example

| Nama Algoritma     | C-Sim | C-Synth | Co-Sim | Export IP | Catatan           |
|--------------------|:-----:|:-------:|:------:|:---------:|--------------------|
| `vadd_example`     | [ ]   | [ ]     | [ ]    | [ ]       |                    |

---

## Keterangan Kolom

- **C-Sim (C Simulation):** Simulasi fungsionalitas C/C++ sebelum sintesis.
- **C-Synth (C Synthesis):** Proses sintesis kode C/C++ menjadi RTL menggunakan Vitis HLS.
- **Co-Sim (Co-Simulation):** Simulasi RTL hasil sintesis terhadap testbench C/C++.
- **Export IP:** Ekspor RTL terverifikasi sebagai IP core untuk Vivado/Vitis.
