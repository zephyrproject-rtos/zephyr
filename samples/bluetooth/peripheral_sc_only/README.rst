.. zephyr:code-sample:: ble_peripheral_sc_only
   :name: Peripheral SC-only
   :relevant-api: bt_conn bluetooth

   Enable "Secure Connections Only" mode for a Bluetooth LE peripheral.

Overview
********

Similar to the :zephyr:code-sample:`ble_peripheral` sample, except that this
application enables the Secure Connections Only mode, i.e. will only
accept connections that are secured using security level 4 (Authenticated LE Secure Connections).


Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/peripheral_sc_only
   :board: <board>
   :goals: build flash
   :compact:

After flashing, use a central device to connect and pair with the peripheral. Upon
connection, the peripheral requests security level 4 (Authenticated LE Secure Connections).
If the central does not support LE Secure Connections, pairing will fail and the
connection will be terminated. BlueZ on Linux or a modern smartphone can be used as
the central.
