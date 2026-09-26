.. zephyr:board:: kit_psc3m5_cc2

Overview
********

The `KIT_PSC3M5_CC2`_ is the connectivity card supplied with the
`KIT_PSC3M5_MC1`_ motor control kit. It is based on the PSOC™ Control C3
family, featuring a PSOC™ Control C3M5 (PSC3M5FDS2AFQ1) microcontroller with
an Arm® Cortex®-M33 core. The card carries the control and communication
interfaces for the kit, while the companion power board carries the inverter
stage.

Key features include 256 KB flash, 64 KB SRAM, an isolated CAN FD interface,
advanced timers with high-resolution capability, and a high-performance
programmable analog subsystem.

The board supports a mikroBUS expansion socket and includes an onboard
isolated SEGGER J-Link LITE programmer/debugger.

Hardware
********

- **SoC:** PSOC™ Control C3M5 (PSC3M5FDS2AFQ1, PG-E-LQFP-80)
- **CPU:** Arm® Cortex®-M33, configured at 180 MHz
- **Flash:** 256 KB
- **SRAM:** 64 KB
- **Connectivity:** Isolated CAN FD, SCB (UART/SPI/I2C)
- **Peripherals:** TCPWM timers, programmable analog (HPPASS)
- **Security:** Arm® TrustZone®-M
- **Debug:** Onboard isolated SEGGER J-Link LITE (SWD + UART bridge)
- **Expansion:** mikroBUS socket, 100-pin power board connector
- **User I/O:** Two user LEDs, one user button, two potentiometers

For more information about the PSOC™ Control C3 and KIT_PSC3M5_CC2:

- `PSOC Control C3 SoC Website`_
- `KIT_PSC3M5_CC2 Board Website`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

LEDs
----

+------+-------------------+
| Name | GPIO Pin          |
+======+===================+
| LED0 | P9.4 (active low) |
+------+-------------------+
| LED1 | P9.5 (active low) |
+------+-------------------+

Push Buttons
------------

+------+-------------------+
| Name | GPIO Pin          |
+======+===================+
| SW1  | P4.6 (active low) |
+------+-------------------+

Default Zephyr Peripheral Mapping
----------------------------------

+------+------------------+-----------------+
| Pin  | Function         | Usage           |
+======+==================+=================+
| P2.2 | SCB1 UART RX     | Console RX      |
+------+------------------+-----------------+
| P2.3 | SCB1 UART TX     | Console TX      |
+------+------------------+-----------------+
| P6.2 | CAN1 RX          | CAN FD receive  |
+------+------------------+-----------------+
| P6.3 | CAN1 TX          | CAN FD transmit |
+------+------------------+-----------------+
| P5.0 | SCB3 SPI MOSI    | mikroBUS MOSI   |
+------+------------------+-----------------+
| P5.1 | SCB3 SPI MISO    | mikroBUS MISO   |
+------+------------------+-----------------+
| P5.2 | SCB3 SPI CLK     | mikroBUS SCK    |
+------+------------------+-----------------+
| P5.3 | SCB3 SPI SELECT0 | mikroBUS CS     |
+------+------------------+-----------------+
| P0.0 | GPIO             | mikroBUS INT    |
+------+------------------+-----------------+
| P0.1 | GPIO             | mikroBUS PWM    |
+------+------------------+-----------------+
| P9.4 | GPIO             | LED0            |
+------+------------------+-----------------+
| P9.5 | GPIO             | LED1            |
+------+------------------+-----------------+
| P4.6 | GPIO             | Button SW1      |
+------+------------------+-----------------+
| P9.0 | TCPWM 2.6        | PWM output      |
+------+------------------+-----------------+

System Clock
============

The PSOC™ Control C3M5 runs from the internal oscillator. The configured
clock path is:

- **FLL**: 96 MHz
- **DPLL-LP0**: 180 MHz
- **DPLL-LP1**: 240 MHz
- **CLK_HF0**: 180 MHz (system clock)
- **CLK_HF2**: 80 MHz (peripheral group 4: UART, CAN FD, SPI)
- **CLK_HF3**: 240 MHz

CLK_HF2 is divided to 80 MHz so that the CAN FD bit timing meets CiA 601-3.

A 16 MHz crystal is fitted on P1.0 and P1.1. Zephyr does not currently provide
an external crystal oscillator clock node for PSOC™ Control C3, so it is
unused. No 32.768 kHz watch crystal is fitted, so the watch crystal oscillator
is disabled in the board devicetree and P0.0 and P0.1 are routed to the
mikroBUS socket instead.

Serial Port
===========

