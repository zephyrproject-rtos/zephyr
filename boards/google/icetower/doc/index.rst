.. zephyr:board:: google_icetower

Overview
********

Google Icetower Development Board is a board created by Google for
fingerprint-related functionality development.

Board has connectors for fingerprint sensors. Console is exposed over `μServo`_
connector. MCU can be flashed using μServo or SWD.

Hardware
********

- STM32H7A3VIT6 LQFP100 package

Pin Mapping
===========

Default Zephyr Peripheral Mapping:
----------------------------------
- USART_1 TX/RX : PA9/PA10
- SPI_1 CS/CLK/MISO/MOSI : PA4/PA5/PA6/PA7
- SPI_4 CS/CLK/MISO/MOSI : PE11/PE12/PE13/PE14

Programming and Debugging
*************************

Build application as usual for the ``google_icetower`` board, and flash
using μServo or an external J-Link connected to J4. If μServo is used, please
follow the `Chromium EC Flashing Documentation`_.

Debugging
=========

Use SWD with a J-Link or ST-Link. Remember that SW2 must be set to CORESIGHT.

References
**********

.. target-notes::

.. _Chromium EC Flashing Documentation:
   https://chromium.googlesource.com/chromiumos/platform/ec#Flashing-via-the-servo-debug-board

.. _μServo:
   https://chromium.googlesource.com/chromiumos/third_party/hdctools/+/master/docs/servo_micro.md
