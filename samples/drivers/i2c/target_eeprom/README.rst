.. zephyr:code-sample:: i2c-eeprom-target
   :name: I2C Target
   :relevant-api: i2c_interface

   Setup an I2C target on the I2C interface.

Overview
********

This sample demonstrates how to setup and use the :ref:`i2c-target-api` using the
:dtcompatible:`zephyr,i2c-target-eeprom` device.

Requirements
************

This sample requires an I2C peripheral which is capable of acting as a target.

This sample has been tested on :zephyr:board:`lpcxpresso55s69`.

Building and Running
********************

The code for this sample can be found in :zephyr_file:`samples/drivers/i2c/target_eeprom`.

To build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/i2c/target_eeprom
   :board: lpcxpresso55s69/lpc55s69/cpu0
   :goals: flash
   :compact:
