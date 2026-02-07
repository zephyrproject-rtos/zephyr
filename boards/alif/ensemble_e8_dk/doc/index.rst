.. zephyr:board:: ensemble_e8_dk

Overview
********

The Alif Ensemble E8 Development Kit is a single-board development
platform that can be configured to operate with the E4, E6, or E8 series devices
in the Alif Ensemble family.

The Ensemble family of devices can contain up to two Cortex-M55 cores available
as two separate CPU clusters.

* **RTSS-HP** (High Performance): Cortex-M55 running at up to 400 MHz
* **RTSS-HE** (High Efficiency): Cortex-M55 running at up to 160 MHz

To build for the above cores, use the following board targets (for Ensemble E8 SOC AE822FA0E5597LS0):

* ``ensemble_e8_dk/ae822fa0e5597ls0/rtss_hp``
* ``ensemble_e8_dk/ae822fa0e5597ls0/rtss_he``

More information about the board can be found at the
`Ensemble E8 DevKit Product Page`_.

Hardware
********

The Ensemble E8 Development Kit provides different hardware components including:

- 64 MB Octal SPI PSRAM
- 128 MB Octal SPI Flash
- Micro SD card slot
- High-Speed USB Device interface
- High-Speed USB Host interface
- USB-to-UART bridge for programming and debug
- 10/100 Mbit/s Ethernet
- ICM-42670: Accelerometer, gyroscope and temperature sensor
- BMI323: Accelerometer, gyroscope, and temperature sensor
- WM8904 I2S Codec
- I2S/PDM digital microphones
- 5-position joystick
- Reset push-button
- Multicolor LEDs
- GPIO headers for I/O expansion
- CSI/parallel camera sensor interfaces
- DSI/parallel display panel interfaces
- click module interface
- JTAG debug connector

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Configuring a Console
=====================

Connect a USB cable from your PC to the USB-to-UART bridge connector, and use
the serial terminal of your choice (minicom, putty, etc.) with the following
settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

The default UART consoles are:

- **RTSS-HP**: UART4
- **RTSS-HE**: UART2

Flashing
========

Alif Ensemble E8 Development Kit can be programmed using the Alif Security
Toolkit (SETOOLS). Refer to the `Ensemble E8 DevKit Product Page`_ for detailed
instructions on using SETOOLS to program the board.

Alternatively, the board can be flashed using a SEGGER J-Link debug probe with
``west flash``. Connect the board to your host computer using the J-Link debug
interface. The sample application :zephyr:code-sample:`hello_world` is used for
this example.

Building and flashing for the High Performance core (RTSS-HP):

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ensemble_e8_dk/ae822fa0e5597ls0/rtss_hp
   :goals: build flash

On the serial terminal, you should see the following message:

.. code-block:: console

   Hello World! ensemble_e8_dk/ae822fa0e5597ls0/rtss_hp

Building and flashing for the High Efficiency core (RTSS-HE):

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ensemble_e8_dk/ae822fa0e5597ls0/rtss_he
   :goals: build flash

On the serial terminal, you should see the following message:

.. code-block:: console

   Hello World! ensemble_e8_dk/ae822fa0e5597ls0/rtss_he

Debugging
=========

You can debug an application using ``west debug``:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ensemble_e8_dk/ae822fa0e5597ls0/rtss_hp
   :maybe-skip-config:
   :goals: debug

This will start a GDB server and connect to the target.

References
**********

.. target-notes::

.. _Ensemble E8 DevKit Product Page:
   https://alifsemi.com/support/kits/ensemble-e8devkit/
