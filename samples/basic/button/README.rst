Button demo
###########

Overview
********

A simple button demo showcasing the use of GPIO input with interrupts.

Requirements
************

The demo assumes that a push button is connected to one of GPIO lines. The
sample code is configured to work on boards with user defined buttons and that
have defined the SW0_* variable in board.h

To use this sample, you will require a board that defines the user switch in its
header file. The :file:`board.h` must define the following variables:

- SW0_GPIO_NAME
- SW0_GPIO_PIN

The following boards currently define the above variables:

- bbc_microbit
- cc3200_launchxl
- frdm_k64f
- nrf51_pca10028
- nrf52840_pca10056
- nrf52_pca10040
- nucleo_f103rb
- :ref:`quark_d2000_devboard`
- quark_se_c1000_devboard
- quark_se_c1000_ss_devboard


Building and Running
********************

This sample can be built for multiple boards, in this example we will build it
for the nucleo_f103rb board:

.. code-block:: console

   $ cd samples/basic/button
   $ make BOARD=nucleo_f103rb


After startup, the program looks up a predefined GPIO device, and configures the
pin in input mode, enabling interrupt generation on falling edge. During each
iteration of the main loop, the state of GPIO line is monitored and printed to
the serial console. When the input button gets pressed, the interrupt handler
will print an information about this event along with its timestamp.
