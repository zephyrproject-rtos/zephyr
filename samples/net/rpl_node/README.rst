.. _rpl-node-sample:

RPL node
###########

Overview
********

This sample builds a simple RPL node and shows how to join into an RPL
mesh network.

This sample assumes that your chosen platform has networking support.
Some code configuration adjustments may be needed.

The sample will listen for RPL multicast messages and joins with the RPL
Border Router node in DAG network.

The sample exports the following resources through a CoAP server role:

.. code-block:: none

   /led
   /rpl-obs

These resources allow you to toggle an on-board LED (if available) and build
the RPL mesh network topology from node RPL information.

Building And Running
********************

Running BR
==========

Follow the instructions from :ref:`rpl-border-router-sample` to run Zephyr
RPL border router.

Running RPL node
================

To build and run RPL node, follow the below steps to build and install
it on IEEE 802.15.4 radio supported board.

.. zephyr-app-commands::
   :zephyr-app: samples/net/rpl-node
   :board: <board to use>
   :conf: <config file to use>
   :goals: build flash
   :compact:

Wait until the RPL node joins with Border-Router and updates the list in the web UI.
