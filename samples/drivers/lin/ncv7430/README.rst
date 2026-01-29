.. zephyr:code-sample:: lin-ncv7430
   :name: NCV7430 LED Controller over LIN
   :relevant-api: lin_controller

   Sample application for using the NCV7430 LED Controller with Zephyr's LIN API.

Overview
********

This sample demonstrates how to use the Local Interconnect Network (LIN) API to control a RGB LED
using NCV7430 LED Controller. You could find more details about NCV7430 in the
`NCV7430 Datasheet`_.

The LIN controller acts as a LIN commander node that sends commands to the NCV7430 LED controller
to change the LED colors. The application cycles through a set of predefined colors, sending the
appropriate LIN messages to the NCV7430 to update the LED color accordingly.

Requirements
************

* A board with LIN controller support (e.g., ek_ra8m1/r7fa8m1ahecbd).
* An NCV7430 evaluation board connected to the LIN controller pins (e.g. `skpang NCV7430 Breakout Board`_).

Building and Running
********************

Building and Running for EK-RA8M1
=================================
The :zephyr:board:`ek_ra8m1` board does not come with an onboard LIN transceiver. In order to use
the LIN bus on the EK-RA8M1 board, an external LIN bus transceiver `mikroe LIN Click`_ must be
connected to this board:

- P609 EK-RA8M1 (TXD) <-> RXD mikroe LIN Click
- P610 EK-RA8M1 (RXD) <-> TXD mikroe LIN Click
- GND EK-RA8M1 <--------> GND mikroe LIN Click
- 3.3V EK-RA8M1 <-------> 3V3 mikroe LIN Click
- P601 EK-RA8M1 <--------> EN mikroe LIN Click
- P602 EK-RA8M1 <--------> WK mikroe LIN Click

.. note::

   An additional pull-up resistor (10k Ohm) is required between EK-RA8M1 P609 (TXD) and 3.3V to
   ensure proper idle state on the LIN bus.

The sample can be built and executed for the EK-RA8M1 as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/lin/ncv7430
   :board: ek_ra8m1
   :goals: build flash
   :compact:

Observe the LED color changes on the NCV7430 evaluation board as the application cycles through
the predefined colors.

Sample Output
=============

.. code-block:: console

   NCV7430 sample started
   Set LED color: Red - read back actual color code: 0xFF0000
   Set LED color: Green - read back actual color code: 0x00FF00
   Set LED color: Blue - read back actual color code: 0x0000FF

.. _skpang NCV7430 Breakout Board:
   https://www.skpang.co.uk/ncv7430-led-controller-breakout-board-p-5187.html

.. _NCV7430 Datasheet:
   https://www.onsemi.com/pdf/datasheet/ncv7430-d.pdf

.. _mikroe LIN Click:
   https://www.mikroe.com/lin-click
