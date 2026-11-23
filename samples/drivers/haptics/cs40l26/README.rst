.. zephyr:code-sample:: cs40l26
   :name: CS40L26 Haptic Driver
   :relevant-api: haptics_interface

   Drive an LRA using the CS40L26/27 haptic driver chip.

Overview
********

CS40L26/27 is a family of haptics drivers designed for mobile applications.

This sample demonstrates ROM features available on CS40L26/27 haptics drivers, including
ROM and buzz effects. A custom shell interface is provided that exposes the CS40L26/27 API;
a basic demonstration is provided if the custom shell interface is not used.

Building and Running
********************

Build the application for the :zephyr:board:`nucleo_f401re` board, and connect a CS40L26/27 haptic driver
on the bus I2C1 at the address 0x40.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/haptics/cs40l26
   :board: nucleo_f401re
   :goals: build
   :compact:

For flashing the application, refer to the Flashing sections of the :zephyr:board:`nucleo_f401re`
board documentation.
