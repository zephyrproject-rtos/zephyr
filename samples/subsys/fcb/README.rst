.. zephyr:code-sample:: fcb
   :name: Flash Circular Buffer (FCB)
   :relevant-api: fcb_api flash_area_api

   Store and retrieve data from flash using the FCB API.

Overview
********

This is a simple application demonstrating use of the FCB
module for non-volatile (flash) storage.  In this application,
a counter is incremented on every reboot and stored in flash,
the application reboots, and the reboot counter data is retrieved.

Requirements
************

* A board with flash support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/subsys/fcb` in the Zephyr tree.

The sample can be built for several platforms, the following commands build the
application for the stm32f4_disco board.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/fcb
   :board: stm32f4_disco
   :goals: build flash
   :compact:

After flashing the image to the board the output on the console shows the
reboot counter and the boards reboots several times to show the reboot counter
is incremented.

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build v4.1.0-1511-g19c6240b6865 ***
   Booted 0 times
   Saving boot_cnt 1 to FCB
   New boot_cnt value written to flash
   Rebooting in 1 second
   *** Booting Zephyr OS build v4.1.0-1511-g19c6240b6865 ***
   Booted 1 times
   Saving boot_cnt 2 to FCB
   New boot_cnt value written to flash
   Rebooting in 1 second
   ...
