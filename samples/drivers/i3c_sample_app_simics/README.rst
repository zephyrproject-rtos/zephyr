.. _i3c_sample_app_simics:

I3C SAMPLE APP FOR SIMICS
################

Overview
********
This is a sample app to read and write the i3c simulated slave on simics and also to do i2c transactions
over i3c bus with the i2c slave (static address 0x50).

Building and Running
********************

This project can be built and executed on as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/i3c_sample_app_simics
   :host-os: unix
   :board: intel_socfpga_agilex5_socdk
   :goals: run
   :compact:
