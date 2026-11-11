.. zephyr:board:: qemu_versatilepb

Overview
********

The ARM Versatile/PB is ARM's platform baseboard for the ARM926EJ-S. This board
runs Zephyr on QEMU's emulation of it, as the reference target of the ARMv5
architecture support: its CPU and peripherals are ARM's own, and the machine
is part of the QEMU the Zephyr SDK ships.

.. list-table::
   :header-rows: 1

   * - Item
     - Detail
   * - CPU
     - ARM926EJ-S (ARMv5TEJ)
   * - RAM
     - 64 MB SDRAM from ``0x00000000``; the image is linked at ``0x00010000``
   * - Console
     - PL011 UART0 at ``0x101F1000``, 115200 8N1
   * - Kernel tick
     - SP804 dual timer at ``0x101E2000``, 1 MHz
   * - Interrupts
     - PL190 VIC at ``0x10140000``, 32 sources

The exception vectors live at the low vector base, address 0, which is SDRAM
on this machine. The bottom 64 kB of SDRAM is left out of the image for them.

Supported Features
******************

.. zephyr:board-supported-hw::

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: qemu_versatilepb
   :goals: run

Press :kbd:`Ctrl-a x` to leave QEMU.
