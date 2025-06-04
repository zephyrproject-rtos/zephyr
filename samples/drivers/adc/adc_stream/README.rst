.. zephyr:code-sample:: adc_stream
   :name: Generic ADC stream
   :relevant-api: adc_interface

   Get data from a ADC using stream.

Overview
********

This sample application demonstrates how to use ADC stream APIs.

Building and Running
********************

This sample supports one ADC. ADC needs to be aliased as ``adc0`` in devicetree.
For example:

.. code-block:: devicetree

 / {
	aliases {
			adc0 = &ad4052;
		};
	};

Make sure the aliase are in devicetree, then build and run with:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/adc/adc_stream
   :board: <board to use>
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

	ADC data for adc405@0 (0.000074) 942995000ns
	ADC data for adc405@0 (0.000446) 963059000ns
	ADC data for adc405@0 (0.000297) 983124000ns
	ADC data for adc405@0 (0.000446) 1003189000ns
