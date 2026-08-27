.. zephyr:board:: balletto_b1_dk

Overview
********

The Alif Balletto B1 Development Kit is a single-board development
platform for the B1 series device in the Alif Balletto family.

The B1 series contains a single CPU cluster:

* **RTSS-HE** (High Efficiency): Cortex-M55 running at up to 160 MHz

The board currently supports the AB1C1F4M51820PH0 SoC variant, with the
board identifier ``balletto_b1_dk/ab1c1f4m51820ph0``.

More information about the board can be found at the
`Balletto B1 DevKit Product Page`_.

Hardware
********

The Balletto B1 Development Kit provides the following hardware components:

- 64 MB Octal SPI HyperRAM PSRAM
- 128 MB Octal SPI Flash
- Micro SD card slot
- High-Speed USB Device interface
- High-Speed USB Host interface
- USB-to-UART bridge for programming and debug
- CAN interface (4-pin header)
- ICM-42670-P: 3-axis accelerometer, gyroscope
- BMI323: 3-axis accelerometer, gyroscope, and temperature sensor
- I2S audio codec with headphone and line-in jacks
- 2x PDM digital microphones
- 5-position joystick
- Reset push-button
- Multicolor LEDs
- GPIO headers for I/O expansion
- JTAG debug connector
- BLE 5.3 radio subsystem

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

Configuring a Console
=====================

Connect a USB cable from your PC to the USB-to-UART bridge connector, and use
the serial terminal of your choice (minicom, putty, etc.) with the following
settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

The default UART console is:

- **RTSS-HE**: UART2

Flashing
========

The Alif Balletto B1 Development Kit is programmed using the Alif Security
Toolkit (SETOOLS). Refer to the `Balletto B1 DevKit Product Page`_ for detailed
instructions on installing and using SETOOLS to program the board.

Building an application for the RTSS-HE core:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: balletto_b1_dk/ab1c1f4m51820ph0
   :goals: build

After building, use SETOOLS to flash the ``build/zephyr/zephyr.bin`` binary to
the board.

After flashing and resetting the board, you should see the following message
on the serial console:

.. code-block:: console

   Hello World! balletto_b1_dk/ab1c1f4m51820ph0

Debugging
=========

The board supports debugging via the JTAG debug connector using standard debugging tools
such as SEGGER J-Link.

References
**********

.. target-notes::

.. _Balletto B1 DevKit Product Page:
   https://alifsemi.com/support/kits/balletto-b1devkit/
