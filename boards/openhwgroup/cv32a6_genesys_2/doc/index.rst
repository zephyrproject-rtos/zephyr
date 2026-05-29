.. zephyr:board:: cv32a6_genesys_2

Overview
********

The Digilent Genesys 2 board features a Xilinx Kintex-7 FPGA which can run various softcore CPUs.
In this configuration, the Genesys 2 is configured with a 32-bit version of the CVA6 RISC-V CPU.
The SoC is configured with a memory controller interfacing with the Genesys' DRAM, PLIC and CLINT
interrupt controllers, a UART device interfacing with the Genesys' USB UART, a RISC-V compatible
debug module that interfaces with the Genesys' FTDI (USB JTAG) chip, a Xilinx SPI interface
interfacing with the Genesys' SD card slot and a Xilinx GPIO interfacing with the Genesys' LEDs
and switches.
The complete hardware sources (see first reference) in conjunction with
instructions for compiling and loading the configuration onto the Genesys 2 are available.

See the following references for more information:

- `CVA6 documentation`_
- `Genesys 2 Reference Manual`_
- `Genesys 2 Schematic`_

Hardware
********

- CVA6 CPU with RV32imac instruction sets with PLIC, CLINT interrupt controllers.
- 1 GB DDR3 DRAM
- 10/100/1000 Ethernet with copper interface, lowRISC Ethernet MAC
- ns16550a-compatible USB UART, 115200 baud
- RISCV debug module, connected via on-board FTDI (USB JTAG)
- Xilinx SPI controller, connected to microSD slot
- Xilinx GPIO, connected to 7 switches and LEDs

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

Loading the FPGA configuration
==============================

You need to build a bitstream with Xilinx Vivado and load it into the FPGA
before you can load zephyr onto the board.
Please refer to the CVA6 documentation for the required steps.
This configuration is compatible with the following build targets:
cv32a6_imac_sv0, cv32a6_imac_sv32, cv32a6_imafc_sv32, cv32a6_ima_sv32_fpga.

Flashing
========
west flash is supported via the openocd runner.
Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: cv32a6_genesys_2
   :goals: build flash

Debugging
=========

west debug, attach and debugserver commands are supported via the openocd runner.
Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: cv32a6_genesys_2
   :goals: build debug

References
**********

.. _CVA6 documentation:
   https://github.com/openhwgroup/cva6

.. _Genesys 2 Reference Manual:
   https://digilent.com/reference/programmable-logic/genesys-2/reference-manual

.. _Genesys 2 Schematic:
   https://digilent.com/reference/_media/reference/programmable-logic/genesys-2/genesys-2_sch.pdf
