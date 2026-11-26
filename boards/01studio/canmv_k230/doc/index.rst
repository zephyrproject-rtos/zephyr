.. zephyr:board:: canmv_k230_01studio

Overview
********

The `01Studio CanMV-K230`_ is a development board based on the Canaan Kendryte
K230 RISC-V AIoT SoC. The K230 integrates two heterogeneous 64-bit XuanTie C908
RISC-V cores: an 800 MHz little core (RV64GC) and a 1.6 GHz big core
(RV64GCV with RVV 1.0).

Zephyr currently supports running on the K230 little core (``c908``) in
Machine mode (M-mode).

Hardware
********

The 01Studio CanMV-K230 includes the following hardware features:

* Canaan Kendryte K230 SoC (XuanTie C908 little core at 800 MHz, XuanTie C908V
  big core at 1.6 GHz)
* 1 GB or 2 GB LPDDR4 memory
* 2 MB on-chip SRAM
* Five DesignWare 8250-compatible UART controllers (UART0--UART4)
* Platform-Level Interrupt Controller (PLIC, 208 interrupt sources)
* Core Local Interruptor (CLINT / ACLINT machine timer)
* Onboard 2.4 GHz Wi-Fi module, microSD card slot, MIPI CSI/DSI, HDMI,
  USB Type-C, USB 2.0 host, 3.5 mm audio jack, and 40-pin GPIO header

Supported Features
==================

.. zephyr:board-supported-hw::

Serial Port
===========

Zephyr uses UART0 as the serial console at 115200 baud (8 data bits, no
parity, one stop bit, no flow control). Connect a 3.3 V USB-to-UART adapter
to the board's UART0 debug header (board TX0 to adapter RX, board RX0 to
adapter TX, and GND to GND). Note that the USB Type-C OTG port is used for
power and BootROM USB flashing rather than the hardware UART0 console.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Building
========

Build the :zephyr:code-sample:`hello_world` sample for the K230 little core:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: canmv_k230_01studio/k230/c908
   :goals: build

Running on QEMU
===============

Upstream QEMU introduced ``-machine k230`` in QEMU 11.1 to emulate the K230
SoC. QEMU 11.1 uses a 10 MHz ACLINT timebase (versus 27 MHz on hardware) and
models the UART controllers via generic ``serial_mm_init`` without the
DesignWare ``USR`` register at offset ``0x7c``. Pass the QEMU overlay and
configuration fragment with a dedicated build directory and set
``QEMU_BIN_PATH`` on the initial configure if the Zephyr SDK bundles an older
QEMU version:

.. code-block:: console

   QEMU_BIN_PATH=/path/to/qemu/bin west build -b canmv_k230_01studio/k230/c908 \
     -d build-k230-qemu samples/hello_world -- \
     -DEXTRA_DTC_OVERLAY_FILE="$PWD/boards/01studio/canmv_k230/support/qemu.overlay" \
     -DEXTRA_CONF_FILE="$PWD/boards/01studio/canmv_k230/support/qemu.conf"
   west build -d build-k230-qemu -t run

or invoke ``qemu-system-riscv64`` directly:

.. code-block:: console

   qemu-system-riscv64 -machine k230 -bios build-k230-qemu/zephyr/zephyr.elf -nographic

You should see the following output on the console:

.. code-block:: console

   *** Booting Zephyr OS build ... ***
   Hello World! canmv_k230_01studio/k230/c908

Running on Hardware (U-Boot)
============================

Ensure U-Boot is running in Machine mode (M-mode), because the ``go`` command
transfers control at the current privilege level. Copy ``zephyr.bin`` to the
``K230_APP`` FAT partition (partition 4 on standard K230 SDK SD images; select
the SD card with ``mmc dev 1`` and verify the partition layout with
``mmc part``), then load and jump to ``0x08000000``:

.. code-block:: console

   K230# mmc dev 1
   K230# mmc part
   K230# fatload mmc 1:4 0x08000000 zephyr.bin
   K230# go 0x08000000

References
**********

* `01Studio CanMV-K230`_
* `01Studio CanMV-K230 GitHub Repository`_
* `K230 Technical Reference Manual`_
* `QEMU Kendryte K230 Documentation`_

.. _01Studio CanMV-K230:
   https://wiki.01studio.cc/

.. _01Studio CanMV-K230 GitHub Repository:
   https://github.com/01studio-lab/CanMV-K230

.. _K230 Technical Reference Manual:
   https://github.com/revyos/external-docs/blob/master/K230/en-us/K230_Technical_Reference_Manual_V0.3.1_20241118.pdf

.. _QEMU Kendryte K230 Documentation:
   https://www.qemu.org/docs/master/system/riscv/k230.html
