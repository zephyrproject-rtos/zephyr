.. _wasm_micro_runtime

WebAssembly Micro Runtime
#########################

Overview
********
The sample project illustrates how to run WebAssembly application with
WebAssembly Micro Runtime (WAMR). More samples can be found on:
  https://github.com/bytecodealliance/wasm-micro-runtime/tree/master/samples

Building and Running
********************
The default config is to build and run wasm app on target qemu-x86-nommu:
  west build -b qemu_x86_nommu . -p always -- -DWAMR_BUILD_TARGET=X86_32
  west build -t run

Sample Output
=============
  Hello world!
  buf ptr: 0xffffe158
  buf: 1234
  elpase: 20

To build and run the wasm app in other targets:
(1) stm32
  west build -b nucleo_f767zi . -p always -- \
             -DCONF_FILE=target_configs/prj_nucleo767zi.conf \
             -DWAMR_BUILD_TARGET=THUMBV7
  west flash

(2) esp32
  # Suppose that the environment variable ESP_IDF_PATH has been set
  west build -b esp32 . -p always -- \
             -DESP_IDF_PATH=$ESP_IDF_PATH \
             -DCONF_FILE=target_configs/prj_esp32.conf \
             -DWAMR_BUILD_TARGET=XTENSA
  # Suppose the serial port is /dev/ttyUSB1
  west flash --esp-device /dev/ttyUSB1

(3) qemu_xtensa
  west build -b qemu_xtensa . -p always -- \
             -DCONF_FILE=target_configs/prj_qemu_xtensa.conf \
             -DWAMR_BUILD_TARGET=XTENSA
  west build -t run

(4) qemu_cortex_a53
  west build -b qemu_cortex_a53 . -p always -- \
             -DCONF_FILE=target_configs/prj_qemu_cortex_a53.conf \
             -DWAMR_BUILD_TARGET=AARCH64
  west build -t run

