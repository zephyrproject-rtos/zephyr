.. zephyr:code-sample:: adc_sequence
   :name: Analog-to-Digital Converter (ADC) sequence sample
   :relevant-api: adc_interface

   Read analog inputs from ADC channels, using a sequence.

Overview
********

This sample demonstrates how to use the :ref:`ADC driver API <adc_api>` using sequences.

Depending on the target board, it reads ADC samples from two channels
and prints the readings on the console, based on the sequence specifications.
Notice how for the whole sequence reading, only one call to the :c:func:`adc_read` API is made.
If voltage of the used reference can be obtained, the raw readings are converted to millivolts.

This example constructs an adc device and setups its channels, according to the
given devicetree configuration.

Building and Running
********************

Make sure that the ADC is enabled (``status = "okay";``) and has each channel as a
child node, with your desired settings like gain, reference, or acquisition time and
oversampling setting (if used). It is also needed to provide an alias ``adc0`` for the
desired adc. See :zephyr_file:`boards/nrf52840dk_nrf52840.overlay
<samples/drivers/adc/adc_dt/boards/nrf52840dk_nrf52840.overlay>` for an example of
such setup.

Building and Running for Nordic nRF52840
========================================

The sample can be built and executed for the
:zephyr:board:`nrf52840dk` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/adc/adc_sequence
   :board: nrf52840dk/nrf52840
   :goals: build flash
   :compact:

To build for another board, change "nrf52840dk/nrf52840" above to that board's name
and provide a corresponding devicetree overlay.

Sample output
=============

You should get a similar output as below, repeated every second:

.. code-block:: console

   ADC sequence reading [1]:
   - ADC_0, channel 0, 5 sequence samples:
   - - 36 = 65mV
   - - 35 = 63mV
   - - 36 = 65mV
   - - 35 = 63mV
   - - 36 = 65mV
   - ADC_0, channel 1, 5 sequence samples:
   - - 0 = 0mV
   - - 0 = 0mV
   - - 1 = 1mV
   - - 0 = 0mV
   - - 1 = 1mV

.. note:: If the ADC is not supported, the output will be an error message.
