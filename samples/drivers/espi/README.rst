.. _espi-sample:

Enhanced Serial Peripheral Interface
####################################

Overview
********

This sample demonstrates how to use the Enhanced Serial Peripheral Interface
(eSPI) API.
It shows how to configure and select eSPI controller capabilities as part of
a simple eSPI handshake that includes exchanging virtual wire packets.

Standard platform signals are sent virtual wire packets over the bus.
Callbacks are registered that will write to the console indicating main
eSPI events and when a virtual wire is received.

Building and Running
********************

The sample can be built and executed on boards supporting eSPI.
Any pins required for minimum eSPI handshake should be configured.

Sample output
=============

.. code-block:: console

   Hello eSPI test!
   eSPI test - I/O initialization...complete
   eSPI slave configured successfully!
   eSPI test - callbacks initialization... complete
   eSPI test - callbacks registration... complete
   eSPI test - Power initialization...complete
   eSPI test - Handshake
   eSPI BUS reset 0
   VW channel is ready

   PLT_RST changed 1
        1st phase completed
        2nd phase completed
        3rd phase completed

note:: The values shown above might differ.
