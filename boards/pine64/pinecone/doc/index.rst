.. zephyr:board:: pinecone

Overview
********

The Pine64 PineCone BL602 Evaluation Board is a low-cost development board built around the
Bouffalo Lab BL602 Wi-Fi + BLE chipset. The BL60x series has 276KB of RAM, and supports 2.4GHz
Wi-Fi 802.11 b/g/n and BLE 5.0.

The PineCone is populated with the BL602C20 package, which provides 2MB SiP flash memory.

Hardware
********

PineCone provides the following hardware components:

- BL602C20 SoC

- 2 LEDs:

   - RGB LED on GPIOs 17, 14 and 11
   - Power LED

- Reset Button (RST)

- CH340N USB-to-UART bridge

For more information about the Bouffalo Lab BL60x MCU and the PineCone board:

- `Bouffalo Lab BL60x MCU Website`_
- `Bouffalo Lab BL60x MCU Datasheet`_
- `Bouffalo Lab Development Zone`_
- `PineCone Wiki`_
- `PineCone Schematics`_

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The PineCone board is configured to run at max speed (192 MHz).

Serial Port
===========

The ``pinecone`` board uses UART0 as its default serial port (GPIO16 as TX and
GPIO7 as RX).  It is connected to the on-board CH340 USB-to-UART converter and
is used for both programming and console output.

Programming and Debugging
*************************

Samples
=======

#. Build the Zephyr kernel and the :zephyr:code-sample:`hello_world` sample
   application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: pinecone
      :goals: build flash

#. To flash, move the boot jumper to the position closest to the board edge (H),
   press and release RST to enter the boot ROM, then run ``west flash``.

#. Run your favorite terminal program to listen for output. Under Linux the
   terminal should be :code:`/dev/ttyUSB0`. For example:

   .. code-block:: console

      $ screen /dev/ttyUSB0 115200

   Connection should be configured as follows:

      - Speed: 115200
      - Data: 8 bits
      - Parity: None
      - Stop bits: 1

   Move the boot jumper back and press and release RST.

   .. code-block:: console

      *** Booting Zephyr OS build v4.4.0 ***
      Hello World! pinecone/bl602c20q2i

Congratulations, you have ``pinecone`` configured and running Zephyr.

.. _Bouffalo Lab BL60x MCU Website:
	https://en.bouffalolab.com/product/?type=detail&id=6

.. _Bouffalo Lab BL60x MCU Datasheet:
	https://github.com/bouffalolab/bl_docs/tree/main/BL602_DS/en

.. _Bouffalo Lab Development Zone:
	https://dev.bouffalolab.com/home?id=guest

.. _PineCone Wiki:
	https://wiki.pine64.org/wiki/PineCone

.. _PineCone Schematics:
	https://pine64.org/documentation/PineCone/Further_information/Information_and_schematics/
