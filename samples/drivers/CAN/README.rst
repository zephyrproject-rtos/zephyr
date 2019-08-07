.. _can-sample:

Controller Area Network
#######################

Overview
********

This sample demonstrates how to use the Controller Area Network (CAN) API.
Messages with standard and extended identifiers are sent over the bus.
Messages are received using message-queues and work-queues.
Reception is indicated by blinking the LED (if present) and output to the console.

Building and Running
********************

In loopback mode, the board receives its own messages. This could be used for
standalone testing.

The sample can be built and executed on boards supporting CAN.
The LED output pin is defined in the board's device tre.

Sample output
=============

.. code-block:: console

   Change LED filter ID: 0
   Finished init.
   Counter filter id: 4

   uart:~$ Counter received: 0
   Counter received: 1
   Counter received: 2
   Counter received: 3

.. note:: The values shown above might differ.
