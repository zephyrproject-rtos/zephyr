.. zephyr:code-sample:: hello-ulp
   :name:

   Sample to demonstrate how to use LP UART with LP Core in ESP32C6.

Overview
********

This is a simple sample that prints 'Hello World' running from LP Core using LP UART peripheral.


Building and Flashing
*********************

Build the sample code as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/espressif/ulp/lp_core/hello_ulp
   :board: esp32c6_devkitc/esp32c6/hpcore
   :west-args: --sysbuild
   :goals: build
   :compact:

Flash it to the device with the command:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/espressif/ulp/lp_core/hello_ulp
   :board: esp32c6_devkitc/esp32c6/hpcore
   :west-args: --sysbuild
   :goals: flash
   :compact:

Sample Output
=============

.. code-block:: console

    Hello World! esp32c6_devkitc/esp32c6/lpcore
