.. _i2c_fujitsu_fram:

I2C Fujitsu FRAM
################

Overview
********
This is a sample app to read and write the Fujitsu MB85RC256V FRAM chip via I2C
on the Quark SE Sensor Subsystem.

This assumes the slave address of FRAM is 0x50, where A0, A1, and A2 are all
tied to ground.

Building and Running
********************

This project can be built and executed on as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/i2c_fujitsu_fram
   :host-os: unix
   :board: quark_se_c1000_devboard
   :goals: run
   :compact:


Sample Output
=============

.. code-block:: console

    Wrote 0xAE to address 0x00.
    Wrote 0x86 to address 0x01.
    Read 0xAE from address 0x00.
    Read 0x86 from address 0x01.
    Wrote 16 bytes to address 0x00.
    Read 16 bytes from address 0x00.
    Data comparison successful.
