.. zephyr:code-sample:: nxp_mcu_poweroff
   :name: NXP MCU Poweroff
   :relevant-api: sys_poweroff

   Use poweroff on NXP MCUs.

Overview
********

This example shows how to poweroff an NXP MCU. In this example,
the wakeup source can be the MCU internal counter or the wakeup
button/gpio. If the counter is used, the MCU will wake up after
5 seconds. If the wakeup button is used, the MCU will wake up
after the wakeup button is pressed.

Building, Flashing and Running
******************************

Building and Running for NXP FRDM-MCXA153
=========================================
Build the application for the :zephyr:board:`frdm_mcxa153` board.
Note: After wakeup, the chip goes through the entire reset process.

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/poweroff
   :board: frdm_mcxa153
   :goals: build flash
   :compact:

Building and Running for NXP FRDM-MCXA156
=========================================
Build the application for the :zephyr:board:`frdm_mcxa156` board.
Note: After wakeup, the chip goes through the entire reset process.

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/poweroff
   :board: frdm_mcxa156
   :goals: build flash
   :compact:

Building and Running for NXP FRDM-MCXA346
=========================================
Build the application for the :zephyr:board:`frdm_mcxa346` board.
Note: After wakeup, the chip goes through the entire reset process.

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/poweroff
   :board: frdm_mcxa346
   :goals: build flash
   :compact:

Building and Running for NXP FRDM-MCXA266
=========================================
Build the application for the :zephyr:board:`frdm_mcxa266` board.
Note: After wakeup, the chip goes through the entire reset process.

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/poweroff
   :board: frdm_mcxa266
   :goals: build flash
   :compact:

Building and Running for NXP FRDM-MCXN236
=========================================
Build the application for the :zephyr:board:`frdm_mcxn236` board.
Note: After wakeup, the chip goes through the entire reset process.

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/poweroff
   :board: frdm_mcxn236/mcxn236
   :goals: build flash
   :compact:

Building and Running for NXP FRDM-MCXN947
=========================================
Build the application for the :zephyr:board:`frdm_mcxn947` board.
Note: After wakeup, the chip goes through the entire reset process.

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/poweroff
   :board: frdm_mcxn947/mcxn947/cpu0
   :goals: build flash
   :compact:

Building and Running for NXP MIMXRT595-EVK
==========================================
Build the application for the :zephyr:board:`mimxrt595_evk` board.
Note: After wakeup, the chip goes through the entire reset process.

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/poweroff
   :board: mimxrt595_evk/mimxrt595s/cm33
   :goals: build flash
   :compact:

Sample Output
=============
FRDM-MCXA153, FRDM-MCXA156, FRDM-MCXA346, FRDM-MCXA266 FRDM-MCXN236, FRDM-MCXN947,
MIMXRT595-EVK, output
----------------------------------------------------------------------------------------

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0-rc1-255-gf71b531cb990 ***
   Press 'enter' key to power off the system
   Will wakeup after 5 seconds
   Powering off

FRDM-MCXN236 output
--------------------

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0-rc3-17-g70b7c64e1094 ***
   Press 'enter' key to power off the system
   Will wakeup after press wakeup button
   Powering off
