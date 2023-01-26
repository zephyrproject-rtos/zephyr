.. _google_dragonclaw_board:

Google Dragonclaw Development Board
###################################

Overview
********

Dragonclaw is a board created by Google for fingerprint-related functionality
development. See the `Dragonclaw Schematics`_ for board schematics, layout and
BOM.

Board has connectors for fingerprint sensors. Console is exposed over μServo
connector. MCU can be flashed using μServo or SWD.

Hardware
********

- STM32F412CGU6 UFQFPN48 package

Peripherial Mapping
===================

- USART_1 TX/RX : PA9/PA10
- USART_2 TX/RX : PA2/PA3
- SPI_1 CS/CLK/MISO/MOSI : PA4/PA5/PA6/PA7
- SPI_2 CS/CLK/MISO/MOSI : PB12/PB13/PB14/PB15

Programming and Debugging
*************************

Build application as usual for the ``dragonclaw`` board, and flash
using μServo or an external J-Link connected to J4. If μServo is used, please
follow the `Chromium EC Flashing Documentation`_.

Debugging
=========

Use SWD with a J-Link or ST-Link. Remember that SW2 must be set to CORESIGHT.

References
**********

.. target-notes::

.. _Dragonclaw Schematics:
   https://chromium.googlesource.com/chromiumos/platform/ec/+/HEAD/docs/schematics/dragonclaw

.. _Chromium EC Flashing Documentation:
   https://chromium.googlesource.com/chromiumos/platform/ec#Flashing-via-the-servo-debug-board
