.. _google_quincy_board:

Google Quincy Development Board
###################################

Overview
********

Quincy is a board created by Google for fingerprint-related functionality
development.

Board has connectors for fingerprint sensors. Console is exposed over μServo
connector. MCU can be flashed using μServo or SWD.

Hardware
********

- NPCX99FPA0BX VFBGA144 package

Peripherial Mapping
===================

- UART_1 (CONSOLE) TX/RX : GPIO65/GPIO64
- UART_2 (PROG) TX/RX : GPIO86/GPIO75
- SPI_0 (SHI) CS/CLK/MISO/MOSI : GPIO53/GPIO55/GPIO47/GPIO46
- SPI_1 (SPIP) CS/CLK/MISO/MOSI : GPIOA6/GPIOA1/GPIO95/GPIOA3
- SPI_2 (GP) CS/CLK/MISO/MOSI : GPIO30/GPIO25/GPIO24/GPIO31

Programming and Debugging
*************************

Build application as usual for the ``quincy`` board, and flash
using μServo or an external J-Link connected to J4. If μServo is used, please
follow the `Chromium EC Flashing Documentation`_.

Debugging
=========

Use SWD with a J-Link.

References
**********

.. target-notes::

.. _Chromium EC Flashing Documentation:
   https://chromium.googlesource.com/chromiumos/platform/ec#Flashing-via-the-servo-debug-board
