.. _wasm-micro-runtime-hello-world:

WebAssembly Micro Runtime Hello World
#####################################

Overview
********
The sample project illustrates how to run WebAssembly (WASM) application with
WebAssembly Micro Runtime (WAMR). The WASM application binary is pre-converted into
a byte array defined in test_wasm.h, which is statically linked with the sample.
The WAMR runtime then loads and executes the WASM application binary.

Building and Running
********************
The default config is to build and run wasm app on target qemu-x86-nommu:

.. code-block:: console

    west build -b qemu_x86_nommu . -p always --
    west build -t run

Sample Output
=============

.. code-block:: console

    Hello world!
    buf ptr: 0x00010018
    buf: 1234
    elpase: 60

To build and run the wasm app in other targets:

(1) stm32

.. code-block:: console

    west build -b nucleo_f767zi . -p always -- \
               -DCONF_FILE=target_configs/prj_nucleo767zi.conf
    west flash

(2) esp32

.. code-block:: console

    # Ref to esp-idf document to set up the software environment
    # Suppose esp-idf is installed under ~/esp/esp-idf, and the xtensa-esp32-elf toolchain
    # is installed under ~/.espressif/tools/xtensa-esp32-elf/esp-2021r1-8.4.0/xtensa-esp32-elf,
    # then we can export the environment variables:
    . ~/esp/esp-idf/export.sh
    export ESP_IDF_PATH=~/esp/esp-idf
    export ZEPHYR_TOOLCHAIN_VARIANT=espressif
    export ESPRESSIF_TOOLCHAIN_PATH=~/.espressif/tools/xtensa-esp32-elf/esp-2021r1-8.4.0/xtensa-esp32-elf
    # And build the image:
    west build -b esp32 . -p always -- \
               -DESP_IDF_PATH=$ESP_IDF_PATH \
               -DCONF_FILE=target_configs/prj_esp32.conf
    # And then run the image: (suppose the serial port is /dev/ttyUSB1)
    west flash --esp-device /dev/ttyUSB1

(3) qemu_xtensa

.. code-block:: console

    west build -b qemu_xtensa . -p always -- \
               -DCONF_FILE=target_configs/prj_qemu_xtensa.conf
    west build -t run

(4) qemu_cortex_a53

.. code-block:: console

    west build -b qemu_cortex_a53 . -p always -- \
               -DCONF_FILE=target_configs/prj_qemu_cortex_a53.conf
    west build -t run

(5) qemu_riscv32

.. code-block:: console

    west build -b qemu_riscv32 . -p always --
    west build -t run

(6) qemu_riscv64

.. code-block:: console

    west build -b qemu_riscv64 . -p always --
    west build -t run

(7) qemu_x86_64

.. code-block:: console

    west build -b qemu_x86_64 . -p always --
    west build -t run

Build WASM application
**********************

Install WASI-SDK
================
Download WASI-SDK from https://github.com/CraneStation/wasi-sdk/releases and extract the archive to default path "/opt/wasi-sdk"

Build WASM application with WASI-SDK
====================================

.. code-block:: console

    cd wasm-app
    /opt/wasi-sdk/bin/clang -O3 \
        -z stack-size=4096 -Wl,--initial-memory=65536 \
        -o test.wasm main.c \
        -Wl,--export=main -Wl,--export=__main_argc_argv \
        -Wl,--export=__data_end -Wl,--export=__heap_base \
        -Wl,--strip-all,--no-entry \
        -Wl,--allow-undefined \
        -nostdlib \

Dump WASM binary file into byte array file
==========================================

.. code-block:: console

    cd wasm-app
    xxd -i test.wasm > test_wasm.h

References
**********

  - WAMR sample: https://github.com/bytecodealliance/wasm-micro-runtime/tree/main/product-mini/platforms/zephyr/simple
