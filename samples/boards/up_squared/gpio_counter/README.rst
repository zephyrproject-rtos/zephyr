.. _up_squared gpio_counter:

UP Squared GPIO Counter
#######################

Overview
********
This sample provides an example of how to configure GPIO input and output to
the UP Squared board.

The sample enables a pin as GPIO input (active high) that triggers the increment
of a counter (range is 0x0 to 0xf). The counter increments for
each change from 0 to 1 on HAT Pin 16 (BIOS Pin 19). The value of the counter is
represented on GPIO output (active high) as a 4-bit value
(bin 0, 1, 2, 3 -> HAT Pin 35, 37, 38, 40).

+------------+-----------------------------+
|  Element   |   Mapping (by column)       |
+============+=====+=====+=====+=====+=====+
| Bit (bin)  | n/a |   3 |   2 |   1 |   0 |
+------------+-----+-----+-----+-----+-----+
| HAT Pin    |  16 |  40 |  39 |  37 |  35 |
+------------+-----+-----+-----+-----+-----+
| BIOS Pin   |  19 |  38 |  27 |  15 |  14 |
+------------+-----+-----+-----+-----+-----+
| Direction  |  IN | OUT | OUT | OUT | OUT |
+------------+-----+-----+-----+-----+-----+
| Active     |   H |   H |   H |   H |   H |
+------------+-----+-----+-----+-----+-----+

For example, a counter value of 0xc (hex) is represented in 0b1100 (binary)
on the GPIO output pins.

Requirements
************

The application requires an UP Squared board connected to the PC through USB
for serial console. The BIOS settings must be updated as specified in the
source code comments for HAT Configurations (see table above).


References
**********

- :ref:`up_squared` board documentation


Building and Running
********************

Build the sample in the following way:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/up_squared/gpio_counter
   :board: up_squared
   :goals: build

Prepare the boot device (USB storage drive) as described for the :ref:`UP Squared <up_squared>`
board. Insert the USB boot device containing the prepared software binary of the sample.

Connect the board to a host computer and open a serial connection for serial
console interface::

   $ minicom -D <tty_device> -b 115200

Replace :code:`<tty_device>` with the port where the UP Squared board
can be found. For example, under Linux, :code:`/dev/ttyUSB0`.
The ``-b`` option sets baud rate.

Power On the board. The board will boot then enter GRUB boot loader unless BIOS
option is selected. Enter the BIOS configuration menu, modify the required HAT
configurations (above) and then select to save the BIOS settings and reset.

The board will reboot and then enter GRUB boot loader. Select to boot Zephyr and
the board will start to execute the sample. Apply input to trigger the increment
of the value of the counter.

There are several ways to observe the sample behavior in addition to serial
console display of the counter value. For example, the input signal can be
implemented with an analog button on a breakout breadboard, or with a basic pulse
provided by a GPIO output (active High) pin of another GPIO device (eg Arduino
Uno). The Up Squared GPIO output signals can each be connected to a simple LED
circuit on a breakout breadboard to illuminate the 4-bit counter value, as
shown in the example below::

   o----> to Up Squared
   |      GPIO Output Pin
  _|_
  \ /
  ---
   |
   R1
   |
   +---> GND
