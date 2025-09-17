.. zephyr:code-sample:: bluetooth_observer
   :name: Observer
   :relevant-api: bt_gap bluetooth

   Scan for Bluetooth devices nearby and print their information.

Overview
********

A simple application demonstrating Bluetooth Low Energy Observer role
functionality. The application will periodically scan for devices nearby.
If any found, prints the address of the device, the RSSI value, the Advertising
type, and the Advertising data length to the console.

If the used Bluetooth Low Energy Controller supports Extended Scanning, you may
enable :kconfig:option:`CONFIG_BT_EXT_ADV` in the project configuration file. Refer to the
project configuration file for further details.

Building Extended Scanning support for BBC Micro Bit board
**********************************************************

.. code-block:: console

   west build -b bbc_microbit . -- -DCONF_FILE='prj_extended.conf' -DEXTRA_CONF_FILE='overlay_bbc_microbit-bt_ll_sw_split.conf'

Thread Analysis for BBC Micro Bit board
***************************************

Due to resource constraints on the BBC Micro Bit board, thread analysis can be enabled to profile
the RAM usage and thread stack sizes be updated to successfully build and run the sample.

.. code-block:: console

   west build -b bbc_microbit . -- -DCONF_FILE='prj_extended.conf' -DEXTRA_CONF_FILE='debug.conf;overlay_bbc_microbit-bt_ll_sw_split.conf'

Requirements
************

* A board with Bluetooth Low Energy support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/observer` in the
Zephyr tree.

See :zephyr:code-sample-category:`Bluetooth samples section <bluetooth>` for details.
