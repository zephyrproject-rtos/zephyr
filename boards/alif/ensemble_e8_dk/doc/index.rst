.. zephyr:board:: ensemble_e8_dk

Overview
********

The Alif Ensemble E8 Development Kit is a single-board development
platform that can be configured to operate with the E4, E6, or E8 series devices
in the Alif Ensemble family.

The Ensemble family of devices can contain multiple CPU clusters: up to two
Cortex-M55 based Real Time SubSystems (RTSS), and on some devices a Cortex-A32
based Application Processor SubSystem (APSS).

* **RTSS-HP** (High Performance): Cortex-M55 running at up to 400 MHz
* **RTSS-HE** (High Efficiency): Cortex-M55 running at up to 160 MHz
* **APSS** (Application Processor SubSystem): Cortex-A32 running at up to 800 MHz

Board Identifiers
=================

The following board identifiers are supported:

+----------------------------------------------+-------------+----------+
| Board Identifier                             | SoC Series  | Core     |
+==============================================+=============+==========+
| ``ensemble_e8_dk/ae402fa0e5597le0/rtss_he``  | E4          | RTSS-HE  |
+----------------------------------------------+-------------+----------+
| ``ensemble_e8_dk/ae402fa0e5597le0/rtss_hp``  | E4          | RTSS-HP  |
+----------------------------------------------+-------------+----------+
| ``ensemble_e8_dk/ae612fa0e5597ls0/rtss_he``  | E6          | RTSS-HE  |
+----------------------------------------------+-------------+----------+
| ``ensemble_e8_dk/ae612fa0e5597ls0/rtss_hp``  | E6          | RTSS-HP  |
+----------------------------------------------+-------------+----------+
| ``ensemble_e8_dk/ae822fa0e5597ls0/rtss_he``  | E8          | RTSS-HE  |
+----------------------------------------------+-------------+----------+
| ``ensemble_e8_dk/ae822fa0e5597ls0/rtss_hp``  | E8          | RTSS-HP  |
+----------------------------------------------+-------------+----------+
| ``ensemble_e8_dk/ae822fa0e5597ls0/apss``     | E8          | APSS     |
+----------------------------------------------+-------------+----------+

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

The on-board USB-to-UART bridge on the PRG USB connector is shared between the
Secure Enclave UART (SE-UART) and the UART2 and UART4 user serial ports,
selected by switch SW4. SW4 selects the SE-UART in its default position; move
it to the UART2 or UART4 position to reach the Zephyr console.

Connect a USB cable from your PC to the PRG USB connector, and use the serial
terminal of your choice (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

The default UART consoles are:

- **RTSS-HP**: UART4
- **RTSS-HE**: UART2
- **APSS**: UART2

Flashing
========

Alif Ensemble E8 Development Kit is programmed with ``west flash`` through the
Alif Security Toolkit (SETOOLS), using the :ref:`alif_flash runner
<runner_alif_flash>`. Set up SETOOLS as described there, move SW4 to the SE-UART
position and connect the PRG USB connector to your host computer. Pass the
SE-UART device with ``--port`` if it differs from the SETOOLS configuration.
Move SW4 back to the console position after flashing.

Alternatively, the board can be flashed using a SEGGER J-Link debug probe with
``west flash -r jlink``. Connect the board to your host computer using the
J-Link debug interface.

The sample application :zephyr:code-sample:`hello_world` is used for these
examples.

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

Building and flashing for the Application Processor (APSS):

.. note::

   On the APSS (Cortex-A32) cluster, Zephyr boots in the Non-Secure state,
   with Arm Trusted Firmware-A (TF-A) acting as the first-stage bootloader
   that initializes the system and hands off to the Zephyr image.
   The TF-A port for the Ensemble APSS cluster is not yet available upstream
   and is currently maintained in Alif's public repository:
   `Alif Trusted Firmware-A`_.

   The ``alif_flash`` runner programs the TF-A BL32 image together with the
   Zephyr image; pass the BL32 image with ``--bl32-bin``.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ensemble_e8_dk/ae822fa0e5597ls0/apss
   :goals: build

.. code-block:: console

   west flash --bl32-bin=<path to bl32.bin>

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

.. _Alif Trusted Firmware-A:
   https://github.com/alifsemi/trusted-firmware-a_alif
