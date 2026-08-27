.. zephyr:board:: ensemble_e1c_dk

Overview
********

The Alif Ensemble E1C Development Kit is a single-board development
platform for the E1C series device in the Alif Ensemble family.

The E1C series contains a single CPU cluster:

* **RTSS-HE** (High Efficiency): Cortex-M55 running at up to 160 MHz

The board currently supports the AE1C1F4051920PH0 SoC variant, with the
board identifier ``ensemble_e1c_dk/ae1c1f4051920ph0/rtss_he``.

More information about the board can be found at the
`Ensemble E1C DevKit Product Page`_.

Hardware
********

The Ensemble E1C Development Kit provides the following hardware components:

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

The Alif Ensemble E1C Development Kit is programmed using the Alif Security
Toolkit (SETOOLS). Refer to the `Ensemble E1C DevKit Product Page`_ for detailed
instructions on installing and using SETOOLS to program the board.

Building an application for the RTSS-HE core:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ensemble_e1c_dk/ae1c1f4051920ph0/rtss_he
   :goals: build

After building, use SETOOLS to flash the ``build/zephyr/zephyr.bin`` binary to
the board.

After flashing and resetting the board, you should see the following message
on the serial console:

.. code-block:: console

   Hello World! ensemble_e1c_dk/ae1c1f4051920ph0/rtss_he

Debugging
=========

The board supports debugging via the JTAG debug connector using standard debugging tools
such as SEGGER J-Link.

References
**********

.. target-notes::

.. _Ensemble E1C DevKit Product Page:
   https://alifsemi.com/support/kits/ensemble-e1cdevkit/
