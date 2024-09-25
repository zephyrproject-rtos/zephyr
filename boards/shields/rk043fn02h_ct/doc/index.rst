.. _rk043fn02h_ct:

RK043FN02H-CT Parallel Display
##############################

Overview
********

RK043FN02H-CT is a 4.3 inch TFT 480*272 pixels with LED backlight and
capacitive touch panel from Rocktech. This LCD panel can work with several i.MX
RT EVKs and LPC MCUs for evaluation of applications with display.

More information about the shield can be found at the `RK043FN02H-CT product
page`_.

This display uses a 40 pin parallel FPC interface plus 6 pin I2C interface,
available on many NXP EVKs. Note that this parallel FPC interface is not
compatible with the MIPI FPC interface present on other NXP EVKs.

Pins Assignment of the Rocktech RK043FN02H-CT Parallel Display
==============================================================

+-----------------------+------------------------+
| Parallel FPC Pin      | Function               |
+=======================+========================+
| 1                     | LED backlight cathode  |
+-----------------------+------------------------+
| 2                     | LED backlight anode    |
+-----------------------+------------------------+
| 3                     | GND                    |
+-----------------------+------------------------+
| 4                     | VDD (3v3)              |
+-----------------------+------------------------+
| 5-7                   | GND                    |
+-----------------------+------------------------+
| 8-12                  | LCD D11-D15            |
+-----------------------+------------------------+
| 13-14                 | GND                    |
+-----------------------+------------------------+
| 15-20                 | LCD D5-D10             |
+-----------------------+------------------------+
| 21-23                 | GND                    |
+-----------------------+------------------------+
| 24-28                 | LCD D0-D4              |
+-----------------------+------------------------+
| 29                    | GND                    |
+-----------------------+------------------------+
| 30                    | LCD CLK                |
+-----------------------+------------------------+
| 31                    | LCD DISP               |
+-----------------------+------------------------+
| 32                    | LCD HSYNC              |
+-----------------------+------------------------+
| 33                    | LCD VSYNC              |
+-----------------------+------------------------+
| 34                    | LCD DE                 |
+-----------------------+------------------------+
| 35                    | NC                     |
+-----------------------+------------------------+
| 36                    | GND                    |
+-----------------------+------------------------+
| 37-40                 | NC                     |
+-----------------------+------------------------+

+-----------------------+------------------------+
| I2C Connector Pin     | Function               |
+=======================+========================+
| 1                     | VDD (3v3)              |
+-----------------------+------------------------+
| 2                     | LCD Touch Reset        |
+-----------------------+------------------------+
| 3                     | LCD Touch Interrupt    |
+-----------------------+------------------------+
| 4                     | LCD I2C SCL            |
+-----------------------+------------------------+
| 5                     | LCD I2C SDA            |
+-----------------------+------------------------+
| 6                     | GND                    |
+-----------------------+------------------------+

Requirements
************

This shield can only be used with a board which provides a configuration
for the 40+6 pin parallel/I2C FPC interface

Programming
***********

Set ``--shield rk043fn02h_ct`` when you invoke ``west build``. For
example:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/display
   :board: mimxrt1060_evk
   :shield: rk043fn02h_ct
   :goals: build

References
**********

.. target-notes::

.. _RK043FN02H-CT product page:
   https://www.nxp.com/design/design-center/development-boards-and-designs/i-mx-evaluation-and-development-boards/4-3-lcd-panel:RK043FN02H-CT
