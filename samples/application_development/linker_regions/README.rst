.. _linker_regions:

Linker Regions
##############

Overview
********

A sample that demonstrates using devicetree to define linker
memory regions. These memory regions allow constants to be
placed at desired memory addresses in a portable manner.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/application_development/linker_regions
   :host-os: unix
   :board: qemu_cortex_m3
   :goals: run
   :compact:
