.. _bt_a2dp_source:

Bluetooth: A2DP
####################

Overview
********

Application demonstrating usage of the A2dp Profile APIs.

This sample can be found under :zephyr_file:`samples/bluetooth/a2dp_source` in
the Zephyr tree.

Check :ref:`bluetooth samples section <bluetooth-samples>` for details.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth BR/EDR (Classic) support

Building and Running
********************

When building targeting mimxrt1060_evk board with the murata 1xk Controller.

Building for an mimxrt1060_evk + murata 1xk
-------------------------------------------

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/a2dp_source/
   :board: mimxrt1060_evk
   :goals: build

Building for the sample
-----------------------

1. input `bt discover` to discover BT devices.
2. input `bt connect <index>` to connect the found BT device, the <index> is the result of step 1.
3. `bt disconnect` can be used to disconnect connection.
