.. zephyr:code-sample:: espi
   :name: Enhanced Serial Peripheral Interface (eSPI)
   :relevant-api: espi_interface spi_interface

   Use eSPI to connect to a slave device and exchange virtual wire packets.

Overview
********

This sample demonstrates how to use the :ref:`Enhanced Serial Peripheral Interface
(eSPI) API <espi_api>`.
It shows how to configure and select eSPI controller capabilities as part of
a simple eSPI handshake that includes exchanging virtual wire packets.

Standard platform signals are sent virtual wire packets over the bus.
Callbacks are registered that will write to the console indicating main
eSPI events and when a virtual wire is received.

Building and Running
********************

The sample can be built and executed on boards supporting eSPI.
Any pins required for minimum eSPI handshake should be configured.
Sample requires a correct harness and fixture setup.
Please connect an eSPI device to the testing board.
After that for the correct execution of that sample in twister, add into
boards's map-file next fixture settings::

      - fixture: espi_device_connect

Sample output
=============

.. code-block:: console

   Hello eSPI test <BOARD USED>
   eSPI target configured successfully!
   eSPI test - callbacks initialization... complete
   eSPI test - callbacks registration... complete

   eSPI BUS reset 1

   eSPI test - Handshake with eSPI controller (host)...
   VW channel event 1
   OOB channel event 1

   1st phase completed
   SLP signal changed 1
   SLP signal changed 1
   SLP signal changed 1

   2nd phase completed
   PLT_RST changed 1

   3rd phase completed

   request_temp
   OOB transaction completed rcvd: 11 bytes
   request_temp
   OOB transaction completed rcvd: 5 bytes
   eSPI sample completed err: 0


note:: The values shown above might differ depending on the subtests executed.
