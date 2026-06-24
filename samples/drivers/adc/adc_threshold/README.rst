.. zephyr:code-sample:: adc_threshold
   :name: Analog-to-Digital Converter (ADC) threshold sample
   :relevant-api: adc_interface

   Read analog inputs from ADC channels.

Overview
********

This sample demonstrates how to use the ADC Threshold API.

Depending on the target board, it configure hardware to trigger an interrupt when a channel
reading exceed 500 millivolts and execute a callback function on this interrupt.

While this sample use asynchronous conversions, this API can also be used with synchronous
conversions and stream requests.

The pins of the ADC channels are board-specific. Please refer to the board
or MCU datasheet for further details.

Building and Running
********************

To build the sample, you need an ADC peripheral supporting the ADC Threshold API and the ADC Async
API under the alias ``adc0`` enabled (``status = "okay";``). To run the sample, at least one channel
sould be provided as a child of the ADC peripheral.
See :zephyr_file:`boards/nrf52dk_nrf52832.overlay
<samples/drivers/adc/adc_threshold/boards/nrf52dk_nrf52832.overlay>` for an example of such setup.

You can add a ``boards/<board>_<soc>.conf`` file to change configuration of the sample.
Two Kconfig are provided to configure the sample :

* ``CONFIG_THRESHOLD_RESOLUTION`` to set common resolution for all channels (default 12 bits). Use
    it if the ADC does not support 12 bits resolution.
* ``CONFIG_THRESHOLD_UPPER_VALUE_MV`` to set the upper threshold in millivolts (mv). Use it if the
    ADC can not convert a value of 500 mv.

Each used channels should have a commun reference voltage, resolution and gain to ensure that the
raw ADC value translating ``CONFIG_THRESHOLD_UPPER_VALUE_MV`` is common to all channels.

Building and Running for Nordic nRF52dk
========================================

The sample can be built and executed for the
:zephyr:board:`nrf52dk` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/adc/adc_threshold
   :board: nrf52dk/nrf52832
   :goals: build flash
   :compact:

To build for another board, change "nrf52dk/nrf52832" above to that board's name
and provide a corresponding devicetree overlay.

Sample output
=============

If the configuration succeed, you should get this output :

.. code-block:: console

   Starting asynchronous reads, waiting for threshold events

.. note:: If the ADC is not supported, the output will be an error message.

Once you wire the ADC input pin to a voltage source higher than 500 mv, you should get a similar
output, repeated every seconde until the pin is connect to a voltage source lower than 500 mv :

.. code-block:: console

   Threshold crossed on channel 0 with value 3370

.. note:: Channel number and conversion result depends on your hardware configuration and input
   voltage. Make sure to never connect the input pin to a voltage source higher than the supported
   voltage for your ADC peripheral.
