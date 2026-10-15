.. zephyr:board:: kit_psc3m6_evk

Overview
********

The PSOC™ Control C3M6 Evaluation Kit (`KIT_PSC3M6_EVK`_) is an evaluation board for the
PSC3M6 microcontroller. The PSC3M6 is an Arm® Cortex®-M33 based SoC from Infineon's
CAT1B family. The PSC3M6GES3AHQ1 variant on this board features 512 KB flash, 128 KB SRAM,
and TrustZone-M security.

Hardware
********

- **SoC:** PSC3M6GES3AHQ1 (PG-E-LQFP-100)
- **CPU:** Arm® Cortex®-M33 at up to 180 MHz
- **Flash:** 512 KB internal flash
- **SRAM:** 128 KB
- **Connectivity:** CAN FD, SCB (UART/SPI/I2C)
- **Security:** TrustZone-M
- **Debug:** SEGGER J-Link (requires SEGGER J-Link version v9.68 or later)
- **User I/O:** Two user LEDs, two user buttons

Kit Contents
============

The `KIT_PSC3M6_EVAL User Guide`_ lists the following kit contents:

- PSOC™ Control C3M6 Evaluation Kit board
- USB Type-A to Type-C cable
- Jumper wires (10 wires)
- Quick start guide (QR code for web information)

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
| LED0    | P8.4 (active low) |
+---------+-------------------+
| LED1    | P8.5 (active low) |
+---------+-------------------+

Push Buttons
------------

+---------+--------------------+
| Name    | GPIO Pin           |
+=========+====================+
| SW4     | P10.2 (active low) |
+---------+--------------------+
| SW3     | P2.0 (active low)  |
+---------+--------------------+

Default Zephyr Peripheral Mapping
----------------------------------

+-----------+-----------------+----------------------------+
| Pin       | Function        | Usage                      |
+===========+=================+============================+
| P6.3      | SCB3 UART TX    | Console TX                 |
+-----------+-----------------+----------------------------+
| P6.2      | SCB3 UART RX    | Console RX                 |
+-----------+-----------------+----------------------------+
| P3.3      | SCB4 UART TX    | User UART TX               |
+-----------+-----------------+----------------------------+
| P3.2      | SCB4 UART RX    | User UART RX               |
+-----------+-----------------+----------------------------+
| P5.3      | CANFD0 CH1 TX   | CAN FD TX                  |
+-----------+-----------------+----------------------------+
| P5.2      | CANFD0 CH1 RX   | CAN FD RX                  |
+-----------+-----------------+----------------------------+
| P9.0      | SCB0 I2C SCL    | I2C clock                  |
+-----------+-----------------+----------------------------+
| P9.2      | SCB0 I2C SDA    | I2C data                   |
+-----------+-----------------+----------------------------+
| P7.0      | SCB2 SPI CLK    | SPI clock                  |
+-----------+-----------------+----------------------------+
| P7.1      | SCB2 SPI MOSI   | SPI MOSI                   |
+-----------+-----------------+----------------------------+
| P7.2      | SCB2 SPI MISO   | SPI MISO                   |
+-----------+-----------------+----------------------------+
| P8.4      | GPIO            | LED0                       |
+-----------+-----------------+----------------------------+
| P8.5      | GPIO            | LED1                       |
+-----------+-----------------+----------------------------+
| P10.2     | GPIO            | Button SW4                 |
+-----------+-----------------+----------------------------+
| P2.0      | GPIO            | Button SW3                 |
+-----------+-----------------+----------------------------+

System Clock
============

The PSOC™ Control C3M6 and evaluation board provide the following clock sources:

- **IMO** (Internal Main Oscillator): 8 MHz
- **IHO** (Internal High-speed Oscillator): 48 MHz
- **ECO** (External Crystal Oscillator): 16 MHz, connected to P1.0 and P1.1
- **WCO** (Watch Crystal Oscillator): 32.768 kHz, connected to P0.0 and P0.1

Serial Port
============

The Zephyr console output is assigned to the debug UART on **SCB3**
(``uart3``).

Default communication settings are **115200 8N1**.

Building
********

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: kit_psc3m6_evk
   :goals: build

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Use ``west flash`` and ``west debug`` with OpenOCD and SEGGER J-Link probe
(requires SEGGER J-Link version v9.68 or later).

Infineon OpenOCD Installation
=============================

The `ModusToolbox™ Programming Tools`_ package includes Infineon OpenOCD.
Alternatively, a standalone installation can be done by downloading the
`Infineon OpenOCD`_ release for your system and extracting the files to a
location of your choice.

Flashing
========

Build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: kit_psc3m6_evk
   :goals: build flash
   :west-args: -p always
   :flash-args: --openocd <path/to/openocd>

References
**********

.. _KIT_PSC3M6_EVK:
    https://www.infineon.com/evaluation-board/KIT-PSC3M6-EVAL

.. _KIT_PSC3M6_EVAL User Guide:
    https://www.infineon.com/document-promo/infineon-kit-psc3m6-eval-user-guide-usermanual-en_3885ebcf-6410-484d-9f5c-0e3c645aec40

.. _ModusToolbox™ Programming Tools:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxprogtools

.. _Infineon OpenOCD:
    https://github.com/Infineon/openocd/releases/latest
