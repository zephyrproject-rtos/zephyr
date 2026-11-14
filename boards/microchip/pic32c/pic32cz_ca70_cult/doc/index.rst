.. zephyr:board:: pic32cz_ca70_cult

Overview
********

The PIC32CZ CA70 Curiosity Ultra development board is a hardware platform to
evaluate the Microchip PIC32CZ CA70 microcontroller, a Cortex-M7 device that
is pin compatible with the SAM E70/S70/V70/V71 family.

Hardware
********

- 144-pin PIC32CZ2051CA70144 microcontroller, Cortex-M7 at up to 300 MHz
- 2 MB of flash and 512 KB of SRAM (shared with TCM)
- 12 MHz and 32.768 kHz crystals
- On-board debugger (PKoB4) with virtual COM port
- Two user LEDs and two user push buttons
- Two CAN FD transceivers
- Ethernet RMII interface to a modular PHY header
- 32 Mbit QSPI flash
- AT24MAC402 EEPROM with EUI-48 MAC address
- Arduino Uno R3, mikroBUS and Xplained Pro extension headers
- Graphics interface

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The console is on UART1 (PA5 RX, PA6 TX), connected to the on-board debugger
virtual COM port, at 115200 baud.

The user LEDs are LED0 on PD23 and LED1 on PD29, both active low. The user
buttons are SW0 on PA1 and SW1 on PA9, both active low.

System Clock
============

The 12 MHz crystal drives PLLA at 300 MHz. The processor runs at 300 MHz and
the master clock (MCK) at 150 MHz.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Boot From Flash
===============

The device boots from the ROM unless GPNVM bit 1 is set. A chip erase clears
this bit, and MPLAB IPE erases the chip before programming. After programming
with MPLAB IPE, set GPNVM bit 1 again, either in MPLAB IPE or with J-Link
Commander:

.. code-block:: console

   w4 0x400E0C04 0x5A00010B

This writes the Set GPNVM Bit command (SGPB) with argument 1 to the EEFC_FCR
register. Never set GPNVM bit 0: it is the security bit, and only a chip erase
through the ERASE pin clears it.

Flash Using MPLAB IPE
=====================

For instructions on flashing using MPLAB IPE see :ref:`microchip-mplab-ipe-flashing`.

Flash Using J-Link
==================

Connect a J-Link probe to the board's debug header, then build and flash an
application, for example:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: pic32cz_ca70_cult
   :goals: build flash
   :flash-args: -r jlink

References
**********

- `PIC32CZ CA70/MC70 Family Data Sheet`_
- `Migration Guide from SAM E70/S70/V70/V71 to PIC32CZ CA70/MC70`_

.. _PIC32CZ CA70/MC70 Family Data Sheet:
   https://www.microchip.com/en-us/product/PIC32CZ2051CA70144

.. _Migration Guide from SAM E70/S70/V70/V71 to PIC32CZ CA70/MC70:
   https://www.microchip.com/en-us/application-notes/an5816
