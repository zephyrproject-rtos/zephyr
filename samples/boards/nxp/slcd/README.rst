.. zephyr:code-sample:: nxp_slcd
   :name: NXP SLCD 4-digit demo
   :relevant-api: auxdisplay_interface

Overview
********

This sample demonstrates driving a 4-digit segment LCD using the NXP SLCD
peripheral via the Zephyr :ref:`auxiliary display driver <auxdisplay_api>`.

The sample writes numbers and decimal points to the display in a loop.

Supported boards
****************

- ``frdm_mcxc444/mcxc444``

Building and Running
********************

Build for FRDM-MCXC444::

  west build -b frdm_mcxc444 zephyr/samples/boards/nxp/slcd

Then flash::

  west flash

You should see a counter updating on the 4-digit LCD.
