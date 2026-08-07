.. zephyr:code-sample:: ble_peripheral_identity
   :name: Peripheral Identity
   :relevant-api: bluetooth

   Use multiple identities to allow connections from multiple central devices.

Overview
********

This sample demonstrates use of multiple identity and the ability to be
connected to from multiple central devices.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/peripheral_identity
   :board: <board>
   :goals: build flash
   :compact:

After flashing, the device advertises using one identity at a time. Each time a central
connects, a new identity is created and advertising restarts, allowing subsequent centrals
to connect on distinct identities. Use multiple central devices (e.g. smartphones with
nRF Connect) to establish several simultaneous connections.
