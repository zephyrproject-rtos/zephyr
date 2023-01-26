.. _gpio-custom-dts-binding-sample:

GPIO with custom devicetree binding
###################################

Overview
********

In Zephyr, all hardware-specific configuration is described in the devicetree.

Consequently, also GPIO pins are configured in the devicetree and assigned to a specific purpose
using a compatible.

This is in contrast to other embedded environments like Arduino, where e.g. the direction (input /
output) of a GPIO pin is configured in the application firmware.

For typical use cases like LEDs or buttons, the existing :dtcompatible:`gpio-leds` or
:dtcompatible:`gpio-keys` compatibles can be used.

This sample demonstrates how to use a GPIO pin for other purposes with a custom devicetree binding.

We assume that a load with high current demands should be switched on or off via a MOSFET. The
custom devicetree binding for the power output controlled via a GPIO pin is specified in the file
:zephyr_file:`samples/basic/custom_dts_binding/dts/bindings/power-switch.yaml`. The gate driver for
the MOSFET would be connected to the pin as specified in the ``.overlay`` file in the boards
folder.

Building and Running
********************

For each board that should be supported, a ``.overlay`` file has to be defined
in the ``boards`` subfolder.

Afterwards, the sample can be built and executed for the ``<board>`` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/custom_dts_binding
   :board: <board>
   :goals: build flash
   :compact:

For demonstration purposes, some boards use the GPIO pin of the built-in LED.

Sample output
=============

The GPIO pin should be switched to active level after one second.

The following output is printed:

.. code-block:: console

   Initializing pin with inactive level.
   Waiting one second.
   Setting pin to active level.
