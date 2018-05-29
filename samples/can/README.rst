.. _can-sample:

Controller Area Network
#######################

Overview
********

This sample demonstrates how to use the Controller Area Network (CAN) API.
Messages with standard and extended identifiers are sent over the bus, triggered
by a button event.
Messages are received using message queues and ISRs.
Reception is indicated by blink LEDs and output to the console.

Building and Running
********************

In loopback mode, the board receives its own messages. This could be used for
standalone testing.

The sample can be built and executed on boards supporting CAN.
The output ports and pins of the LEDs can be configured by Kconfig.

Sample output
=============

.. code-block:: console

   Finished init. waiting for Interrupts
   TX thread is running.
   filter id: 1
   Button pressed! Send message 1
   Button pressed 1 times
   Button pressed! Send message 0
   Button pressed 2 times
   String sent over CAN
   Button pressed! Send message 1
   Button pressed 3 times
   Button pressed! Send message 0
   Button pressed 4 times
   String sent over CAN

.. note:: The values shown above might differ.
