.. zephyr:code-sample:: mspi-async
   :name: MSPI asynchronous transfer
   :relevant-api: mspi_interface

   Use the MSPI API to interact with MSPI memory device asynchronously.

Overview
********

This sample demonstrates using the :ref:`MSPI API <mspi_api>` on a MSPI
memory device.  The asynchronous transceive call need to be supported
either by a software queue or hardware queue from the controller hardware.
To this sample, however, the implementation should make no difference.

Building and Running
********************

The application will build only for a target that has a :ref:`devicetree <dt-guide>`
``dev0`` alias that refers to an entry with the following bindings as a compatible:

* :dtcompatible:`ambiq,mspi-device`, `mspi-aps6404l`

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/mspi/mspi_async
   :board: apollo3p_evb
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v3.5.0-8581-gc80b243c7598 ***
   w:3,r:3
   Read data matches written data
