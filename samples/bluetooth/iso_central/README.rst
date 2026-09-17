.. zephyr:code-sample:: ble_central_iso
   :name: ISO (Central)
   :relevant-api: bt_iso bluetooth

   Transfer isochronous data to a peer device using an isochronous channel as a central.

Overview
********

This sample demonstrates how to use an isochronous channel as a central.
The sample scans for a peripheral, establishes a connection, and sets up a connected isochronous channel to it.
Once the isochronous channel is connected, isochronous data is transferred to the peer device every 10 milliseconds.
It is recommended to run this sample together with the :zephyr:code-sample:`ble_peripheral_iso` sample.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth Low Energy 5.2 support
* A Bluetooth Controller and board that supports setting
  :kconfig:option:`CONFIG_BT_CTLR_CENTRAL_ISO`.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/iso_central
   :board: <board>
   :goals: build flash
   :compact:

Use ``-DEXTRA_CONF_FILE=overlay-bt_ll_sw_split.conf`` to enable
required ISO feature support in Zephyr Bluetooth Controller on supported boards.

After flashing, check the following:

1. Start the application.
   In the terminal window, check that it is scanning for other devices.

      Bluetooth initialized
      Scanning successfully started
      Device found: R:D3:3A:5D:F5:73:33 (RSSI -78)
      Device found: R:70:7B:F4:2B:76:AD (RSSI -68)
      Device found: R:65:CF:20:0D:CB:9D (RSSI -82)

2. Observe that the device connects.

      Connected: R:65:CF:20:0D:CB:9D

3. Observe that the ISO channel is connected.

      ISO Channel 0x200048f8 connected

See :zephyr:code-sample-category:`bluetooth` samples for more details.
