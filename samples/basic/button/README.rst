.. zephyr:code-sample:: button
   :name: Button
   :relevant-api: input_interface

   Handle GPIO button inputs using the input subsystem.

Overview
********

A simple button demo showcasing the use of buttons with the :ref:`input` APIs.
The sample prints a message to the console each time a button is pressed.

Requirements
************

The board hardware must have a device node capable of generating input KEY
events, typically a push button connected via a GPIO pin and defined in a
"gpio-keys" node. These are called "User buttons" on many of Zephyr's
:ref:`boards`.

The sample additionally supports an optional ``led0`` devicetree alias. If this
is provided, the LED will be turned on when the button is pressed, and turned
off when it is released.

Building and Running
********************

This sample can be built for multiple boards, in this example we will build it
for the nucleo_f103rb board:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/button
   :board: nucleo_f103rb
   :goals: build
   :compact:
