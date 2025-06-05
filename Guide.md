# Platform
```
xilinx_u250_gen3x16_xdma_4_1_202210_1
```

# Usage
## 1. Buat kernel .xo dari HLS (--target sw_emu or hw_emu)
```
v++ -c -k vadd --target sw_emu --platform xilinx_u250_gen3x16_xdma_4_1_202210_1 --save-temps -I. -o vadd.xo vadd.cpp
```

## 2. Buat binary .xclbin (--target sw_emu or hw_emu)
```
v++ -l --target sw_emu --platform xilinx_u250_gen3x16_xdma_4_1_202210_1 -o vadd.xclbin vadd.xo
```


## 3. Buat emconfig.json
```
emconfigutil --platform xilinx_u250_gen3x16_xdma_4_1_202210_1
```

## 4. COMPILE HOST Program
```
XILINX_XRT        : /opt/xilinx/xrt
PATH              : /opt/xilinx/xrt/bin:/home/vlsi-elka/.local/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin:/snap/bin
LD_LIBRARY_PATH   : /opt/xilinx/xrt/lib
PYTHONPATH        : /opt/xilinx/xrt/python
```
```
g++ -std=c++17 host.cpp -o host -I$XILINX_XRT/include -L$XILINX_XRT/lib -lxrt_coreutil -pthread
```

## 5. Jalankan Emulation

### Software Emulation
```
export XCL_EMULATION_MODE=sw_emu
./host
```
### Hardware Emulation
```
export XCL_EMULATION_MODE=hw_emu
./host
```

## 6. Compile ke target HW
```
v++ -c --target hw --platform xilinx_u250_gen3x16_xdma_4_1_202210_1 -k vadd -I . -o vadd_hw.xo vadd.cpp
```

## 7. Convert .xo -> .xclbin
```
v++ -l --target hw --platform xilinx_u250_gen3x16_xdma_4_1_202210_1 -o vadd_hw.xclbin vadd_hw.xo
```

## 8. Jalankan di FPGA
```
unset XCL_EMULATION_MODE
./host vadd_hw.xclbin
```