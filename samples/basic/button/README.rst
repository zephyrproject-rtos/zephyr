.. _button-sample:

Button demo
###########

Overview
********

A simple button demo showcasing the use of GPIO input with interrupts.

Requirements
************

The demo assumes that a push button is connected to one of GPIO lines. The
sample code is configured to work on boards with user defined buttons and that
have defined the SW0_* variables.

To use this sample, you will require a board that defines the user switch in its
header file. The :file:`board.h` must define the following variables:

- SW0_GPIO_NAME (or DT_ALIAS_SW0_GPIOS_CONTROLLER)
- DT_ALIAS_SW0_GPIOS_PIN

Alternatively, this could also be done by defining 'sw0' alias in the board
devicetree description file.


Building and Running
********************

This sample can be built for multiple boards, in this example we will build it
for the nucleo_f103rb board:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/button
   :board: nucleo_f103rb
   :goals: build
   :compact:

After startup, the program looks up a predefined GPIO device, and configures the
pin in input mode, enabling interrupt generation on falling edge. During each
iteration of the main loop, the state of GPIO line is monitored and printed to
the serial console. When the input button gets pressed, the interrupt handler
will print an information about this event along with its timestamp.
