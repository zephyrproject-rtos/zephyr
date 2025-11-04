.. zephyr:code-sample:: lin-counter
   :name: Local Interconnect Network (LIN) counter
   :relevant-api: lin_controller

   Responses to LIN messages in responder mode.

Overview
********
This sample demonstrates how to use the Local Interconnect Network (LIN) API.
The LIN controller acts as a LIN responder node that responds to LIN commander requests.
The LIN commander sends a request for a counter value, and the LIN responder responds
with the current counter value. The counter value is incremented periodically with duration could
be configured in Kconfig.
Responses are indicated by blinking the LED (if present) and output of the current counter value
to the console.

Requirements
************

* A board with LIN controller support (e.g., ek_ra8m1/r7fa8m1ahecbd).
* A LIN transceiver connected to the LIN controller pins (e.g., `mikroe LIN Click`_).
* A host PC with LIN adapter and LIN commander software (e.g., `PCAN-USB Pro FD`_).

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
   :zephyr-app: samples/drivers/lin/counter
   :board: ek_ra8m1
   :goals: build flash
   :compact:

Testing LIN Communication
=========================
To test the LIN communication, connect the LIN transceiver to a host PC with LIN adapter and LIN
commander software. Configure the LIN commander to send a request for the counter value with the
same identifier as defined in the sample (default is ``0x20``). Start the LIN commander software
and observe the console output of the Zephyr application. The counter value should be printed
each time the LIN commander sends a request. The LED (if present) should blink each time a response
is sent.

For examples, to use the `PCAN-USB Pro FD`_ adapter:

1. Download and install the `PLIN-View Pro`_ tool from the PEAK-System website.

2. Connect the PCAN-USB Pro FD adapter to your computer, then configure the LIN bus settings
   (such as channel, baud rate, and commander mode) in PLIN-View Pro.

3. Set up a LIN schedule table that includes a frame to request the counter value from the
   responder device. Ensure the frame ID matches the counter ID specified in this sample.

4. Run the LIN commander schedule in PLIN-View Pro. Monitor the Zephyr application's console for
   counter updates and LED activity.

Sample output
=============

.. code-block:: console

   LIN DUT started in RESPONDER mode
   Counter ID: 0x20
   Counter: 0
   Counter: 1
   Counter Update Frame has been transmitted

.. _mikroe LIN Click:
   https://www.mikroe.com/lin-click

.. _PCAN-USB Pro FD:
   https://www.peak-system.com/PCAN-USB-Pro-FD.366.0.html?&L=1

.. _PLIN-View Pro:
   https://www.peak-system.com/PLIN-View-Pro.243.0.html?&L=1
