.. zephyr:code-sample:: pca9633
   :name: PCA9633 LED
   :relevant-api: led_interface

   Control 4 LEDs connected to a PCA9633 driver chip.

Overview
********

This sample controls 4 LEDs connected to a PCA9633 driver, using the
following pattern:

 1. turn on LEDs
 2. turn off LEDs
 3. set the brightness to 50%
 4. turn off LEDs
 5. blink the LEDs
 6. turn off LEDs

Building and Running
********************

Build the application for the :ref:`nucleo_f334r8_board` board, and connect
a PCA9633 LED driver on the bus I2C Arduino.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led_pca9633
   :board: nucleo_f334r8_board
   :goals: build
   :compact:

For flashing the application, refer to the Flashing section of the
:ref:`nucleo_f334r8_board` board documentation.

References
**********

- PCA9633: https://www.nxp.com/docs/en/data-sheet/PCA9633.pdf
