.. zephyr:code-sample:: ti_vtm
   :name: TI VTM temperature sensor
   :relevant-api: sensor_interface

   Read temperature from a TI VTM sensor and exercise its threshold triggers.

Overview
********

This sample continuously reads the die temperature reported by a TI VTM
(Voltage and Thermal Manager) sensor instance. On startup it also arms all
three threshold triggers (cold, hot, critical) to -5C, +3C and +8C away
from current temperature respectively and registers a handler, so that
heating up or cooling down the SoC during demo will fire the corresponding
interrupt causing the message in the handler to print.

Building and Running
*********************

Enable a TI VTM sensor child node and use the label sensor0 for it.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/ti_vtm
   :board: am62l_evm/am62l3/a53
   :goals: build flash
   :compact:

Sample Output
==============

.. code-block:: console

    VTM temperature: 42.12 C
    Armed threshold at 37.12 C
    Armed threshold at 45.12 C
    Armed threshold at 50.12 C
    VTM temperature: 42.31 C
    VTM temperature: 42.50 C
    [TH1] hot threshold crossed
    VTM temperature: 45.62 C
