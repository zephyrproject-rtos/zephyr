.. _env_logger:

Bluetooth: Environmental Logger
###############################

Overview
********

Application serves as digital thermometer, with capability of sending its
readings over BLE.
Sample also contains python scripts in 'raspi_script' catalogue. Those
scripts can be run on linux host with configured TICK stack to fetch data
from logger. Telegraf has to be configured to run 'telegraf.py' script.


Requirements
************

* Reel Board
* Linux host with TICK stack (for data logging part)

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/env_logger` in the
Zephyr tree.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.
