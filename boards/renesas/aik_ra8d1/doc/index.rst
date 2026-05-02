.. zephyr:board:: aik_ra8d1

Overview
********

The AIK-RA8D1 provides developers with multiple and reconfigurable connectivity functions
to expedite AI/ML system development. This easy-to-use hardware platform for multimodal
AI/ML solutions is based on the RA8D1 MCU from the RA family of 32-bit MCUs.

The hardware platform is intended for use in a variety of AI/ML single or multimodal use
cases supporting vision, real-time analytics, and audio. It enables multiple combinations
of different plug-in sensors with the available onboard analog microphones and camera.

On-board support is included for several of the most commonly used peripherals, as
well as interfaces for several common ecosystem standards.

The key features of the AIK-RA8D1 board are categorized as follows:

**Renesas RA8D1 Microcontroller Group**

- R7FA8D1BHECBD
- 176-pin LQFP package
- 480 MHz Arm® Cortex® -M85 core with Helium®
- 1 MB on-chip SRAM
- 2 MB on-chip code flash memory
- 12 KB on-chip data flash memory
- On-board external SDRAM (128Mbit)

**Connectivity**

- One USB micro AB full speed connector for the Main MCU
- SEGGER J-Link® On-board (OB) interface for debugging and programming of the RA8D1 MCU. A 10
  pin JTAG/SWD interface are also provided for connecting optional external debuggers and
  programmers
- Six PMOD connectors, allowing use of appropriate PMOD compliant peripheral plug-in modules for
  rapid prototyping
- One Auxiliary Port connector
- One CAN interface
- One RJ-45 RMII Ethernet interface

**Other on-board features**

- 20-pin DVP interface for camera input
- 2 Analog MEMS microphones
- Two user push button switches
- One LED RGB

Hardware
********
Detailed Hardware features for the AIK-RA8D1 kit can be found at `AIK-RA8D1 - User's Manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``aik_ra8d1`` board can be built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on building and running.

Configuring the Debug Probe
===========================

The AIK-RA8D1 board includes an on-board SEGGER J-Link debugger.
SEGGER J-Link drivers are available at https://www.segger.com/downloads/jlink

To use the on-board J-Link debugger, ensure that:

1. The J-Link OB is connected to the host PC via the USB port (J10).

2. The J-Link OB jumper is in the default configuration as described in the `AIK-RA8D1 - User's Manual`_.

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
   :board: aik_ra8d1
   :goals: flash

Open a serial terminal, reset the board (push the reset switch S1), and you should
see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v4.3.0-xxx-xxxxxxxxxxxxx *****
   Hello World! aik_ra8d1

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: aik_ra8d1
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v4.3.0-xxx-xxxxxxxxxxxxx *****
   Hello World! aik_ra8d1

References
**********

.. target-notes::

.. _AIK-RA8D1 Website:
   https://www.renesas.com/en/design-resources/boards-kits/aik-ra8d1

.. _RA8D1 MCU group Website:
   https://www.renesas.com/en/products/microcontrollers-microprocessors/ra-cortex-m-mcus/ra8d1-480-mhz-arm-cortex-m85-based-graphics-microcontroller-helium-and-trustzone

.. _AIK-RA8D1 - User's Manual:
   https://www.renesas.com/en/document/mat/aik-ra8d1-users-manual

.. _RA8D1 Group User's Manual Hardware:
   https://www.renesas.com/en/document/mah/ra8d1-group-users-manual-hardware
