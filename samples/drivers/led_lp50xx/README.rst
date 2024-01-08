.. zephyr:code-sample:: lp50xx
   :name: LP50XX RGB LED
   :relevant-api: led_interface

   Control up to 12 RGB LEDs connected to an LP50xx driver chip.

Overview
********

This sample controls up to 12 LEDs connected to a LP50xx driver.

First, for each LED information is retrieved using the led_get_info syscall
and printed in the log messages. Next, from an infinite loop, a test pattern
(described below) is applied to all the LEDs simultaneously (using the
:c:func:`led_write_channels` syscall) and then to each LED one by one (using the
:c:func:`led_set_brightness` and :c:func:`led_set_color` syscalls).

Test pattern
============

For all the LEDs first (using channel API) and then for each LED one by one:

For each color in red green blue white yellow purple cyan orange:

- set the color
- turn on
- turn off
- set the brightness gradually to the maximum level
- turn off

Building and Running
********************

This sample can be built and executed on boards with an I2C LP5009, LP5012,
LP5018, LP5024, LP5030 or LP5036 LED driver connected. A node matching the
the device type binding should be defined in the board DTS files.

As an example this sample provides a DTS overlay for the :ref:`lpcxpresso11u68`
board (:file:`boards/lpcxpresso11u68.overlay`). It assumes that a I2C LP5030
LED driver (with 10 LEDs wired) is connected to the I2C0 bus at address 0x30.

References
**********

- LP5009/LP5012: https://www.ti.com/lit/ds/symlink/lp5012.pdf
- LP5018/LP5024: https://www.ti.com/lit/ds/symlink/lp5024.pdf
- LP5030/LP5036: https://www.ti.com/lit/ds/symlink/lp5036.pdf
