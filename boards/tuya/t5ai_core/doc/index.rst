.. zephyr:board:: t5ai_core

Overview
********

T5AI Core is a Tuya board based on the Beken BK7258 SoC. This board target
runs Zephyr on the BK7258 CPU0 cluster.

Hardware
********

- Beken BK7258 SoC
- Arm Cortex-M33 CPU
- 128 KiB SRAM available to Zephyr
- 8 MiB physical flash; Zephyr currently uses a 3792 KiB logical XIP region
- UART1 console on GPIO0 TX and GPIO1 RX

Supported Features
==================

.. zephyr:board-supported-hw::

Serial Console
==============

UART1 is used as the default console at 115200 baud, 8 data bits, no parity,
and 1 stop bit.

Programming and Debugging
*************************

Build the Hello World sample with:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: t5ai_core/bk7258/cpu0
   :goals: build

The build produces :file:`build/zephyr/zephyr.bk7258.bin` for programming.
The image is linked at the BK7258 logical flash address ``0x02000000``. The
build writes the BootROM boot marker ``BK7236`` at image offset ``0x100``, pads
each 32-byte logical block with ``0xff`` when necessary and appends its
big-endian CRC-16/CMS value. No first-stage bootloader is required.

The BootROM does not start the image after a cold reset without the boot marker.
The marker occupies the vectors of IRQ 48 and 49, so these interrupts are not
available. Program :file:`zephyr.bk7258.bin` rather than loading
:file:`zephyr.elf` with a debug probe, as the ELF file does not contain the
marker.

The publicly available `BK7231 GUI Flash Tool`_ supports the BK7258 UART
download protocol. To program the board:

#. Connect a 3.3 V USB-to-UART adapter to the board's BK7258 download UART and
   connect the grounds.
#. Select ``BK7258 (T5)`` and the adapter's serial port in the tool.
#. Back up the existing flash before writing the board.
#. Select :file:`build/zephyr/zephyr.bk7258.bin` and write it at physical offset
   ``0x0``. Do not install a separate first-stage bootloader.
#. Reset or power-cycle the board when the tool waits for the BootROM download
   handshake.

This board does not currently provide a debugging runner.

.. _BK7231 GUI Flash Tool:
   https://github.com/openshwprojects/BK7231GUIFlashTool
