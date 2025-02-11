.. _adc_cmp_npcx:

NPCX ADC Comparator
###################

Overview
********

This sample show how to use the NPCX ADC Comparator driver. The
sample supports the :ref:`npcx9m6f_evb`.

This application is a voltage comparator with hysteresis, upper limit is
set at 1 V while lower limit is 250 mV. Initially configured to detect
upper limit.

Building and Running
********************

Build the application for the :ref:`npcx9m6f_evb` board, and provide voltage
to ADC channel 8, when voltages cross upper/lower limits, detection messages
will be printed.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/adc_cmp_npcx
   :board: npcx9m6f_evb
   :goals: flash
   :compact:
