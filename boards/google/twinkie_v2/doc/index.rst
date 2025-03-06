.. zephyr:board:: google_twinkie_v2

Overview
********

Google Twinkie V2 is a reference board for the google power delivery analyzer
(PDA) Twinkie V2.

Hardware
********

- STM32G0B1REI6

Supported Features
==================

.. zephyr:board-supported-hw::

Pin Mapping
===========

Default Zephyr Peripheral Mapping:
----------------------------------
- CC1_BUF : PA1
- CC2_BUF : PA3
- VBUS_READ_BUF : PB11
- CSA_VBUS : PC4
- CSA_CC2 : PC5

Programming and Debugging
*************************

Build application as usual for the ``google_twinkie_v2`` board, and flash
using dfu-util or J-Link.

Debugging
=========

Use SWD with a J-Link or ST-Link.
