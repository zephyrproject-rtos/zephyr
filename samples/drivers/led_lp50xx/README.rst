.. _lp50xx:

LP5018/24/30/36: 6, 8, 10 or 12 RGB channels
############################################

Overview
********

This sample controls up to 12 LEDs connected to a LP50xx driver.

First, for each LED information is retrieved using the led_get_info syscall
and printed in the log messages. Next, from an infinite loop, a test pattern
(described below) is applied to all the LEDs simultaneously (using the LP50xx
bank mode) and then to each LED one by one (using the
led_set_{brightness,color} syscalls).

Test pattern
============

For all the LEDs first (using channel API) and then for each LED one by one:

For each color in red green blue white yellow purple cyan orange (assuming
the color mapping is RGB):

- set the color
- turn on
- turn off
- set the brightness gradually to the maximum level
- turn off

Building and Running
********************

This sample can be built and executed on boards with an I2C LP5018/24/30/36
LED driver connected. A node matching the "ti,lp50xx" binding shall be defined
in the board DTS files.

As an example this sample provides a DTS overlay for the :ref:`lpcxpresso11u68`
board (:file:`boards/lpcxpresso11u68.overlay`). It assumes that a I2C LP5030
LED driver (with 10 LEDs wired) is connected to the I2C0 bus at address 0x30.

References
**********

- LP5009/12: https://www.ti.com/lit/ds/symlink/lp5012.pdf
- LP5018/24: https://www.ti.com/lit/ds/symlink/lp5024.pdf
- LP503x: https://www.ti.com/lit/ds/symlink/lp5036.pdf
