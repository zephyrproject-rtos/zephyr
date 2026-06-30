.. zephyr:code-sample:: adc_stream
   :name: Generic ADC stream
   :relevant-api: adc_interface

   Get data from a ADC using stream.

Overview
********

This sample demonstrates the use of the ADC streaming API.

The sample supports two conversion modes:

* Standard ADC streaming, where conversions are initiated by the ADC driver.
* External PWM-triggered ADC streaming, where conversions are initiated by an external PWM signal.

The externall PWM mode is enabled when the ADC DeviceTree node contains a ``pwms`` property.
If no PWM configuration is provided, the sample uses the standard streaming behavior.

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

Make sure the aliases are in devicetree, then build and run with:

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
