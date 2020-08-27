.. _ls0xx_generic_shield:

Sharp memory display generic shield
#########################################

Overview
********

This is a generic shield for Sharp memory pixel LCD. It supports
displays of LS0XX type. These displays have an SPI interface and
few other pins. Note that the SCS is active high for this display.
The DISP pin controls whether to display memory
contents or show all white pixels, this can be connected
directly to VDD, to always display memory contents. There is
an EXTCOMIN pin which can be configured to control the VCOM.

Sharp memory displays require toggling the VCOM signal periodically
to prevent a DC bias ocurring in the panel. The DC bias can damage
the LCD and reduce the life. This signal must be supplied
from either serial input (sw) or an external signal on the
EXTCOMIN pin. The driver can do this internally
`CONFIG_LS0XX_VCOM_DRIVER` =y (default) else user can handle it in
application code `CONFIG_LS0XX_VCOM_DRIVER` =n.

CONFIG_LS0XX_VCOM_DRIVER=y
  Driver will handle VCOM toggling. User can control method of toggling
  using `CONFIG_LS0XX_VCOM_EXTERNAL`.

CONFIG_LS0XX_VCOM_EXTERNAL=n
  This is the default option.
  VCOM is toggled through serial input by software.
  EXTMODE pin is connected to VSS
  Important: User has to make sure `display_write` is called periodically
  for toggling VCOM. If there is no data to update then buf can
  be set to NULL then only VCOM will be toggled.

CONFIG_LS0XX_VCOM_EXTERNAL=y
  VCOM is toggled using external signal EXTCOMIN.
  EXTMODE pin is connected to VDD
  Important: Driver will start a thread which will
  toggle the EXTCOMIN pin every 500ms. There is no
  dependency on user.

Pins Assignment of the Generic Sharp memory Display Shield
==========================================================

+---------------+---------------------------------------------------------+
| Pin           | Function                                                |
+===============+=========================================================+
| SCS           | Serial Slave Select                                     |
+---------------+---------------------------------------------------------+
| SI            | Serial Data Input                                       |
+---------------+---------------------------------------------------------+
| SCLK          | Serial Clock Input                                      |
+---------------+---------------------------------------------------------+
| EXTCOMIN      | VCOM Inversion Polarity Input (VCOM can be controlled   |
|               | through SW)                                             |
+---------------+---------------------------------------------------------+
| DISP          | Display ON/OFF switching signal (Can be connected       |
|               | directly to VDD)                                        |
+---------------+---------------------------------------------------------+
| EXTMODE       | COM Inversion Selection                                 |
+---------------+---------------------------------------------------------+


Current supported displays
==========================

Following displays are supported but shield only exists
for LS013B7DH03. Other shields can be added by using the LS013B7DH03 as
a reference and changing the width, height, etc configurations.

LS012B7DD01
LS012B7DD06
LS013B7DH03
LS013B7DH05
LS013B7DH06
LS027B7DH01A
LS032B7DD02
LS044Q7DH01

+----------------------+------------------------------+
| Display              | Shield Designation           |
|                      |                              |
+======================+==============================+
| LS013B7DH03          | ls013b7dh03                  |
+----------------------+------------------------------+

Requirements
************

This shield can only be used with a board that provides a configuration
for Arduino connectors and defines node aliases for SPI and GPIO interfaces
(see :ref:`shields` for more details).

Programming
***********

Set ``-DSHIELD=ls013b7dh03`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/gui/lvgl
   :board: nrf52840dk_nrf52840
   :shield: ls013b7dh03
   :goals: build

References
**********

.. target-notes::

.. _LS013B7DH03 Datasheet:
   https://www.mouser.com/datasheet/2/365/LS013B7DH03%20SPEC_SMA-224806.pdf

.. _Sharp memory display app note:
   https://www.sharpsde.com/fileadmin/products/Displays/2016_SDE_App_Note_for_Memory_LCD_programming_V1.3.pdf
