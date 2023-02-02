.. _lp5018:

LP5018/24: 6 or 8 RGB channels
##############################

Overview
********

This sample controls up to 6 LEDs connected to a LP5018 driver.

First, for each LED information is retrieved using the led_get_info syscall
and printed in the log messages. Next a test pattern (described below) is
applied to all the LEDs.

Test pattern
============

Implicitly uses led on and led off.

- Loop through the colors
- Set the brightness from 0 to 100%
- Set the LEDs blinking (if enabled)
- Set color and brightness over channel API
- Turn all LEDs off

Building and Running
********************

This sample can be built and executed on boards with an I2C LP5018 LED driver
connected. A node matching the "ti,lp5018" binding should be defined in the
board DTS files.

As an example this sample provides a DTS overlay for the :ref:`stm32h747i_disco_board`
board (:file:`boards/stm32h747i_disco_m7.overlay`). It assumes that an I2C LP5018
LED driver (with 6 LEDs wired) is connected to the I2C4 bus at address 0x28.

References
**********

- LP5018: https://www.ti.com/lit/ds/symlink/lp5018.pdf
- EVM LP5024: https://www.ti.com/tool/LP5024EVM
