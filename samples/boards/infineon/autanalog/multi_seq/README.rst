.. zephyr:code-sample:: autanalog_multi_seq
   :name: AutAnalog ADC Multi-Sequencer
   :relevant-api: adc_interface

   Read multiple AutAnalog ADC inputs configured through a sequencer table.

Overview
********

This sample reads multiple ADC channels defined in devicetree and prints the
raw and millivolt values once per second. The board overlay demonstrates a
high-speed AutAnalog sequencer table that spans GPIO and internal MUX inputs.

Building and Running
********************

The sample requires:

* ADC channels in the ``zephyr,user`` node ``io-channels`` property.
* Sequencer child nodes under the ADC controller.

For :zephyr:board:`kit_pse84_eval`, the ``kit_pse84_eval/pse846gps2dbzc4a/m55``
overlay reads eight GPIO inputs and two MUX-routed inputs described by a
four-entry high-speed sequencer table under the ADC controller.

The sample can be built and flashed as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/infineon/autanalog/multi_seq
   :board: kit_pse84_eval/pse846gps2dbzc4a/m55
   :goals: build flash
   :compact:

Sample output
=============

You should get output similar to the following:

.. code-block:: console

   ADC reading[0]:
   - ADC_0, channel 0: 36 = 65 mV
   - ADC_0, channel 1: 40 = 72 mV
   - ADC_0, channel 2: 38 = 68 mV
   - ADC_0, channel 3: 41 = 74 mV
   - ADC_0, channel 4: 39 = 70 mV
   - ADC_0, channel 5: 37 = 66 mV
   - ADC_0, channel 6: 42 = 76 mV
   - ADC_0, channel 7: 40 = 72 mV
   - ADC_0, channel 8: 36 = 65 mV
   - ADC_0, channel 9: 40 = 72 mV
