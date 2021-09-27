.. _wasm-micro-runtime-wasm_hello_world:

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

This example can be built in the standard way,
or example with the :zephyr:board:`qemu_x86_nommu` target:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/wasm_micro_runtime/wasm_hello_world/
   :board: qemu_x86_nommu
   :goals: build

For QEMU target, we can launch with following command.

.. code-block:: console

    west build -t run

Sample Output
=============

.. code-block:: console

    Hello world!
    buf ptr: 0x00010018
    buf: 1234
    elpase: 60

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
