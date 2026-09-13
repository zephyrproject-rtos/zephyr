.. zephyr:code-sample:: ti_vtm
   :name: TI VTM temperature sensor
   :relevant-api: sensor_interface

   Read temperature from a TI VTM sensor and exercise its threshold triggers.

Overview
********

This sample continuously reads the die temperature reported by a TI VTM
(Voltage and Thermal Manager) sensor instance. On startup it also arms all
three threshold triggers (cold, hot, critical) a few degrees away from the
current temperature and registers a handler for each, so that heating up or
cooling down the SoC during the demo will fire the corresponding interrupt.

Building and Running
*********************

Enable a TI VTM sensor child node and add an alias named ``vtm-temp0``
pointing to it.

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
