.. zephyr:code-sample:: lz4
   :name: LZ4

   Compress and decompress data using the LZ4 module.

Overview
********

A simple sample that can be used with any :ref:`supported board <boards>` and
compress & decompress the user data to the console.

Building and Running
********************

Add the lz4 module to your West manifest and pull it:

.. code-block:: console

    west config manifest.project-filter -- +lz4
    west update

The sample can be built and executed on nrf52840dk/nrf52840 as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/compression/lz4
   :board: nrf52840dk/nrf52840
   :goals: build flash
   :compact:

To build for another board, change "nrf52840dk/nrf52840" above to that board's name.
