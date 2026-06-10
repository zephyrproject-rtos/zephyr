.. _sdio_device_sample:

SDIO Device Sample
##################

Overview
********

This sample demonstrates the SDIO Device driver API.

Building and Running
********************

Build and flash as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/sd_dev
   :board: <board>
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build ***
   [00:00:00.000,000] <inf> sdio_device_sample: SDIO Device Sample Started
   [00:00:00.001,000] <inf> sdio_device_core: Initializing SDIO Device Core
   [00:00:00.002,000] <inf> sdio_device_nxp_rw6xx: Initializing NXP RW6xx SDIO Device
