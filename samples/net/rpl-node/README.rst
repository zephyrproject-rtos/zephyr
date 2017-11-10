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
   /ipv6/neighbors
   /rpl-info
   /rpl-info/parent
   /rpl-info/rank
   /rpl-info/link-metric

These resources allow you to toggle an on-board LED (if available) and build
the RPL mesh network topology from node RPL information.

Building And Running
********************

If you're using a Sparrow border router, follow the steps below to build and
run Sparrow BR.  (Sparrow has its own TLV mechanism to build topology that
Zephyr doesn't support.)  A patch is provided in the sample folder to to support
building topology with CoAP-based responses.

Running Sparrow BR
==================

.. code-block:: console

   git clone https://github.com/sics-iot/sparrow.git
   cd sparrow
   git am 0001-Added-CoAP-support-for-Sparrow-Border-Router.patch
   cd products/sparrow-border-router
   sudo make connect-high PORT=/dev/ttyACM0

If your PC is using an http proxy, you should unset it for this sample.
Wait until the border router is up and running. The python script used below
will run a web-based UI.

.. code-block:: console

   cd examples/sparrow
   ./wsdemoserver.py

Wait until you see "Connected" message on console. Unset proxy in browser
and open 127.0.0.1:8000.

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
