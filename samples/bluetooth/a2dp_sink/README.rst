.. _bt_a2dp_sink:

Bluetooth: A2DP
####################

Overview
********

Application demonstrating usage of the A2dp Profile APIs.

This sample can be found under :zephyr_file:`samples/bluetooth/a2dp_sink` in
the Zephyr tree.

Check :ref:`bluetooth samples section <bluetooth-samples>` for details.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth BR/EDR (Classic) support

Building and Running
********************

Building for an any board that supports br/edr
----------------------------------------------

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/a2dp_sink/
   :board: <board>
   :goals: build

When building targeting mimxrt1060_evk board with the murata 1xk Controller adn the board codec (WM8960).

Building for an mimxrt1060_evk + murata 1xk + board codec (WM8960)
------------------------------------------------------------------

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/a2dp_sink/
   :board: mimxrt1060_evk
   :goals: build
   :gen-args: -DEXTRA_CONF_FILE=mimxrt1060_evk_wm8960.conf
