.. zephyr:board:: ok3506b_s12

Overview
********

The Forlinx OK3506B-S12 is a development board based on the Rockchip RK3506B
processor. Depending on the board variant, it provides 256 MB or 512 MB DDR3
memory, NAND flash or eMMC storage, a 100M Ethernet interface, USB interfaces,
a TF card slot, and a Raspberry Pi compatible 40-pin GPIO header.

The RK3506B integrates three Cortex-A7 cores running up to 1.5 GHz and a
Cortex-M0 co-processor. Zephyr is ported to run on the Cortex-A7 core. This
board configuration boots standalone Zephyr on Cortex-A7 CPU0, uses UART0 as
the console, and is RAM-loaded by U-Boot into a 1 MiB DDR window at
``0x03e00000``.

The board features:

Rockchip RK3506B SoC
   Three Cortex-A7 cores running up to 1.5 GHz
   Cortex-M0 co-processor
   2D GPU

RAM
   256 MB or 512 MB DDR3

Storage
   256 MB or 512 MB NAND flash
   8 GB eMMC, depending on board variant
   TF card slot

Display
   MIPI-DSI
   RGB888, shared with the RGB, FlexBUS, and DSMC pin functions

Ethernet
   One RMII 100M Ethernet interface

Expansion
   Raspberry Pi compatible 40-pin GPIO header

Audio
   Stereo headphone output
   Headset microphone input

Debug
   Type-C debug connector

Supported Features
==================

.. zephyr:board-supported-hw::

Devices
-------

System Clock
------------

This board configuration uses the ARM architected timer at 24 MHz.

Serial Port
-----------

The default console is UART0 at 115200 baud.

This minimal board port assumes that U-Boot has already configured the UART0
clock and pinmux before starting Zephyr.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build a basic image:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: ok3506b_s12
   :goals: build

Use U-Boot to load ``zephyr.bin`` to RAM and start it:

.. code-block:: console

   loadz 0x03e00000
   printenv filesize
   md.l 0x03e00000 10
   dcache flush 0x03e00000 ${filesize}
   go 0x03e00000

If your U-Boot has working network support, the same flow can be used with
TFTP:

.. code-block:: console

   tftp 0x03e00000 zephyr.bin
   dcache flush 0x03e00000 ${filesize}
   go 0x03e00000

Some vendor U-Boot builds fault on ``icache flush``. It is therefore not part
of the required OK3506B-S12 flow above.

Use the synchronization sample to validate timer, scheduler, and UART output:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: ok3506b_s12
   :goals: build

Expected output is similar to:

.. code-block:: console

   *** Booting Zephyr OS build ... ***
   thread_a: Hello World from cpu 0 on ok3506b_s12!
   thread_b: Hello World from cpu 0 on ok3506b_s12!
