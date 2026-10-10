.. zephyr:board:: pynq_z1

Overview
********

The `Digilent PYNQ-Z1`_ is built around the Xilinx Zynq-7000 XC7Z020, a dual-core ARM Cortex-A9
processing system (PS) with 7-series programmable logic (PL) on one die.

Hardware
********

- Xilinx Zynq-7000 XC7Z020-1CLG400C, dual-core ARM Cortex-A9 at 650 MHz
- 512 MiB DDR3, 16-bit bus
- 16 MiB quad SPI flash on QSPI0, ``MIO1``..``MIO6``, feedback clock on ``MIO8``
- microSD slot on SDIO0, ``MIO40``..``MIO45``
- USB-UART bridge on UART0, ``MIO14``/``MIO15``
- Gigabit Ethernet on GEM0, RGMII on ``MIO16``..``MIO27``, MDIO on ``MIO52``/``MIO53``, Realtek
  RTL8211E PHY at MDIO address 1
- USB 2.0 OTG on USB0, ``MIO28``..``MIO39``
- Two Pmod ports, an Arduino/chipKIT shield connector, HDMI in and out, audio, LEDs, push buttons
  and slide switches, all on the programmable logic

Supported Features
==================

.. zephyr:board-supported-hw::

The RGMII and MDIO pin multiplexing and the GEM0 reference clock are set by the processing system
initialization. GEM1 is not routed on this board. The LEDs, buttons and switches are on the PL
and need a bitstream; no ``led0`` or ``sw0`` alias is defined.

Programming and Debugging
*************************

A Zynq-7000 leaves its boot ROM with the PLLs bypassed, the pins unrouted and the DDR controller in
reset. A first stage boot loader brings the processing system up and loads the image; over JTAG,
``west flash`` does both. ``support/ps7_init.cfg`` resets the processing system to its boot ROM
state and runs the register list the Xilinx tools generate for the board, through the memory
access port of the DAP. Regenerate the list from the hardware design if the clocks, the pins or the
DDR configuration change.

The image is linked into DDR from 1 MiB up; the boot ROM maps the on-chip memory over the first
megabyte.

Flashing
========

Set ``JP4`` to ``JTAG``, open a serial terminal on the USB-UART bridge at 115200 baud and power the
board. Build and run the :zephyr:code-sample:`hello_world` application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: pynq_z1
   :goals: flash

.. code-block:: console

   *** Booting Zephyr OS vx.xx.x-xxx-gxxxxxxxxxxxx ***
   Hello World! pynq_z1/xc7z020

Booting from the SD card
========================

A first stage boot loader on the card brings the processing system up and starts ``zephyr.bin``
at ``0x100000``: the Xilinx FSBL built from the board's hardware design, or the Das U-Boot SPL
with a device tree for the board, which upstream U-Boot does not ship. Set ``JP4`` to ``SD``.
From the U-Boot prompt, with the caches off because Zephyr brings up the MMU and the caches
itself:

.. code-block:: console

   Zynq> fatload mmc 0 0x100000 zephyr.bin
   Zynq> dcache off
   Zynq> icache off
   Zynq> go 0x100000
   ## Starting application at 0x00100000 ...
   *** Booting Zephyr OS vx.xx.x-xxx-gxxxxxxxxxxxx ***
   Hello World! pynq_z1/xc7z020

Debugging
=========

With the board set up as for flashing:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: pynq_z1
   :goals: debug

.. _Digilent PYNQ-Z1:
   https://digilent.com/reference/programmable-logic/pynq-z1/start
