.. zephyr:board:: kit_psc3m8_evk

Overview
********

The PSOC™ Control C3M8 Evaluation Kit (`KIT_PSC3M8_EVK`_) is an evaluation board for the
PSC3M8 microcontroller from Infineon's CAT1B family. The PSC3M8GFS3PAHQ1 variant on this board features:

* **Main Core:** Arm® Cortex®-M33 core running at up to 180 MHz with FPU, DSP, and TrustZone-M security.
  The board also features 512 KB internal flash, 128 KB system SRAM, up to seven SCBs, and two CAN FD channels
* **PPCA Core:** Subsystem Programmable Power Control Accelerator(PPCA) containing two Arm® Cortex®-M33 cores
  running at up to 200 MHz with FPU and DSP, 112 KB of dedicated SRAM, and feature-rich analog and digital peripherals
  designed for complex power conversion and motor control applications. Key features include 12-bit, 25 Msps ADCs, comparators
  with differential slope generators, hardware filters, custom logic blocks, DACs,
  and advanced timers with high-resolution capabilities.

Hardware
********

- **SoC:** PSC3M8GFS3PAHQ1 (PG-E-LQFP-100)
- **Main CPU:** Arm® Cortex®-M33 at up to 180 MHz (with FPU, DSP)
- **PPCA Cores:** Two Arm® Cortex® M33 running at up to 200 MHz (with FPU, DSP and 112 KB SRAM)
- **Flash:** 512 KB internal flash
- **SRAM:** 128 KB
- **Connectivity:** CAN FD, SCB (UART/SPI/I2C)
- **Security:** TrustZone-M
- **Debug:** Onboard programmer/debugger (KitProg3)
- **User I/O:** six user LEDs, two user buttons, three potentiometers

Kit Contents
============

The `KIT_PSC3M8_EVK`_ website lists the following kit contents:

- PSOC™ Control C3M8 Evaluation Kit board
- USB Type-A to Type-C cable
- Quick Start Guide

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

LEDs
----

+---------+-------------------+
| Name    | GPIO Pin          |
+=========+===================+
| LED0    | P1.5 (active low) |
+---------+-------------------+
| LED1    | P4.1 (active low) |
+---------+-------------------+
| LED2    | P1.7 (active low) |
+---------+-------------------+
| LED3    | P1.4 (active low) |
+---------+-------------------+
| LED4    | P9.1 (active low) |
+---------+-------------------+
| LED5    | P4.0 (active low) |
+---------+-------------------+

Push Buttons
------------

+---------+--------------------+
| Name    | GPIO Pin           |
+=========+====================+
| SW3     | P5.2 (active low)  |
+---------+--------------------+
| SW4     | P7.0 (active low)  |
+---------+--------------------+

Default Zephyr Peripheral Mapping
----------------------------------

+-----------+-----------------+----------------------------+
| Pin       | Function        | Usage                      |
+===========+=================+============================+
| P3.3      | SCB2 UART TX    | Console TX                 |
+-----------+-----------------+----------------------------+
| P3.4      | SCB2 UART RX    | Console RX                 |
+-----------+-----------------+----------------------------+
| P7.4      | CANFD0 CH1 TX   | CAN FD TX                  |
+-----------+-----------------+----------------------------+
| P7.5      | CANFD0 CH1 RX   | CAN FD RX                  |
+-----------+-----------------+----------------------------+
| P2.3      | SCB0 I2C SCL    | I2C clock                  |
+-----------+-----------------+----------------------------+
| P2.4      | SCB0 I2C SDA    | I2C data                   |
+-----------+-----------------+----------------------------+
| P2.1      | SCB6 SPI CLK    | SPI clock                  |
+-----------+-----------------+----------------------------+
| P2.0      | SCB6 SPI MOSI   | SPI MOSI                   |
+-----------+-----------------+----------------------------+
| P2.2      | SCB6 SPI MISO   | SPI MISO                   |
+-----------+-----------------+----------------------------+
| P1.5      | GPIO            | LED0                       |
+-----------+-----------------+----------------------------+
| P4.1      | GPIO            | LED1                       |
+-----------+-----------------+----------------------------+
| P1.7      | GPIO            | LED2                       |
+-----------+-----------------+----------------------------+
| P1.4      | GPIO            | LED3                       |
+-----------+-----------------+----------------------------+
| P9.1      | GPIO            | LED4                       |
+-----------+-----------------+----------------------------+
| P4.0      | GPIO            | LED5                       |
+-----------+-----------------+----------------------------+
| P5.2      | GPIO            | Button SW3                 |
+-----------+-----------------+----------------------------+
| P7.0      | GPIO            | Button SW4                 |
+-----------+-----------------+----------------------------+

System Clock
============

The PSOC™ Control C3M8 and evaluation board provide the following clock sources:

- **IMO** (Internal Main Oscillator): 8 MHz
- **IHO** (Internal High-speed Oscillator): 48 MHz
- **ECO** (External Crystal Oscillator): 16 MHz (supports 4 MHz to 35 MHz)
- **WCO** (Watch Crystal Oscillator): 32.768 kHz

Serial Port
===========

The Zephyr console output is assigned to the debug UART on **SCB2**
(``uart2``).

Default communication settings are **115200 8N1**.

Building
********

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: kit_psc3m8_evk/psc3m8gfs3pahq1/main
   :goals: build

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The KIT_PSC3M8_EVK includes an onboard programmer/debugger (`KitProg3`_) to
provide debugging, flash programming, and serial communication over USB. Flash
and debug commands use OpenOCD and require Infineon OpenOCD to be installed.

Infineon OpenOCD Installation
=============================

The `ModusToolbox™ Programming Tools`_ package includes Infineon OpenOCD.
Alternatively, a standalone installation can be done by downloading the
`Infineon OpenOCD`_ release for your system and extracting the files to a
location of your choice.

Flashing
========

One time, set the Infineon OpenOCD path:

.. code-block:: shell

   west config build.cmake-args -- -DOPENOCD=path/to/infineon/openocd/bin/openocd

Build and flash the application:

.. code-block:: shell

   west build -b kit_psc3m8_evk/psc3m8gfs3pahq1/main -p always samples/hello_world
   west flash

You should see the following message on the console:

.. code-block:: console

   *** Booting Zephyr OS build vX.Y.Z ***
   Hello World! kit_psc3m8_evk/psc3m8gfs3pahq1/main

References
**********

.. _KIT_PSC3M8_EVK:
    https://www.infineon.com/evaluation-board/KIT-PSC3M8-EVK

.. _ModusToolbox™ Programming Tools:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxprogtools

.. _Infineon OpenOCD:
    https://github.com/Infineon/openocd/releases/latest

.. _KitProg3:
    https://github.com/Infineon/KitProg3
