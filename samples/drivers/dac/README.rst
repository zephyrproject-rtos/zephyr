.. zephyr:code-sample:: dac
   :name: Digital-to-Analog Converter (DAC)
   :relevant-api: dac_interface

   Generate an analog sawtooth signal using the DAC driver API.

Overview
********

This sample demonstrates how to use the :ref:`DAC driver API <dac_api>`.

Building and Running
********************

The DAC output is defined in the board's devicetree and pinmux file.

The board's :ref:`/zephyr,user <dt-zephyr-user>` node must have ``dac``,
``dac-channel-id``, and ``dac-resolution`` properties set. See the predefined
overlays in :zephyr_file:`samples/drivers/dac/boards` for examples.

Building and Running
====================
The sample can be built and executed as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dac
   :board: <board-name>
   :goals: build flash
   :compact:

Sample output
=============

You should see a sawtooth signal with an amplitude of the DAC reference
voltage and a period of approx. 4 seconds at the DAC output pin specified
by the board.

The following output is printed:

.. code-block:: console

   Generating sawtooth signal at DAC channel 1.

.. note:: If the DAC is not supported, the output will be an error message.
