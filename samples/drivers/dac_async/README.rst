.. _dac-sample:

Digital-to-Analog Converter (DAC)
#################################

Overview
********

This sample demonstrates how to use the DAC driver continuous API.

Building and Running
********************

The DAC output is defined in the board's devicetree and pinmux file.

Building and Running for ST Nucleo L4R5ZI
=========================================
The sample can be built and executed for the
:ref:`nucleo_l4r5_board` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dac_async
   :board: nucleo_l4r5zi
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
