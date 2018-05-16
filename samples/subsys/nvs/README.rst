.. _nvs-sample:

NVS: Non-Volatile Storage
#########################

Overview
********

A simple application demonstrating the use of the nvs module for Non-Volatile
(flash) Storage. In this application data is stored in flash, the application
reboots and the data can be retrieved over a reboot. A reboot counter is stored
and is increased for every reboot.

Requirements
************

* A board with flash support

Building and Running
********************

This sample can be found under :file:`samples/subsys/nvs/` in the
Zephyr tree.
