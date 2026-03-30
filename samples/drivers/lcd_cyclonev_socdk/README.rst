.. zephyr:code-sample:: lcd-cyclonev-socdk
   :name: LCD Cyclone V SoC DevKit
   :relevant-api: i2c_interface

   Display text on an I2C LCD connected to the Cyclone V SoC FPGA DevKit.

Overview
********

This sample demonstrates how to control a Newhaven NHD-0216K3Z 2x16 alphanumeric
LCD display over I2C on the Cyclone V SoC FPGA DevKit. The application
verifies the I2C bus readiness, sends a single ASCII character to the first
line of the display, then writes a string to the second line using the
display's command set.

Requirements
************

- Cyclone V SoC FPGA DevKit

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/lcd_cyclonev_socdk
   :board: cyclonev_socdk
   :goals: build flash
   :compact:

Sample Output
*************

.. code-block:: console

    Hello World! cyclonev_socdk
    i2c is ready
    Success!
    Success!
    Success!

Display Output
**************

.. code-block:: none

    A
    Hello world!

References
**********

- Newhaven NHD-0216K3Z datasheet
