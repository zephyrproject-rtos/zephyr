.. zephyr:board:: xg26_dk2608a

Overview
********

The `EFR32xG26-DK2608A Dev Kit`_ is a compact, feature-packed development platform.
The development platform includes a broad range of sensors, various peripheral devices,
and a Qwiic connector, allowing you to explore the endless world of Sparkfun expansion hardware. It provides the fastest path to develop and prototype wireless IoT products.

.. _EFR32xG26-DK2608A Dev Kit:
    https://www.silabs.com/development-tools/wireless/efr32xg26-dev-kit

Hardware
********

- EFR32MG26B510F3200IM68 SoC
- CPU core: 32-bit ARM® Cortex®-M33 with FPU
- Flash memory: 3200 kB
- RAM: 512 kB
- Transmit power: up to +10 dBm
- Operation frequency: 2.4 GHz
- Crystals for LFXO (32.768 kHz) and HFXO (39 MHz) on the board.
- Kit features:

  - Silicon Labs Si7021 relative humidity and temperature sensor
  - Silicon Labs Si7210 hall effect sensor
  - TDK InvenSense ICM-40627 6-axis inertial sensor
  - Two ICS-43434 MEMS microphones
  - Bosch Sensortec BMP384 barometric pressure sensor
  - Ambient light sensor (VEML6035)
  - Macronix ultra-low-power 64 Mbit SPI flash (MX25R6435F)
  - RGB LED and two push buttons
  - U.FL connector and precise external voltage reference for ADC measurements
  - Power enable signals and isolation switches for ultra-low-power operation
  - On-board SEGGER J-Link debugger (USB virtual COM port and Packet Trace Interface)
  - Mini Simplicity connector for access to energy profiling and advanced wireless network debugging
  - Breakout pads for GPIO access and connection to external hardware
  - Qwiic connector for connecting external hardware from the Qwiic Connect System
  - Reset button
  - Automatic switchover between USB and battery power
  - CR2032 coin cell holder and external battery connector

For more information about the EFR32MG26 SoC and DK2608A Dev Kit, refer to these documents:

- `EFR32MG26 Datasheet`_
- `EFR32xG26 Reference Manual`_
- `xG26-DK2608A User Guide`_

.. _EFR32MG26 Datasheet:
   https://www.silabs.com/documents/public/data-sheets/efr32mg26-datasheet.pdf

.. _EFR32xG26 Reference Manual:
   https://www.silabs.com/documents/public/reference-manuals/efr32xg26-rm.pdf

.. _xG26-DK2608A User Guide:
   https://www.silabs.com/documents/public/user-guides/ug584-brd2608a-user-guide.pdf

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The EFR32MG26 SoC is configured to use the HFRCODPLL oscillator at 78 MHz as the system clock,
locked to the 39 MHz crystal oscillator on the board.

Serial Port
===========

The EFR32MG26 SoC has 3 USARTs and 4 EUSARTs.
USART0 is connected to the board controller and is used for the console.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

Connect the DK2608A Dev Kit to your host computer using the USB port.

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: xg26_dk2608a
   :goals: flash

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you should see the following message in the terminal:

.. code-block:: console

   Hello World! xg26_dk2608a

Bluetooth
=========

To use Bluetooth functionality, run the command below to retrieve necessary binary
blobs from the Silicon Labs HAL repository.

.. code-block:: console

   west blobs fetch hal_silabs

Then build the Zephyr kernel and a Bluetooth sample with the following
command. The :zephyr:code-sample:`bluetooth_observer` sample application is used in
this example.

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/observer
   :board: xg26_dk2608a
   :goals: build
