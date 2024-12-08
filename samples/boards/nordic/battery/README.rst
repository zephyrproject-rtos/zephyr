.. zephyr:code-sample:: nrf_battery
   :name: Battery Voltage Measurement

   Measure the voltage of the device power supply.

Overview
********

This sample demonstrates using Nordic configurations of the Zephyr ADC
infrastructure to measure the voltage of the device power supply.  Two
power supply configurations are supported:

* If the board devicetree has a ``/vbatt`` node with compatible
  ``voltage-divider`` then the voltage is measured using that divider. An
  example of a devicetree node describing a voltage divider for battery
  monitoring is:

   .. code-block:: devicetree

      / {
         vbatt {
            compatible = "voltage-divider";
            io-channels = <&adc 4>;
            output-ohms = <180000>;
            full-ohms = <(1500000 + 180000)>;
            power-gpios = <&sx1509b 4 0>;
         };
      };

* If the board does not have a voltage divider and so no ``/vbatt`` node
  present, the ADC configuration (device and channel) needs to be provided via
  the ``zephyr,user`` node. In this case the power source is assumed to be
  directly connected to the digital voltage signal, and the internal source for
  ``Vdd`` is selected. An example of a Devicetree configuration for this case is
  shown below:

   .. code-block :: devicetree

      / {
         zephyr,user {
            io-channels = <&adc 4>;
         };
      };

Note that in many cases where there is no voltage divider the digital
voltage will be fed from a regulator that provides a fixed voltage
regardless of source voltage, rather than by the source directly. In
configuration involving a regulator the measured voltage will be
constant.

The sample provides discharge curves that map from a measured voltage to
an estimate of remaining capacity.  The correct curve depends on the
battery source: a standard LiPo cell requires a curve that is different
from a standard alkaline battery.  Curves can be measured, or estimated
using the data sheet for the battery.

Application Details
===================

The application initializes battery measurement on startup, then loops
displaying the battery status every five seconds.

Requirements
************

A Nordic-based board, optionally with a voltage divider specified in its
devicetree configuration as noted above.

Building and Running
********************

The code can be found in :zephyr_file:`samples/boards/nordic/battery`.

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nordic/battery
   :board: thingy52/nrf52832
   :goals: build flash
   :compact:


Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v2.2.0-318-g84219bdc1ac2  ***
   [0:00:00.016]: 4078 mV; 10000 pptt
   [0:00:04.999]: 4078 mV; 10000 pptt
   [0:00:09.970]: 4078 mV; 10000 pptt
   [0:00:14.939]: 4069 mV; 10000 pptt
   [0:00:19.910]: 4078 mV; 10000 pptt
   [0:00:24.880]: 4069 mV; 10000 pptt
