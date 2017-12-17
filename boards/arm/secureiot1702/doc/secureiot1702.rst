.. _secureiot1702:

Microchip SecureIoT1702
#######################

Overview
********

Board configuration for Microchip CEC1702 demo board. This is a cryptographic
embedded controlled based on an ARM Cortex-M4.

Highlights of the board:

- CEC1702 32-bit ARM® Cortex®-M4F Controller with Integrated Crypto
- Compact, high-contrast, serial graphic LCD Display Module with back-light
- 2x4 matrix of push buttons inputs
- USB-UART Converter as debug interface
- Potentiometer to ADC channel
- Serial Quad I/O (SQI) flash
- OTP programmable in CEC1702
- Two expansion headers compatible with MikroElektronika mikroBUS™ Expansion interface

More information can be found from `SecureIoT1702 website`_ and
`CEC1702 website`_. The SoC programming information is available
in `CEC1702 datasheet`_.

Supported Features
==================

Following devices are supported:

- Nested Vectored Interrupt Controller (NVIC)
- System Tick System Clock (SYSTICK)
- Serial Ports (NS16550)
- Quad Master-only SPI controller (QMSPI)


Connections and IOs
===================

Please refer to the `SecureIoT1702 schematics`_ for the pin routings.
Additional devices can be connected via mikroBUS expansion interface.

Programming and Debugging
*************************

Flashing
========
[How to use this board with Zephyr and how to flash a Zephyr binary on this
device]


Debugging
=========
[ How to debug this board]


References
**********

.. target-notes::

.. _CEC1702 website:
   http://www.microchip.com/CEC1702

.. _CEC1702 datasheet:
   http://www.microchip.com/p/207/

.. _SecureIoT1702 website:
   http://www.microchip.com/Developmenttools/ProductDetails.aspx?PartNO=DM990012

.. _SecureIoT1702 schematics:
   http://microchipdeveloper.com/secureiot1702:schematic
