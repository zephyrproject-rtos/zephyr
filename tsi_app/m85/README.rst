.. _m85:

TSI Banner
###########

Overview
********

This is the core TSI M85 Zephyr platform application startup code. It prints TSI Banner to the console, enables logging functionality  so that developers can log messages to console at different levels of severity such as Info, Warning, Error & Debug. It also enables the shell functionality.

Building and Running
********************

This application can be built as follows:

.. zephyr-app-commands::
   :zephyr-app: tsi_m85
   :host-os: unix
   :board: tsi/skyp/m85
   :goals: run
   :compact:


Output
=============

.. code-block:: console
***** !! WELCOME TO TSAVORITE SCALABLE INTELLIGENCE !! *****
[00:00:00.010,000] <inf> tsi_m85: Test Platform: tsi/skyp/m85
[00:00:00.020,000] <wrn> tsi_m85: Testing on FPGA; Multi module init TBD