The PSOC™ Control C3M5 has six SCB (Serial Communication Block) interfaces
that can be configured as UART, SPI, or I2C. The Zephyr console output is
assigned to **SCB1** (``uart1``), which is routed through the onboard J-Link
LITE USB-UART bridge. Hardware flow control is not routed on this board.

Default communication settings are **115200 8N1**.

CAN FD
======

The CAN interface is galvanically isolated. The controller signals reach an
Infineon TLE9371VSJ transceiver through a 2DIB1400F dual digital isolator::

   P6.3 CAN1_TX --> INA  ==||== OUTA --> TXD --+ TLE9371VSJ +-- CANH
   P6.2 CAN1_RX <-- OUTB ==||== INB  <-- RXD --+  STB = GND +-- CANL
                     2DIB1400F

The transceiver standby pin is tied to the isolated ground. Standby is active
high, so the transceiver is permanently in normal mode and requires no software
control. The board devicetree therefore contains no ``can-transceiver-gpio``
node, and ``can1`` has no ``phys`` property. CAN applications run on this board
without an overlay.

mikroBUS Socket
===============

The board provides the ``mikrobus_header`` GPIO nexus and the ``mikrobus_spi``
bus label, so SPI and GPIO shields under :file:`boards/shields` can be built
against the board directly:

.. code-block:: console

   west build -b kit_psc3m5_cc2 --shield mikroe_adc_click samples/basic/blinky

Header Pin Mapping
------------------

+-------+--------+------------+
| Index | Signal | Pin        |
+=======+========+============+
| 0     | AN     | Not mapped |
+-------+--------+------------+
| 1     | RST    | Not mapped |
+-------+--------+------------+
| 2     | CS     | P5.3       |
+-------+--------+------------+
| 3     | SCK    | P5.2       |
+-------+--------+------------+
| 4     | MISO   | P5.1       |
+-------+--------+------------+
| 5     | MOSI   | P5.0       |
+-------+--------+------------+
| 6     | PWM    | P0.1       |
+-------+--------+------------+
| 7     | INT    | P0.0       |
+-------+--------+------------+
| 8     | RX     | P2.2       |
+-------+--------+------------+
| 9     | TX     | P2.3       |
+-------+--------+------------+
| 10    | SCL    | Not mapped |
+-------+--------+------------+
| 11    | SDA    | Not mapped |
+-------+--------+------------+

The AN pin is an analog net shared with potentiometer POT1 rather than a GPIO,
and the RST pin is tied to 3V3 through a pull-up and not routed to the
microcontroller. SCL and SDA are isolated from the microcontroller by unfitted
0 Ohm links, as described below. None of the four is present in the nexus.
``mikrobus_uart`` is not declared because the socket's UART signals are the
console SCB.

SPI
---

SCB3 serves the socket's SPI signals:

+------+------+
| Pin  | SPI  |
+======+======+
| P5.0 | MOSI |
+------+------+
| P5.1 | MISO |
+------+------+
| P5.2 | CLK  |
+------+------+
| P5.3 | CS   |
+------+------+

The chip select is driven by the SCB rather than as a GPIO.

I2C is not available
--------------------

The socket's SCL and SDA pins are not connected to the microcontroller. The
0 Ohm links that would tie them to the SCB3 data lines are not fitted, so the
pins terminate at the socket.

``mikrobus_i2c`` is therefore not declared and nexus indices 10 and 11 are left
unmapped, so an I2C shield fails to build rather than failing on hardware.

Analog Inputs
=============

The HPPASS SAR ADC provides 28 channels: twelve directly sampled channels
(0-11) followed by four muxed samplers of four channel slots each (12-15,
16-19, 20-23, 24-27). Sixteen dedicated analog pins feed them.

+---------+----------+-----------------------------+--------------------+
| Channel | Pin      | Net                         | Usage              |
+=========+==========+=============================+====================+
| 0-2     | AN_A0-A2 | Motor 1 phase current U/V/W | Motor drive        |
+---------+----------+-----------------------------+--------------------+
| 3-4     | AN_A3-A4 | Motor 1 IDC / VDC link      | Motor drive        |
+---------+----------+-----------------------------+--------------------+
| 5-7     | AN_A5-A7 | PFC IL0 / IL1 / Iac         | PFC sense          |
+---------+----------+-----------------------------+--------------------+
| 8-10    | AN_B0-B2 | Motor 2 phase current U/V/W | Motor drive        |
+---------+----------+-----------------------------+--------------------+
| 11      | AN_B3    | Motor 2 IDC link            | Motor drive        |
+---------+----------+-----------------------------+--------------------+
| 12      | AN_B4    | Vac0                        | Potentiometer POT1 |
+---------+----------+-----------------------------+--------------------+
| 16      | AN_B5    | Vac1                        | Potentiometer POT2 |
+---------+----------+-----------------------------+--------------------+
| 20      | AN_B6    | BrakeTemp1                  | Brake thermistor   |
+---------+----------+-----------------------------+--------------------+
| 24      | AN_B7    | Vbus PFC                    | PFC sense          |
+---------+----------+-----------------------------+--------------------+

