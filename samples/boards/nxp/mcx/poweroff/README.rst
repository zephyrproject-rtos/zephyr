.. zephyr:code-sample:: nxp_mcx_poweroff
   :name: NXP MCXA/N Series MCUs Poweroff
   :relevant-api: sys_poweroff

   Use poweroff on NXP MCXA/N series MCUs.

Overview
********

This example demonstrates how to power off NXP MCXA/N series MCUs.

Building, Flashing and Running
******************************

Building and Running for NXP FRDM-MCXA153
=========================================
Build the application for the :zephyr:board:`frdm_mcxa153` board.

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/mcx/poweroff
   :board: frdm_mcxa153
   :goals: build flash
   :compact:

Building and Running for NXP FRDM-MCXA156
=========================================
Build the application for the :zephyr:board:`frdm_mcxa156` board.

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/mcx/poweroff
   :board: frdm_mcxa156
   :goals: build flash
   :compact:

Building and Running for NXP FRDM-MCXA166
=========================================
Build the application for the :zephyr:board:`frdm_mcxa166` board.

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/mcx/poweroff
   :board: frdm_mcxa166
   :goals: build flash
   :compact:

Building and Running for NXP FRDM-MCXA276
=========================================
Build the application for the :zephyr:board:`frdm_mcxa276` board.

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/mcx/poweroff
   :board: frdm_mcxa276
   :goals: build flash
   :compact:

Building and Running for NXP FRDM-MCXN236
=========================================
Build the application for the :zephyr:board:`frdm_mcxn236/mcxn236` board.

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/mcx/poweroff
   :board: frdm_mcxn236/mcxn236
   :goals: build flash
   :compact:

Building and Running for NXP FRDM-MCXN947
=========================================
Build the application for the :zephyr:board:`frdm_mcxn947/mcxn947/cpu0` board.

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/mcx/poweroff
   :board: frdm_mcxn947/mcxn947/cpu0
   :goals: build flash
   :compact:

Sample Output
=============
FRDM-MCXA153, FRDM-MCXA156, FRDM-MCXA166, FRDM-MCXA276 FRDM-MCXN236, FRDM-MCXN947 output
----------------------------------------------------------------------------------------

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0-rc1-255-gf71b531cb990 ***
   Will wakeup after 5 seconds
   Press key to power off the system
   Powering off
