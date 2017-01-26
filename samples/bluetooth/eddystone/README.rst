Bluetooth: Eddystone
####################

Overview
********
Application demostrating `Eddystone Configuration Service`_

The Eddystone Configuration Service runs as a GATT service on the beacon while
it is connectable and allows configuration of the advertised data, the
broadcast power levels, and the advertising intervals. It also forms part of
the definition of how Eddystone-EID beacons are configured and registered with
a trusted resolver.


Requirements
************

* BlueZ running on the host, or
* A board with BLE support

Building and Running
********************
This sample can be found under :file:`samples/bluetooth/eddystone` in the
Zephyr tree.

See :ref:`bluetooth setup section <bluetooth_setup>` for details.

.. _Eddystone Configuration Service: https://github.com/google/eddystone/tree/master/configuration-service
