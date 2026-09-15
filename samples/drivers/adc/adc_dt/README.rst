.. zephyr:code-sample:: adc_dt
   :name: Analog-to-Digital Converter (ADC) with devicetree
   :relevant-api: adc_interface

   Read analog inputs from ADC channels.

Overview
********

This sample demonstrates how to use the :ref:`ADC driver API <adc_api>`.

Depending on the target board, it reads ADC samples from one or more channels
and prints the readings on the console. If voltage of the used reference can
be obtained, the raw readings are converted to millivolts.

The pins of the ADC channels are board-specific. Please refer to the board
or MCU datasheet for further details.

Building and Running
********************

The sample reads the channels listed in the ``io-channels`` property of the
``zephyr,user`` node when an overlay provides one, and otherwise every channel
node of an enabled ADC controller. The channel nodes carry the configuration of
each channel (settings like gain, reference, acquisition time, resolution and
oversampling), as in this overlay:

.. code-block:: devicetree

   / {
       zephyr,user {
           io-channels = <&adc1 0>, <&adc1 1>;
       };
   };

   &adc1 {
       #address-cells = <1>;
       #size-cells = <0>;

       channel@0 {
           reg = <0>;
           zephyr,gain = "ADC_GAIN_1";
           zephyr,reference = "ADC_REF_INTERNAL";
           zephyr,acquisition-time = <ADC_ACQ_TIME_DEFAULT>;
           zephyr,resolution = <12>;
       };

       channel@1 {
           reg = <1>;
           zephyr,gain = "ADC_GAIN_1";
           zephyr,reference = "ADC_REF_INTERNAL";
           zephyr,acquisition-time = <ADC_ACQ_TIME_DEFAULT>;
           zephyr,resolution = <12>;
       };
   };

Boards that describe the channels available for testing in the
:ref:`snippet-hw-test` snippet need no overlay: build with the snippet to read
those channels.

Building and Running for ST Nucleo L073RZ
=========================================

The sample can be built and executed for the
:zephyr:board:`nucleo_l073rz` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/adc/adc_dt
   :board: nucleo_l073rz
   :snippets: hw-test
   :goals: build flash
   :compact:

To build for another board, change "nucleo_l073rz" above to that board's name,
and provide a devicetree overlay if the board does not describe its channels in
the snippet.

Sample output
=============

You should get a similar output as below, repeated every second:

.. code-block:: console

   ADC reading:
   - ADC_0, channel 7: 36 = 65mV

.. note:: If the ADC is not supported, the output will be an error message.
