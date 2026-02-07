.. zephyr:board:: fpb_ra8e1

Overview
********

The FPB-RA8E1, a Fast Prototyping Board for the RA8E1 MCU Group, enables users to seamlessly evaluate
the features of the RA8E1 MCU group and develop embedded systems applications using Flexible Software
Package (FSP) and the e2 studio IDE. Users can use on-board features along with their choice of popular
ecosystems add-ons to bring their big ideas to life.

The MCU in this series incorporates a high-performance Arm® Cortex®-M85 core with HeliumTMrunning up to 360 MHz
with the following features:

- 1 MB code flash memory
- 544 KB SRAM (32 KB of TCM RAM, 512 KB of user SRAM)
- Octal Serial Peripheral Interface (OSPI)
- Ethernet MAC Controller (ETHERC), USBFS
- Analog peripherals
- Security and safety features

The following components are included in the FPB-RA8E1 box:

- FPB-RA8E1 v1 board
- FPB-RA8E1 v1 Quick Start Guide

The specifications of the CPU board are shown below:

**MCU specifications**

- 360 MHz Arm® Cortex®-M85 core based RA8E1 MCU 144 pins, LQFP package
- ROM/SRAM size: 1MB/512KB
- MCU input clock: 20MHz (Generate with external crystal oscillator)
- Power supply: DC 5V, select one way automatically from the below:

  - Power is supplied from the Debug USB connector (J10)
  - Power is supplied from an external 5V input (J60 or via TP7/TP9)

**Connector**

- Two Digilent Pmod™ (SPI, I2C and UART [Pmod 1] and SPI/UART [Pmod 2]) connectors
- Arduino™ (UNO R3) connectors
- JTAG/SWD Connector
- Debug USB-C connector
- External +5 V power source connector (J60)
- Real Time Clock backup supply battery connector
- 20-pin Camera Interface connector

**Onboard debugger**

This product has the onboard debugger circuit, J-Link On-Board (hereinafter called “J-Link-OB”). When you write a program,
connect the CPU board to PC with USB cable. J-Link-OB operates as debugger equivalent to J-Link. If connecting from flash
programing tool (e.g. J-Flash Lite by SEGGER), set the type of debugger (tool) to “JLink”

Hardware
********
Detailed Hardware features for the RA8E1 MCU group can be found at:
- The RA8E1 MCU group: `RA8E1 Group User's Manual Hardware`_
- The FPB-RA8E1 board: `FPB-RA8E1 - User's Manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``fpb_ra8e1`` board can be built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on building and running.

Configuring the Debug Probe
===========================

The FPB-RA8E1 board includes an on-board SEGGER J-Link debugger.
SEGGER J-Link drivers are available at https://www.segger.com/downloads/jlink

To use the on-board J-Link debugger, ensure that:

1. The J-Link OB is connected to the host PC via the USB port (J10).

2. The J-Link OB jumper is in the default configuration as described in the `FPB-RA8E1 - User's Manual`_.

Configuring a Console
=====================

Connect a USB cable from your PC to the on-board virtual COM port (J10), and use a
serial terminal of your choice (minicom, PuTTY, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flashing
========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: fpb_ra8e1
   :goals: flash

Open a serial terminal, reset the board (push the reset switch S2), and you should
see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v4.3.0-xxx-xxxxxxxxxxxxx *****
   Hello World! fpb_ra8e1

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: fpb_ra8e1
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v4.3.0-xxx-xxxxxxxxxxxxx *****
   Hello World! fpb_ra8e1

References
**********
- `FPB-RA8E1 Website`_
- `RA8E1 MCU group Website`_

.. _FPB-RA8E1 Website:
   https://www.renesas.com/en/design-resources/boards-kits/fpb-ra8e1

.. _RA8E1 MCU group Website:
   https://www.renesas.com/en/products/ra8e1

.. _RA8E1 Group User's Manual Hardware:
   https://www.renesas.com/en/document/mah/ra8e1-group-users-manual-hardware

.. _FPB-RA8E1 - User's Manual:
   https://www.renesas.com/en/document/mat/fpb-ra8e1-v1-users-manual
