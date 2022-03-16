.. _ina219:

INA219 Bidirectional Power/Current Monitor
##########################################

Overview
********

This sample application measures shunt voltage, bus voltage, power and current
every 2 seconds and prints them to console.
The calibration/configuration parameters can be set in the devicetree file.

References
**********

 - `INA219 sensor <https://www.ti.com/product/INA219>`_

Wiring
******

The supply voltage of the INA219 can be in the 3V to 5.5V range.
The common mode voltage of the measured bus can be in the 0V to 26V range.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/ina219
   :board: blackpill_f411ce
   :goals: build flash

Sample Output
=============
When monitoring a 3.3 V bus with a 0.1 Ohm shunt resistor
you should get a similar output as below, repeated every 2 seconds:

.. code-block:: console

        Shunt: 0.001570 [V] -- Bus: 3.224000 [V] -- Power: 0.504000 [W] -- Current: 0.157000 [A]


A negative sign indicates current flowing in reverse direction:

.. code-block:: console

        Shunt: -0.001560 [V] -- Bus: 3.224000 [V] -- Power: 0.502000 [W] -- Current: -0.156000 [A]
