.. zephyr:code-sample:: soc_voltage
   :name: SoC Voltage Sensor
   :relevant-api: sensor_interface

   Get voltage data from an SoC's voltage sensor(s).

Overview
********

This sample reads one or more of the various voltage sensors an SoC might have and
displays the results.

On STM32 boards with an okay ``st,stm32-vref`` node and
``CONFIG_ADC_STM32_VREFINT_CALIBRATE=y``, the voltage reported here should
agree with :c:func:`adc_ref_internal` on the VREFINT-owning ADC. Prefer the
ADC API for ``adc_raw_to_millivolts_dt()``; use this sensor sample for
explicit voltage channels (VREF+/VBAT).

Building and Running
********************

In order to run this sample, enable the sensor node that supports
``SENSOR_CHAN_VOLTAGE`` and create an alias named ``volt-sensor0`` to link to the node.
The tail ``0`` is the sensor number.  This sample support up to 16 sensors.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/soc_voltage
   :board: nucleo_g071rb
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

   Sensor voltage: 2.99 V
