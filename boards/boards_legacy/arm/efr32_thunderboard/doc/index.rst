.. _efr32_thunderboard:

EFR32 Thunderboard-style boards
###############################

Overview
********

There are a couple of very similar boards, which we categorize as
"Thunderboard-style boards". The name can be seen on some of Silicon Labs products
that use boards of this style, such as EFR32™ Blue Gecko Starter Kit,
a.k.a Thunderboard EFR32BG22.

Those boards contains an MCU from the EFR32BG family built on ARM® Cortex®-M33F
processor with low power capabilities.

For an example of such board, refer to the following site:

- `Thunderboard EFR32BG22 Website`_

Currently the following devices are considered "Thunderboard-style":

.. toctree::
   :maxdepth: 1

   brd4184.rst
   brd2602.rst

Serial Port
===========

The SoCs used on these boards have two USARTs.
USART1 is connected to the board controller and is used for the console.

Programming and Debugging
*************************

.. note::
   Before using the kit the first time, you should update the J-Link firmware
   from `J-Link-Downloads`_

Flashing
========

The Thunderboard boards include `J-Link`_ serial and debug adapters built into the
board. The adapter provides:

- A USB connection to a host computer running `J-Link software`_ or `Silicon Labs
  Simplicity Commander`_.
- A physical UART connection which is relayed over a USB Serial port interface.

For detailed instructions regarding flashing, refer to documentation of a specific
device.

.. _Thunderboard EFR32BG22 Website:
   https://www.silabs.com/development-tools/thunderboard/thunderboard-bg22-kit

.. _J-Link:
   https://www.segger.com/jlink-debug-probes.html

.. _J-Link-Downloads:
   https://www.segger.com/downloads/jlink

.. _J-Link software:
   https://www.segger.com/downloads/jlink

.. _Silicon Labs Simplicity Commander:
   https://www.silabs.com/developers/mcu-programming-options
