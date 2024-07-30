.. zephyr:code-sample:: mspi-flash
   :name: JEDEC MSPI-NOR flash
   :relevant-api: flash_interface

   Use the flash API to interact with a MSPI NOR serial flash memory device.

Overview
********

This sample demonstrates using the :ref:`flash API <flash_api>` on a MSPI NOR serial flash
memory device.  While trivial it is an example of direct access and
allows confirmation that the flash is working and that automatic power
savings is correctly implemented.

Building and Running
********************

The application will build only for a target that has a :ref:`devicetree <dt-guide>`
``flash0`` alias that refers to an entry with the following bindings as a compatible:

* :dtcompatible:`ambiq,mspi-device`, `mspi-atxp032`

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/mspi/mspi_flash
   :board: apollo3p_evb
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v2.3.0-2142-gca01d2e1d748  ***

   JEDEC MSPI-NOR flash testing
   ==========================

   Test 1: Flash erase
   Flash erase succeeded!

   Test 2: Flash write
   Attempting to write 4 bytes
   Data read matches data written. Good!
