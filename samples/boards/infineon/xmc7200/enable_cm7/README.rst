.. zephyr:code-sample:: xmc7200_m0plus_start
   :name: Enable M7 on XMC7200

   This code sample shows how to enable the M7 cores on the XMC7200.

Overview
********
This application demonstrates how to enable the M7 cores
from the M0 core of the :zephyr:board:`kit_xmc72_evk`

Building and Running
********************
Build this application for the M0 core of the :zephyr:board:`kit_xmc72_evk`

.. zephyr-app-commands::
   :zephyr-app: samples/boards/infineon/xmc7200/enable_cm7
   :board: kit_xmc72_evk/xmc7200d_e272k8384/m0p
   :goals: build flash
   :compact:
