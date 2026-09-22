.. zephyr:board:: ensemble_e8_ak

Overview
********

The Alif Ensemble E8 AI/ML AppKit is a development platform built around the
CSP-packaged AE822FA0E5597BS0 device from the Alif Ensemble E8 series, targeted
at artificial intelligence and machine learning applications.

The Ensemble family of devices contain multiple CPU clusters: up to two
Cortex-M55 based Real Time SubSystems (RTSS), and a Cortex-A32 based
Application Processor SubSystem (APSS).

* **RTSS-HP** (High Performance): Cortex-M55 running at up to 400 MHz
* **RTSS-HE** (High Efficiency): Cortex-M55 running at up to 160 MHz
* **APSS** (Application Processor SubSystem): Cortex-A32 running at up to 800 MHz

Board Identifiers
=================

The following board identifiers are supported:

+----------------------------------------------+-------------+----------+
| Board Identifier                             | SoC Series  | Core     |
+==============================================+=============+==========+
| ``ensemble_e8_ak/ae822fa0e5597bs0/rtss_he``  | E8          | RTSS-HE  |
+----------------------------------------------+-------------+----------+
| ``ensemble_e8_ak/ae822fa0e5597bs0/rtss_hp``  | E8          | RTSS-HP  |
+----------------------------------------------+-------------+----------+
| ``ensemble_e8_ak/ae822fa0e5597bs0/apss``     | E8          | APSS     |
+----------------------------------------------+-------------+----------+

Hardware
********

The Ensemble E8 AI/ML AppKit provides the following hardware features:

- 64 MB Hex SPI HyperRAM PSRAM
- 128 MB Octal SPI (xSPI) Flash
- Micro SD card slot
- High-Speed USB Host interface (Type-A)
- High-Speed USB Device interface (Micro-AB)
- On-board J-Link debugger (``PRG USB``, High-Speed USB Device, Micro-AB)
- 20-pin Arm CoreSight JTAG connector (1.27 mm)
- ICM-42670-P: 3-axis accelerometer and gyroscope
- BMI323: 3-axis accelerometer, gyroscope and temperature sensor
- CH201 ultrasonic range sensor
- I2S digital microphones
- PDM digital microphones
- 3.5 mm headphone (line out) and line in audio jacks
- 5-position joystick
- Reset push-button
- Multicolor LED
- 4.3-inch 480 x 800 color TFT LCD with capacitive touchscreen
- 2-lane MIPI CSI camera connector
- 38.4 MHz crystal oscillator

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Configuring a Console
=====================

Connect a USB cable from your PC to the ``PRG USB`` connector, and use the
serial terminal of your choice (minicom, putty, etc.) with the following
settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

The target UART connected to ``PRG USB`` is selected with the 3-position
slide switch ``SW4``:

+------------+-------------+-------------------------------------+
| ``SW4``    | Target UART | Board target                        |
+============+=============+=====================================+
| Top        | SEUART      | Secure Enclave (not used by Zephyr) |
+------------+-------------+-------------------------------------+
| Center     | UART2       | ``rtss_he``, ``apss``               |
+------------+-------------+-------------------------------------+
| Bottom     | UART4       | ``rtss_hp``                         |
+------------+-------------+-------------------------------------+

Flashing
========

Alif Ensemble E8 AI/ML AppKit can be programmed using the Alif Security
Toolkit (SETOOLS). Refer to the `Alif Software & Tools`_ page for the SETOOLS
download and detailed instructions on using it to program the board.

Alternatively, the board can be flashed with ``west flash`` using the on-board
J-Link debugger. Connect a USB cable from your host computer to the
``PRG USB`` connector. The sample application :zephyr:code-sample:`hello_world`
is used for this example.

Building and flashing for the High Performance core (RTSS-HP):

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ensemble_e8_ak/ae822fa0e5597bs0/rtss_hp
   :goals: build flash

On the serial terminal, you should see the following message:

.. code-block:: console

   Hello World! ensemble_e8_ak/ae822fa0e5597bs0/rtss_hp

Building and flashing for the High Efficiency core (RTSS-HE):

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ensemble_e8_ak/ae822fa0e5597bs0/rtss_he
   :goals: build flash

On the serial terminal, you should see the following message:

.. code-block:: console

   Hello World! ensemble_e8_ak/ae822fa0e5597bs0/rtss_he

Building for the Application Processor (APSS):

.. note::

   On the APSS (Cortex-A32) cluster, Zephyr boots in the Non-Secure state,
   with Arm Trusted Firmware-A (TF-A) acting as the first-stage bootloader
   that initializes the system and hands off to the Zephyr image.
   The TF-A port for the Ensemble APSS cluster is not yet available upstream
   and is currently maintained in Alif's public repository:
   `Alif Trusted Firmware-A`_.

   The resulting image is programmed and launched using the Alif Security
   Toolkit (SETOOLS); the ``west flash`` and ``west debug`` runners are not
   supported for the APSS target.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ensemble_e8_ak/ae822fa0e5597bs0/apss
   :goals: build

Debugging
=========

You can debug an application using ``west debug``:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ensemble_e8_ak/ae822fa0e5597bs0/rtss_hp
   :maybe-skip-config:
   :goals: debug

This will start a GDB server and connect to the target.

References
**********

.. target-notes::

.. _Alif Software & Tools:
   https://alifsemi.com/support/software-tools/view-all/

.. _Alif Trusted Firmware-A:
   https://github.com/alifsemi/trusted-firmware-a_alif
