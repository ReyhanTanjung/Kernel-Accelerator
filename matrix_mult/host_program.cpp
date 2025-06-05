#include <iostream>
#include <cstring>

// XRT includes
#include "xrt/xrt_bo.h"
#include <experimental/xrt_xclbin.h>
#include "xrt/xrt_device.h"
#include "xrt/xrt_kernel.h"

#define DATA_SIZE 4096

int main(int argc, char** argv) {

    std::cout << "argc = " << argc << std::endl;
    for(int i = 0; i < argc; i++){
        std::cout << "argv[" << i << "] = " << argv[i] << std::endl;
    }

    // Pastikan argumen yang benar diberikan
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <xclbin>" << std::endl;
        return EXIT_FAILURE;
    }

    // Baca xclbin file dari argumen
    std::string binaryFile = argv[1];
    int device_index = 0;

    std::cout << "Open the device " << device_index << std::endl;
    auto device = xrt::device(device_index);
    std::cout << "Load the xclbin " << binaryFile << std::endl;
    auto uuid = device.load_xclbin(binaryFile);

    size_t vector_size_bytes = sizeof(int) * DATA_SIZE;

    auto krnl = xrt::kernel(device, uuid, "matrix_mult", xrt::kernel::cu_access_mode::exclusive);

    std::cout << "Allocate Buffer in Global Memory\n";
    auto boIn1 = xrt::bo(device, vector_size_bytes, krnl.group_id(0)); // Match kernel arguments
    auto boIn2 = xrt::bo(device, vector_size_bytes, krnl.group_id(1));
    auto boOut = xrt::bo(device, vector_size_bytes, krnl.group_id(2));

    // Map buffer ke host memory
    auto bo0_map = boIn1.map<int*>();
    auto bo1_map = boIn2.map<int*>();
    auto bo2_map = boOut.map<int*>();

    // Inisialisasi buffer input dan reference
    int bufReference[DATA_SIZE];
    for (int i = 0; i < DATA_SIZE; ++i) {
        bo0_map[i] = i;
        bo1_map[i] = i;
        bufReference[i] = bo0_map[i] * bo1_map[i]; // Asumsi operasi matrix multiplication sederhana
    }

    // Sinkronisasi buffer input ke device
    std::cout << "Synchronizing input buffer data to device global memory\n";
    boIn1.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    boIn2.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    std::cout << "Execution of the kernel\n";
    auto run = krnl(boIn1, boIn2, boOut, DATA_SIZE);
    run.wait();

    // Ambil output dari device
    std::cout << "Get the output data from the device\n";
    boOut.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

    // Validasi hasil
    if (std::memcmp(bo2_map, bufReference, vector_size_bytes)) {
        std::cerr << "ERROR: Value read back does not match reference\n";
        return EXIT_FAILURE;
    }

    std::cout << "TEST PASSED\n";
    return EXIT_SUCCESS;
}
