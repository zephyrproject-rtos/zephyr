.. _dac-sample:

Digital-to-Analog Converter (DAC)
#################################

Overview
********

This sample demonstrates how to use the DAC driver API.

Building and Running
********************

The DAC output is defined in the board's devicetree and pinmux file.

Only board nucleo_l073rz is supported for now.

Sample output
=============

You should see a sawtooth signal with an amplitude of the DAC reference
voltage and a period of approx. 4 seconds at the DAC output pin specified
by the board.

The following output is printed:

.. code-block:: console

   DAC internal reference voltage: 3300 mV
   Generating sawtooth signal at DAC channel 1.

.. note:: If the DAC is not supported, the output will be an error message.
