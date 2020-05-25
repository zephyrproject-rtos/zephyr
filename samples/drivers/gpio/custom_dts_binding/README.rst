.. _gpio-custom-dts-binding-sample:

GPIO with custom Devicetree binding
###################################

Overview
********

In Zephyr, all hardware-specific configuration is described in the devicetree.

Consequently, also GPIO pins are configured in the devicetree and assigned to
a specific purpose using a compatible.

This is in contrast to other embedded environments like Arduino, where e.g.
the direction (input / output) of a GPIO pin is configured in the application
firmware.

For typical use cases like LEDs or buttons, the existing ``gpio-leds`` or
``gpio-keys`` compatibles can be used.

This sample demonstrates how to use a GPIO pin for other purposes with a
custom dts binding.

We assume that a load with high current demands should be switched on or off
via a MOSFET. The custom DTS binding for the power output controlled via a
GPIO pin is specified in the file ``dts/bindings/power-output.yaml``. The gate
driver for the MOSFET would be connected to the pin as specified in the
``.overlay`` file in the boards folder.

Building and Running
********************

For each board that should be supported, a ``.overlay`` file has to be defined
in the ``boards`` subfolder.

Building and Running for ST Nucleo L073RZ
=========================================
The sample can be built and executed for the
:ref:`nucleo_l073rz_board` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/gpio/custom_dts_binding
   :board: nucleo_l073rz
   :goals: build flash
   :compact:

For demonstration purposes, we use the GPIO pin of the built-in LED.

To build for another board, change "nucleo_l073rz" above to that board's name.

Sample output
=============

The GPIO pin should be switched to active level after one second.

The following output is printed (pin number and port may differ):

.. code-block:: console

   Initializing pin 5 on port GPIOA with inactive level.
   Waiting one second.
   Setting pin to active level.
