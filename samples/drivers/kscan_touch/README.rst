.. zephyr:code-sample:: kscan-touch
   :name: KSCAN touch panel
   :relevant-api: kscan_interface

   Use the KSCAN API to interface with a touch panel.

Overview
********

This sample demonstrates how to interface with a touch panel. When touches are
detected a log message is output on the console.

Building and Running
********************

The sample can be built and executed on boards with a touch panel for example
stm32f746g_disco or mimxrt1060_evk. The boards dts file should contain an alias
to kscan0

Sample output
=============

.. code-block:: console

   KSCAN test for touch panels.
   Note: You are expected to see several callbacks
   as you touch the screen.
