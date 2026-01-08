.. zephyr:board:: mck_ra4t1

Overview
********

The MCK-RA4T1 is an Motor Control Kit for Renesas RA4T1 MCU Group which integrates multiple series of software and
pin-compatible Arm®-based 32-bit cores that share a common set of Renesas peripherals to facilitate design scalability
and efficient platform-based product development.

The MCU in this series incorporates a high-performance Arm Cortex®-M33 core running up to 100 MHz with the following
features:

- Up to 256 KB code flash memory
- 40 KB SRAM
- Analog peripherals
- Security and safety features

MCK-RA4T1 kit includes the items below:

- RA4T1 CPU board (`MCB-RA4T1`_)
- Inverter board (`MCI-LV-1`_)
- Brushless DC Motor
- Accessories (cables, standoffs, etc.)

The specifications of the CPU board are shown below:

**MCU specifications**

- 100 MHz Arm® Cortex®-M33 core based RA4T1 MCU 64 pins, LFQFP package
- ROM/SRAM size: 256KB/40KB
- MCU input clock: 10MHz (Generate with external crystal oscillator)
- Power supply: DC 5V, select one way automatically from the below:

  - Power is supplied from compatible inverter board
  - Power is supplied from USB connector

**Connector**

- Inverter board connector
- USB connector for J-Link OB
- SCI connector for Renesas Motor Workbench communication
- Through hole for CAN communication
- 10 pin through hole for Arm debugger
- PMOD connectors
- MCU reset switch
- User-controllable LED x2, Power LED x1

**Onboard debugger**

This product has the onboard debugger circuit, J-Link On-Board (hereinafter called “J-Link-OB”). When you write a program,
connect the CPU board to PC with USB cable. J-Link-OB operates as debugger equivalent to J-Link. If connecting from flash
programing tool (e.g. J-Flash Lite by SEGGER), set the type of debugger (tool) to “JLink”

Hardware
********
Detailed Hardware features for the RA4T1 MCU group can be found at:
- The RA4T1 MCU group: `RA4T1 Group User's Manual Hardware`_
- The MCB-RA4T1 board: `MCB-RA4T1 - User's Manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``mck_ra4t1`` board can be built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on building and running.

Configuring the Debug Probe
===========================

The MCK-RA4T1 board includes an on-board SEGGER J-Link debugger.
SEGGER J-Link drivers are available at https://www.segger.com/downloads/jlink

To use the on-board J-Link debugger, ensure that:

1. The J-Link OB is connected to the host PC via the USB port (CN9).

2. The J-Link OB jumper is in the default configuration as described in the `MCB-RA4T1 - User's Manual`_.

Configuring a Console
=====================

Connect a USB cable from your PC to the on-board virtual COM port (CN9), and use a
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
   :board: mck_ra4t1
   :goals: flash

Open a serial terminal, reset the board (push the reset switch S1), and you should
see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v4.3.0-xxx-xxxxxxxxxxxxx *****
   Hello World! mck_ra4t1

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mck_ra4t1
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v4.3.0-xxx-xxxxxxxxxxxxx *****
   Hello World! mck_ra4t1

References
**********
- `MCK-RA4T1 Website`_
- `RA4T1 MCU group Website`_

.. _MCK-RA4T1 Website:
   https://www.renesas.com/en/design-resources/boards-kits/mck-ra4t1

.. _RA4T1 MCU group Website:
   https://www.renesas.com/en/products/ra4t1

.. _MCB-RA4T1 - User's Manual:
   https://www.renesas.com/en/document/mat/mcb-ra4t1-users-manual

.. _RA4T1 Group User's Manual Hardware:
   https://www.renesas.com/en/document/mah/ra4t1-group-users-manual-hardware

.. _MCB-RA4T1:
   https://www.renesas.com/en/design-resources/boards-kits/mcb-ra4t1

.. _MCI-LV-1:
   https://www.renesas.com/en/design-resources/boards-kits/mci-lv-1
