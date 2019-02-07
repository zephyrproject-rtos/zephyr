.. _settings-sample-default:

Settings: Non-Volatile Storage of settings
##########################################

Overview
********

This is a simple application demonstrating use of the settings
module for non-volatile (flash) storage. The application demonstrates the
storing, deleting and loading of variables from different default backends.

Requirements
************

* A board with flash support

Building and Running
********************

This sample can be found under :file:`samples/subsys/settings/default` in the Zephyr tree.

The sample can be built for several platforms. The following commands build the
application for the nrf51_pca10028 board. For the nrf51 the selected backend
can only be FCB or NVS.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/settings/default
   :board: nrf51_pca10028
   :goals: build flash
   :compact:
