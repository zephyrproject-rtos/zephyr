.. zephyr:code-sample:: ble_peripheral_gatt_write
   :name: Peripheral GATT Write
   :relevant-api: bt_gatt bluetooth

   Write a value to a characteristic using GATT Write Without Response.

Overview
********

Similar to the :zephyr:code-sample:`ble_peripheral` sample, except that this
application uses GATT Write Without Response.


Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/peripheral_gatt_write
   :board: <board>
   :goals: build flash
   :compact:

Notification modes
******************

By default both peers exchange GATT Write Without Response commands. Two Kconfig
options (defined in the central sample's ``Kconfig.gatt_write``, shared by this
sample) select notification-based workloads instead:

* ``CONFIG_USE_NOTIFY`` switches the workload to GATT Notifications: this
  peripheral streams notifications and the central subscribes to and receives
  them.
* ``CONFIG_USE_FULL_DUPLEX`` (enabled by default, and only meaningful together
  with ``CONFIG_USE_NOTIFY``) makes both peers subscribe to and notify each
  other. Disable it to run the one-directional (simplex) path where only this
  peripheral notifies.

For example, to build in simplex notification mode:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/peripheral_gatt_write
   :board: <board>
   :goals: build
   :gen-args: -DCONFIG_USE_NOTIFY=y -DCONFIG_USE_FULL_DUPLEX=n
   :compact:

See :zephyr:code-sample-category:`bluetooth` samples for details.
