.. zephyr:code-sample:: ble_peripheral_ets
   :name: Elapsed Time Service (ETS) Peripheral
   :relevant-api: bt_ets bluetooth

   Expose an Elapsed Time Service (ETS) GATT Service.

Overview
********

Similar to the :zephyr:code-sample:`ble_peripheral` sample, except that this
application specifically exposes the ETS (Elapsed Time Service) GATT Service.

The sample demonstrates time-of-day mode using the system realtime clock
(``SYS_CLOCK_REALTIME``). It supports both UTC time (default) and local time
with TZ/DST offset (via overlay). The implementation uses ETS helper APIs for
all time conversions between Unix epoch (1970) and ETS epoch (2000).


Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/peripheral_ets`
in the Zephyr tree.

Building with UTC time mode (default)
--------------------------------------

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/peripheral_ets
   :board: <board>
   :goals: build flash

Building with local time mode
------------------------------

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/peripheral_ets
   :board: <board>
   :goals: build flash
   :gen-args: -DEXTRA_CONF_FILE=overlay-local-time.conf

See :zephyr:code-sample-category:`bluetooth` samples for details.
