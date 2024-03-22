.. zephyr:code-sample:: nvs
   :name: Non-Volatile Storage (NVS)
   :relevant-api: nvs_high_level_api

   Store and retrieve data from flash using the NVS API.

Overview
********

This is a simple application demonstrating use of the NVS
module for non-volatile (flash) storage.  In this application,
a counter is incremented on every reboot and stored in flash,
the application reboots, and the reboot counter data is retrieved.

Requirements
************

* A board with flash support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/subsys/nvs` in the Zephyr tree.

The sample can be build for several platforms, the following commands build the
application for the nrf51dk/nrf51822 board.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/nvs
   :board: nrf51dk/nrf51822
   :goals: build flash
   :compact:

After flashing the image to the board the output on the console shows the
reboot counter and the boards reboots several times to show the reboot counter
is incremented.

Sample Output
=============

.. code-block:: console

   ***** Booting Zephyr OS v1.12.0-rc1-176-gf091be783 *****
   [fs/nvs] [DBG] nvs_reinit: (Re)Initializing sectors
   [fs/nvs] [DBG] _nvs_sector_init: f->write_location set to c
   [fs/nvs] [INF] nvs_mount: maximum storage length 256 byte
   [fs/nvs] [INF] nvs_mount: write-align: 1, write-addr: c
   [fs/nvs] [INF] nvs_mount: entry sector: 0, entry sector ID: 1
   No address found, adding 192.168.1.1 at id 1
   No key found, adding it at id 2
   No Reboot counter found, adding it at id 3
   Id: 4 not found, adding it
   Longarray not found, adding it as id 4
   Reboot counter history: ...0
   Oldest reboot counter: 0
   Rebooting in ...5...4...3...2...1
   ***** Booting Zephyr OS v1.12.0-rc1-176-gf091be783 *****
   [fs/nvs] [INF] nvs_mount: maximum storage length 256 byte
   [fs/nvs] [INF] nvs_mount: write-align: 1, write-addr: c7
   [fs/nvs] [INF] nvs_mount: entry sector: 0, entry sector ID: 1
   Entry: 1, Address: 192.168.1.1
   Id: 2, Key: ff fe fd fc fb fa f9 f8
   Id: 3, Reboot_counter: 1
   Id: 4, Data: DATA
   Id: 5, Longarray: 0 1 2 3 4 5 6 7 8 9 a b c d e f 10 11 12 13 14 15 16 17 18
   Reboot counter history: ...1...0
   Oldest reboot counter: 0
   Rebooting in ...5...4...3...2...1
   ...
