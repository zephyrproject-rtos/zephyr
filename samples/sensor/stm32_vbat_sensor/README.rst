.. _stm32_vbat_sensor:

STM32 VBat Sensor
#################

Overview
********

This sample reads the Vbat from the STM32 Internal
Sensor and displays the results.

Building and Running
********************

In order to run this sample, make sure to enable ``stm32_vbat`` node in your
board DT file or with a board overlay in the samples/sensor/stm32_temp_sensor/boards :


.. code-block:: dts

    stm32_vbat: stm32vbat {
        compatible = "st,stm32-vbat";
        label = "VBAT";
        io-channels = <&adc1 14>;
        ratio = <3>;
        status = "okay";
    };


Enable the corresponding ADC, with the correct vref value (in mV)

.. code-block:: dts

    &adc1 {
	vref-mv = <3000>;
	status = "okay";
    };


.. zephyr-app-commands::
   :zephyr-app: samples/sensor/stm32_vbat_sensor
   :board: nucleo_g071rb
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

Current Vbat voltage: 3.04 V
