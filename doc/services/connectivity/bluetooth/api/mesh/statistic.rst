.. _bluetooth_mesh_stat:

Frame statistic
###############

The frame statistic API allows monitoring the number of received frames over
different interfaces, and the number of planned and succeeded transmission and
relaying attempts.

The API helps the user to estimate the efficiency of the advertiser configuration
parameters and the scanning ability of the device. The number of the monitored
parameters can be easily extended by customer values.

An application can read out and clean up statistics at any time.

API reference
*************

.. doxygengroup:: bt_mesh_stat
