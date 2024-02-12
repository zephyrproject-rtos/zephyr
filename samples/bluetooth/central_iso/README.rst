.. _bluetooth_central_iso:

Bluetooth: Central ISO
######################

Overview
********

This sample demonstrates how to use an isochronous channel as a central.
The sample scans for a peripheral, establishes a connection, and sets up a connected isochronous channel to it.
Once the isochronous channel is connected, isochronous data is transferred to the peer device every 10 milliseconds.
It is recommended to run this sample together with the :ref:`Bluetooth: Peripheral ISO <peripheral_iso>` sample.

To run the sample with an encrypted isochronous channel, enable :kconfig:option:`CONFIG_BT_SMP`.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth Low Energy 5.2 support
* A Bluetooth Controller and board that supports setting
  :kconfig:option:`CONFIG_BT_CTLR_CENTRAL_ISO`.

Building and Running
********************
This sample can be found under :zephyr_file:`samples/bluetooth/central_iso` in
the Zephyr tree.

1. Start the application.
   In the terminal window, check that it is scanning for other devices.

      Bluetooth initialized
      Scanning successfully started
      Device found: D3:3A:5D:F5:73:33 (random) (RSSI -78)
      Device found: 70:7B:F4:2B:76:AD (random) (RSSI -68)
      Device found: 65:CF:20:0D:CB:9D (random) (RSSI -82)

2. Observe that the device connects.

      Connected: 65:CF:20:0D:CB:9D (random)

3. Observe that the ISO channel is connected

      ISO Channel 0x200048f8 connected

See :ref:`bluetooth samples section <bluetooth-samples>` for more details.
