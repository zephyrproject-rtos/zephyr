.. _lcd_par_s035:

NXP LCD_PAR_S035 TFT LCD Module
###############################

Overview
********

The LCD-PAR-S035 is a 3.5” 480x320 IPS TFT LCD module with wide viewing angle
and 5-point capacitive touch functionality. The LCD module can be controlled
through either SPI or parallel (8/16bit) 8080/6800.
More information about the shield can be found
at the `LCD-PAR-S035 product page`_.

Requirements
************

This shield can only be used with FRDM-X evaluation kits with a parallel LCD
connector or a PMOD connector.

Programming
***********

Set ``--shield lcd_par_s035_8080`` or ``--shield lcd_par_s035_spi`` when you
invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/display
   :board: frdm_mcxn947/mcxn947/cpu0
   :shield: lcd_par_s035_8080
   :goals: build

.. include:: ../../../nxp/common/board-footer.rst
   :start-after: nxp-board-footer

References
**********

.. target-notes::

.. _LCD-PAR-S035 product page:
   https://www.nxp.com/design/design-center/development-boards-and-designs/general-purpose-mcus/3-5-480x320-ips-tft-lcd-module:LCD-PAR-S035
