.. zephyr:code-sample:: ble_peripheral_hr
   :name: Heart-rate Monitor (Peripheral)
   :relevant-api: bt_hrs bt_bas bluetooth

   Expose a Heart Rate (HR) GATT Service generating dummy heart-rate values.

Overview
********

Similar to the :zephyr:code-sample:`ble_peripheral` sample, except that this
application specifically exposes the HR (Heart Rate) GATT Service. Once a device
connects it will generate dummy heart-rate values.


Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/peripheral_hr` in the
Zephyr tree.

Building a minimal variant
--------------------------

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/peripheral_hr
   :board: qemu_cortex_m3
   :goals: build
   :gen-args: -DCONF_FILE=prj_minimal.conf

Building a minimal variant for bbc_microbit
-------------------------------------------

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/peripheral_hr
   :board: bbc_microbit
   :goals: build
   :gen-args: -DCONF_FILE=prj_minimal.conf -DEXTRA_CONF_FILE=overlay-bt_ll_sw_split-minimal.conf