Only channels 12 and 16 are user inputs. The remaining channels carry motor
drive and PFC sense signals that reach the microcontroller through the power
board connector.

Potentiometer POT1 shares its net with the mikroBUS AN pin, so a Click board
that drives AN also affects the POT1 reading. POT2 is independent of the
socket.

Power Board Connector
=====================

The pins that are not assigned to the interfaces above are the gate drive and
enable signals of the two motor inverters, routed to the power board connector.

.. warning::

   These pins drive inverter gate drivers. Do not run an application that
   toggles them while a power board is attached. Driving a high side signal and
   its matching low side signal together would shoot through the half bridge.

+-----------+------------------+----------------------------+----------------------+
| Pin       | Net              | Function                   | TCPWM Output         |
+===========+==================+============================+======================+
| P4.0-P4.5 | U1/V1/W1 H and L | Motor 1 gate drive         | None                 |
+-----------+------------------+----------------------------+----------------------+
| P8.2      | U2_H             | Motor 2 phase U, high side | pwm2_5               |
+-----------+------------------+----------------------------+----------------------+
| P8.3      | U2_L             | Motor 2 phase U, low side  | pwm2_5 complementary |
+-----------+------------------+----------------------------+----------------------+
| P9.0      | V2_H             | Motor 2 phase V, high side | pwm2_6               |
+-----------+------------------+----------------------------+----------------------+
| P9.1      | V2_L             | Motor 2 phase V, low side  | pwm2_6 complementary |
+-----------+------------------+----------------------------+----------------------+
| P9.2      | W2_H             | Motor 2 phase W, high side | pwm2_7               |
+-----------+------------------+----------------------------+----------------------+
| P9.3      | W2_L             | Motor 2 phase W, low side  | pwm2_7 complementary |
+-----------+------------------+----------------------------+----------------------+
| P8.4      | ENPOW1           | Power stage 1 enable       | None                 |
+-----------+------------------+----------------------------+----------------------+
| P8.5      | ENPOW2           | Power stage 2 enable       | None                 |
+-----------+------------------+----------------------------+----------------------+

The board devicetree enables ``pwm2_6`` on P9.0. The remaining outputs can be
enabled from an application overlay.

Neither user LED can be driven from hardware PWM. P9.4 and P9.5 have no TCPWM
option, and every pin that does have one is an inverter gate drive signal. An
application that requires a dimmable on-board LED must use a software PWM
implementation.

Building
********

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: kit_psc3m5_cc2
   :goals: build

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The `KIT_PSC3M5_CC2`_ includes an onboard isolated SEGGER J-Link LITE
programmer/debugger which can be used to program and debug the PSOC™ Control
C3M5 Cortex®-M33 core. The `SEGGER J-Link Software`_ package must be installed
on the host.

Configuring a Console
=====================

Connect a USB cable from your PC to the debug USB connector on the
`KIT_PSC3M5_CC2`_. Use the serial terminal of your choice (minicom, PuTTY,
etc.) with the following settings:

- **Speed:** 115200
- **Data:** 8 bits
- **Parity:** None
- **Stop bits:** 1

Flashing
========

Build and flash the application:

.. code-block:: shell

   west build -b kit_psc3m5_cc2 -p always samples/hello_world
   west flash

You should see the following message on the console:

.. code-block:: console

   *** Booting Zephyr OS build vX.Y.Z ***
   Hello World! kit_psc3m5_cc2

Debugging
=========

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: kit_psc3m5_cc2
   :goals: debug

Once the GDB console starts, you may set breakpoints and perform standard GDB
debugging on the PSOC™ Control C3M5 Cortex®-M33 core.

References
**********

.. _KIT_PSC3M5_CC2:
    https://www.infineon.com/cms/en/product/evaluation-boards/kit_psc3m5_cc2/

.. _KIT_PSC3M5_CC2 Board Website:
    https://www.infineon.com/cms/en/product/evaluation-boards/kit_psc3m5_cc2/

.. _KIT_PSC3M5_MC1:
    https://www.infineon.com/cms/en/product/evaluation-boards/kit_psc3m5_mc1/

.. _PSOC Control C3 SoC Website:
    https://www.infineon.com/cms/en/product/microcontroller/32-bit-psoc-arm-cortex-microcontroller/32-bit-psoc-control-arm-cortex-m33-mcu/

.. _SEGGER J-Link Software:
    https://www.segger.com/downloads/jlink/
