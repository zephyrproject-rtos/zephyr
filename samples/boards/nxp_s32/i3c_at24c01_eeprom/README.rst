.. nxp_s32_i3c_eeprom

NXP_S32 I3C EEPROM
##################

Overview
********
This is a sample app to read and write the Microchip AT24C01C-XHM EEPROM chip via
I3C driver.

This assumes the slave address of EEPROM is 0x50.

Building and Running
********************

This project can be built and executed on as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp_s32/i3c_at24c01_eeprom
   :board: s32z270dc2_rtu0_r52
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   Wrote 0x1 to address 0x10
   Read 0x1 from address 0x10
   Wrote 20 bytes to address starting from 0x10
   Read 20 bytes from starting address 0x10
   EEPROM read/write successful
