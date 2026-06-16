.. _bluetooth_mesh_stat:

Mesh Statistic
##############

The statistic API provides runtime monitoring of Bluetooth Mesh communication.

Frame statistic
***************

The frame statistic API allows monitoring the number of received frames over
different interfaces, and the number of planned and succeeded transmission and
relaying attempts.

The API helps the user to estimate the efficiency of the advertiser configuration
parameters and the scanning ability of the device. The number of the monitored
parameters can be easily extended by customer values.

LPN timing measurement
**********************

When :kconfig:option:`CONFIG_BT_MESH_LOW_POWER` is enabled, the statistic module
measures LPN friendship timing parameters by timestamping protocol events:

* T1: Poll TX complete
* T2: Scanner enabled (ReceiveDelay elapsed)
* T3: Friend response received or ReceiveWindow expired

From these timestamps the module computes the measured ReceiveDelay (T2 - T1)
and ReceiveWindow (T3 - T2) in microseconds. The application can use these
values to evaluate the actual on-air timing against configured friendship
parameters.

An application can read out and reset statistics at any time.

API reference
*************

.. doxygengroup:: bt_mesh_stat
