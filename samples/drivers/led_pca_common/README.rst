.. _pca_common:

PCA_COMMON: Multiple Channel LED controller
###########################################

Overview
********

This sample controls 4 LEDs connected to a any of `pca_common` supported device, using the
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
any of `pca_common` supported device on the bus I2C Arduino.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led_pca_common
   :board: nucleo_f334r8_board
   :goals: build
   :compact:

For flashing the application, refer to the Flashing section of the
:ref:`nucleo_f334r8_board` board documentation.

References
**********

- PCA9633: https://www.nxp.com/docs/en/data-sheet/PCA9633.pdf
- PCA9634: https://www.nxp.com/docs/en/data-sheet/PCA9634.pdf
- PCA9956: https://www.nxp.com/docs/en/data-sheet/PCA9956A.pdf
- TLC59108: https://www.ti.com/lit/ds/symlink/tlc59108.pdf
