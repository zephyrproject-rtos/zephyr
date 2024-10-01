.. zephyr:code-sample:: bluetooth_broadcaster_multiple
   :name: Multiple Broadcaster
   :relevant-api: bluetooth

   Advertise multiple advertising sets.

Overview
********

A simple application demonstrating the Bluetooth Low Energy Broadcaster that
uses multiple advertising sets functionality.

This sample advertises two non-connectable non-scannable advertising sets with
two different SID. Number of advertising sets can be increased by updating the
:kconfig:option:`CONFIG_BT_EXT_ADV_MAX_ADV_SET` value in the project configuration file.

When building this sample combined with a Bluetooth LE Controller, the
advertising data length can be increased from the default 31 bytes by updating
the Controller's :kconfig:option:`CONFIG_BT_CTLR_ADV_DATA_LEN_MAX` value. The size of the
manufacturer data is calculated to maximize the use of supported AD data length.

Requirements
************

* A board with Bluetooth Low Energy with Extended Advertising support.

Building and Running
********************

This sample can be found under
:zephyr_file:`samples/bluetooth/broadcaster_multiple` in the Zephyr tree.

To test this sample use the Observer sample with Extended Scanning enabled,
found under
:zephyr_file:`samples/bluetooth/observer` in the Zephyr tree.

See :zephyr:code-sample-category:`Bluetooth samples section <bluetooth>` for details.
