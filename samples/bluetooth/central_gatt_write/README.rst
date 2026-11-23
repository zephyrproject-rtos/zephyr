.. zephyr:code-sample:: ble_central_gatt_write
   :name: Central / GATT Write
   :relevant-api: bluetooth

   Scan for a Bluetooth LE device, connect to it and write a value to a characteristic.

Overview
********

Similar to the :zephyr:code-sample:`ble_central` sample, except that this
application use GATT Write Without Response.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/central_gatt_write
   :board: <board>
   :goals: build flash
   :compact:

After flashing, the sample scans for nearby Bluetooth LE devices, connects to the
first one found, and continuously sends GATT Write Without Response commands. Use
the :zephyr:code-sample:`ble_peripheral_gatt_write` sample on a second board as the
peripheral.

Notification modes
******************

By default both peers exchange GATT Write Without Response commands. Two Kconfig
options (defined in ``Kconfig.gatt_write``) select notification-based workloads
instead:

* ``CONFIG_USE_NOTIFY`` switches the workload to GATT Notifications: the
  peripheral streams notifications and the central subscribes to and receives
  them.
* ``CONFIG_USE_FULL_DUPLEX`` (enabled by default, and only meaningful together
  with ``CONFIG_USE_NOTIFY``) makes both peers subscribe to and notify each
  other. Disable it to run the one-directional (simplex) path where only the
  peripheral notifies and the central receives.

For example, to build in simplex notification mode:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/central_gatt_write
   :board: <board>
   :goals: build
   :gen-args: -DCONFIG_USE_NOTIFY=y -DCONFIG_USE_FULL_DUPLEX=n
   :compact:
