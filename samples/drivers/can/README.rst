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

The LED output pin is defined in the board's devicetree.

The sample can be built and executed for boards with a SoC that have an
integrated CAN controller or for boards with a SoC that has been augmented
with a stand alone CAN controller.

Integrated CAN controller
=========================

For the NXP TWR-KE18F board:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/CAN
   :board: twr_ke18f
   :goals: build flash

Stand alone CAN controller
==========================

For the nrf52dk_nrf52832 board combined with the DFRobot CAN bus V2.0 shield that
provides the MCP2515 CAN controller:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/CAN
   :board: nrf52dk_nrf52832
   :shield: dfrobot_can_bus_v2_0
   :conf: prj.mcp2515.conf
   :goals: build flash

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
