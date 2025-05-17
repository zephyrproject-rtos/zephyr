.. zephyr:code-sample:: interrupt-ulp
   :name: Interrupt ULP

   HP Core interrupt LP Core.

Overview
********

This sample shows how to trigger LP Core interrupt from the HP core.

Building and Flashing
*********************

Build the sample code as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/espressif/ulp/lp_core/interrupt_ulp
   :board: esp32c6_devkitc/esp32c6/hpcore
   :west-args: --sysbuild
   :goals: build
   :compact:

Flash it to the device with the command:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/espressif/ulp/lp_core/interrupt_ulp
   :board: esp32c6_devkitc/esp32c6/hpcore
   :west-args: --sysbuild
   :goals: flash
   :compact:

Sample Output
=============

Console output on the HP core:

.. code-block:: console

    Triggering ULP interrupt...
    Triggering ULP interrupt...
    Triggering ULP interrupt...
    Triggering ULP interrupt...
    Triggering ULP interrupt...

Console output on the LP core:

.. code-block:: console

    LP PMU interrupt received: 0
    LP PMU interrupt received: 1
    LP PMU interrupt received: 2
    LP PMU interrupt received: 3
    LP PMU interrupt received: 4
