.. zephyr:code-sample:: echo-ulp
   :name: Echo ULP

   Leverage Zephyr's UART API to use the LP UART on the ESP32-C6's LP core.

Overview
********

This sample application demonstrates how to use poll-based APIs from the Zephyr
UART driver subsystem. It reads characters from the LP UART using
:c:func:`uart_poll_in` and echoes them back using :c:func:`uart_poll_out`.

Building and Flashing
*********************

Build the sample code as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/espressif/ulp/lp_core/echo_ulp
   :board: esp32c6_devkitc/esp32c6/hpcore
   :west-args: --sysbuild
   :goals: build
   :compact:

Flash it to the device with the command:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/espressif/ulp/lp_core/echo_ulp
   :board: esp32c6_devkitc/esp32c6/hpcore
   :west-args: --sysbuild
   :goals: flash
   :compact:

Sample Output
=============

.. code-block:: console

    UART echo example started. Type something...
