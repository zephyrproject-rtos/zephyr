.. zephyr:code-sample:: lin-ncv7430-responder
   :name: NCV7430 mock LIN responder
   :relevant-api: lin_controller

   Sample application that mocks the NCV7430 LED Controller as a LIN responder node.

Overview
********

This sample demonstrates how to use the Local Interconnect Network (LIN) API in **responder**
mode by mocking the LIN-facing behavior of the NCV7430 LED Controller. You could find more
details about the real NCV7430 protocol in the `NCV7430 Datasheet`_.

The LIN controller acts as a LIN responder node that reacts to the same frame IDs used by the
:zephyr:code-sample:`lin-ncv7430` commander sample: it stores the LED color/control data written
by ``Set_LED_Control`` (0x23) and ``Set_Color`` (0x24) frames, and answers ``Commander_Command`` /
``Responder_Response`` (0x3C/0x3D) read requests with the last color it was told to set.

This lets the :zephyr:code-sample:`lin-ncv7430` commander sample (or any other LIN commander
sending the same frame IDs) be exercised on a real LIN bus without needing an actual NCV7430 chip
- two boards wired together on the LIN bus are enough.

.. note::

   This sample does not install a LIN RX filter (:c:func:`lin_set_rx_filter`), so it inspects every
   header seen on the bus. This keeps the mock simple; a responder for a bus with many other nodes
   would typically filter on just the frame IDs it needs to answer.

Requirements
************

* Two boards with LIN controller support (e.g., ek_ra8m1/r7fa8m1ahecbd): one running this sample,
  one running the :zephyr:code-sample:`lin-ncv7430` commander sample.
* An external LIN bus transceiver on each board (e.g. two :ref:`mikroe_lin_click_shield`), with
  their LIN bus and GND lines tied together.

Building and Running
*********************

Building and Running for EK-RA8M1
==================================

The :zephyr:board:`ek_ra8m1` board does not come with an onboard LIN transceiver. In order to use
the LIN bus on the EK-RA8M1 board, an external LIN bus transceiver :ref:`mikroe_lin_click_shield`
must be connected to this board, same as for the :zephyr:code-sample:`lin-ncv7430` sample:

- P609 EK-RA8M1 (TXD) <-> RXD mikroe LIN Click
- P610 EK-RA8M1 (RXD) <-> TXD mikroe LIN Click
- GND EK-RA8M1 <--------> GND mikroe LIN Click
- 3.3V EK-RA8M1 <-------> 3V3 mikroe LIN Click
- P601 EK-RA8M1 <--------> EN mikroe LIN Click
- P602 EK-RA8M1 <--------> WK mikroe LIN Click

.. note::

   An additional pull-up resistor (10k Ohm) is required between EK-RA8M1 P609 (TXD) and 3.3V to
   ensure proper idle state on the LIN bus.

Wire the LIN bus and GND lines of this board's mikroe LIN Click shield to the LIN bus and GND
lines of the second board's shield (the one running the :zephyr:code-sample:`lin-ncv7430`
commander sample). Each board powers its own transceiver independently.

The sample can be built and executed for the EK-RA8M1 as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/lin/ncv7430_responder
   :board: ek_ra8m1
   :goals: build flash
   :shield: mikroe_lin_click
   :compact:

Flash the :zephyr:code-sample:`lin-ncv7430` commander sample to the second board and observe on
its console that the LED color it sets is read back correctly, proving this mock responder
answered the read request with the color it was told to set.

Sample Output
=============

.. code-block:: console

   NCV7430 mock responder started
   NCV7430 mock: LED color updated to R=0xFF G=0x00 B=0x00
   NCV7430 mock: LED color updated to R=0x00 G=0xFF B=0x00
   NCV7430 mock: LED color updated to R=0x00 G=0x00 B=0xFF

.. _NCV7430 Datasheet:
   https://www.onsemi.com/pdf/datasheet/ncv7430-d.pdf
