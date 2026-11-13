.. zephyr:board:: mr_vmu_tropic

Overview
********

The MR-VMU-TROPIC is a vehicle management unit built around the NXP
i.MX RT1064 crossover MCU. The i.MX RT1064 runs on the Arm Cortex-M7 core
at up to 600 MHz and integrates 1 MB of on-chip SRAM together with a 4 MB
on-package QSPI NOR flash. The board pairs the MCU with a flight sensor
set, 100BASE-T1 automotive Ethernet, redundant serial links and RC input,
making it suitable as an autopilot management unit.

Hardware
********

- MIMXRT1064DVL6A MCU (600 MHz Arm Cortex-M7, 1024 KB on-chip SRAM,
  4096 KB on-package QSPI flash)

- Memory

  - FlexRAM configured as 256 KB ITCM and 256 KB DTCM
  - microSD socket on USDHC1

- Ethernet

  - 100BASE-T1 single-pair Ethernet through a TJA1103 PHY
  - ENET PTP hardware clock for an 802.1AS timebase

- USB

  - USB 2.0 device connector

- CAN

  - FlexCAN3 with an on-board transceiver

- Serial and RC

  - Five general purpose LPUART ports, one of them single-wire capable
  - CRSF RC input for compatible receivers

- Sensors

  - ICM-45686 6-axis IMU
  - BMI088 6-axis IMU
  - BMM350 3-axis magnetometer
  - IST8310 3-axis magnetometer
  - BMP390 barometer

- Indicators

  - RGB status LED driven by three FlexPWM channels
  - NCP5623 I2C RGB LED controller on the optional GNSS module, not fitted
    by default
  - Buzzer on the optional GNSS module, not fitted by default

- Inputs

  - Arming button on the optional GNSS module, not fitted by default

For more information about the i.MX RT1064 SoC, see these references:

- `i.MX RT1064 Website`_
- `i.MX RT1064 Datasheet`_
- `i.MX RT1064 Reference Manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The board exposes its interfaces on JST-GH connectors for serial, I2C and
CAN, an RJ45 style connector for the 100BASE-T1 link, a microSD socket and
a USB device port. The pin multiplexing for every peripheral is defined in
the board :file:`mr_vmu_tropic-pinctrl.dtsi` and enabled from the board
devicetree.

Serial Port
===========

The i.MX RT1064 SoC has eight LPUART interfaces. LPUART6 is used for the
Zephyr console. The remaining ports are wired to the telemetry, GNSS and RC
connectors.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Configuring a Console
=====================

Use the console LPUART with the following settings for your serial terminal
of choice (screen, minicom, putty and similar):

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flashing
========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mr_vmu_tropic
   :goals: flash

You should see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS ***
   Hello World! mr_vmu_tropic/mimxrt1064

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mr_vmu_tropic
   :goals: debug

Open a serial terminal, step through the application in your debugger and you
should see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS ***
   Hello World! mr_vmu_tropic/mimxrt1064

References
**********

.. _i.MX RT1064 Website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-based-processors-and-mcus/i.mx-applications-processors/i.mx-rt-series/i.mx-rt1064-crossover-processor-with-arm-cortex-m7-core:i.MX-RT1064

.. _i.MX RT1064 Datasheet:
   https://www.nxp.com/docs/en/data-sheet/IMXRT1064CEC.pdf

.. _i.MX RT1064 Reference Manual:
   https://www.nxp.com/webapp/Download?colCode=IMXRT1064RM
