.. zephyr:board:: xiao_mg24

Overview
********

Seeed Studio XIAO MG24 is a mini development board based on Silicon Labs' MG24. XIAO MG24 is based
on ARM Cortex-M33 core, 32-bit RISC architecture with a maximum clock speed of 78MHz, supporting DSP
instructions and FPU floating-point operations, possessing powerful computing power, and built-in
AL/ML hardware accelerator MVP, which can efficiently process AI/machine learning algorithms.
Secondly, it has excellent RF performance, with a transmission power of up to+19.5 dBm and a
reception sensitivity as low as -105.4 dBm. It supports multiple IoT and wireless transmission
protocols such as Matter, Thread, Zigbee, Bluetooth LE 5.3,Bluetooth mesh etc.

Hardware
********

- EFR32MG24B220F1536IM48 Mighty Gecko SoC
- CPU core: ARM Cortex®-M33 with FPU
- Flash memory: 1536 kB
- RAM: 256 kB
- Transmit power: up to +20 dBm
- Operation frequency: 2.4 GHz
- Crystals for LFXO (32.768 kHz) and HFXO (39 MHz).
- 3.7v LiPo power and charge support
- User and battery charge LEDs

For more information about the EFR32MG24 SoC and XIAO MG24 board, refer to these
documents:

- `EFR32MG24 Website`_
- `EFR32MG24 Datasheet`_
- `EFR32xG24 Reference Manual`_
- `XIAO MG24 Wiki`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

In the following table, the column **Name** contains Pin names. For example, PA2
means Pin number 2 on PORTA, as used in the board's datasheets and manuals.

+-------+-------------+------------------+
| Name  | Function    | Usage            |
+=======+=============+==================+
| PA7   | GPIO        | LED0             |
+-------+-------------+------------------+
| PA8   | USART0_TX   | UART Console TX  |
+-------+-------------+------------------+
| PA9   | USART0_RX   | UART Console RX  |
+-------+-------------+------------------+

The default configuration can be found in
:zephyr_file:`boards/seeed/xiao_mg24/xiao_mg24_defconfig`

System Clock
============

The EFR32MG24 SoC is configured to use the 39 MHz external oscillator on the
board.

Serial Port
===========

The EFR32MG24 SoC has one USART and two EUSARTs.
USART0 is connected to the board controller and is used for the console.

Programming and Debugging
*************************

The XIAO MG24 contains an SAMD11 with CMSIS-DAP, allowing flashing, debugging, logging, etc. over
the USB port. Doing so requires a version of OpenOCD that includes support for the flash on the MG24
MCU. Until those changes are included in stock OpenOCD, the version bundled with Arduino can be
used, or can be installed from the `OpenOCD Arduino Fork`_. When flashing, debugging, etc. you may
need to include ``--openocd=/usr/local/bin/openocd
--openocd-search=/usr/local/share/openocd/scripts/`` options to the command.

PyOCD can also be used by passing ``-r pyocd`` to the flashing and debugging commands. Add support
for the XIAO MG24 by installing the CMSIS Device Family Pack if not already installed.

.. code-block:: console

   pyocd pack install EFR32MG24

If the pack install command seems to hang indefinitely, ensure that the ``cmsis-pack-manager``
Python package is updated to version ``0.6.0`` or newer.

Flashing
========

Connect the XIAO MG24 board to your host computer using the USB port. A USB CDC ACM serial port
should appear on the host, that can be used to view logs from the flashed application.

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: xiao_mg24
   :goals: flash

Open a serial terminal (minicom, putty, etc.) connecting to the UCB CDC ACM serial port.

Reset the board and you should see the following message in the terminal:

.. code-block:: console

   Hello World! xiao_mg24


.. _XIAO MG24 Wiki:
   https://wiki.seeedstudio.com/xiao_mg24_getting_started/

.. _BRD4187C User Guide:
   https://www.silabs.com/documents/public/user-guides/ug526-brd4187c-user-guide.pdf

.. _EFR32MG24 Website:
   https://www.silabs.com/wireless/zigbee/efr32mg24-series-2-socs

.. _EFR32MG24 Datasheet:
   https://www.silabs.com/documents/public/data-sheets/efr32mg24-datasheet.pdf

.. _EFR32xG24 Reference Manual:
   https://www.silabs.com/documents/public/reference-manuals/brd4187c-rm.pdf

.. _OpenOCD Arduino Fork:
   https://github.com/facchinm/OpenOCD/tree/arduino-0.12.0-rtx5
