.. _st-s2ram:

STMicroelectronics suspend to ram configuration (st-s2ram)
################################################

Overview
********

This snippet allows users to build Zephyr applications for STMicroelectronics devices
using a suspend to ram configuration (mapped on stdby hw feature).
Tested on nucleo_wba55cg and nucleo_wba65ri boards.

.. code-block:: pm

   west build -S st-s2ram [...]

Requirements
************

Actually this configuration has been tested only nucleo_wba55cg and nucleo_wba65ri