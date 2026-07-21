.. zephyr:code-sample:: autanalog_fir_fifo
   :name: AutAnalog ADC FIR+FIFO
   :relevant-api: adc_interface

   Read AutAnalog ADC channels with FIR filtering and FIFO bulk collection.

Overview
********

This sample demonstrates the AutAnalog SAR ADC FIR and FIFO features:

* Four ADC channels are sampled each second: a raw GPIO input (P15.0),
  the same input via the internal MUX, and two FIR filter pseudo-channels.
* FIR0 is a 4-tap moving average configured in devicetree (four
  ``0x2000`` coefficients in Q1.15 fixed-point).
* FIR1 is a 4-tap moving average loaded at runtime by the application.
* Each channel routes its results to a dedicated FIFO buffer. When the FIFO
  watermark level is reached the driver fires a callback and the application
  drains all four buffers in bulk.

Building and Running
********************

The sample requires:

* ADC channels in the ``zephyr,user`` node ``io-channels`` property.
* Two FIR child nodes under the ADC controller.
* A ``fifo`` child node under the ADC controller.

For :zephyr:board:`kit_pse84_eval`, the ``kit_pse84_eval/pse846gps2dbzc4a/m55``
overlay configures:

* Channel 0  - raw GPIO input P15.0, routed to FIFO buffer 0.
* Channel 8  - P15.0 via MUX, routed to FIFO buffer 1.
* Channel 24 - FIR0 pseudo-channel, routed to FIFO buffer 2.
* Channel 25 - FIR1 pseudo-channel, routed to FIFO buffer 3.
* FIR0 coefficients loaded from devicetree; FIR1 coefficients loaded at runtime.
* FIFO watermark level set to 4 words per buffer.

The sample can be built and flashed as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/infineon/autanalog/fir_fifo
   :board: kit_pse84_eval/pse846gps2dbzc4a/m55
   :goals: build flash
   :compact:

Sample output
=============

You should get output similar to the following:

.. code-block:: console

   ADC reading[0]:
   - adc0, channel 0: 36 = 65 mV
   - adc0, channel 8: 36 = 65 mV
   - adc0, channel 24: 36 = 65 mV
   - adc0, channel 25: 36 = 65 mV
   FIFO: watermark event (status=0x0000000f)
   FIFO[0] GPIO: 4 words - 36 36 36 36
   FIFO[1] MUX : 4 words - 36 36 36 36
   FIFO[2] FIR0: 4 words - 36 36 36 36
   FIFO[3] FIR1: 4 words - 36 36 36 36
