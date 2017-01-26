Bluetooth: IPSP Sample
######################

Overview
********
Application demonstrating the IPSP (Internet Protocol Support Profile) Node
role. IPSP is the Bluetooth profile that underneath utilizes 6LoWPAN, i.e. gives
you IPv6 connectivity over BLE.

Requirements
************

This application currently only works with HCI based firmware since it
requires L2CAP channels support.

Building and Running
********************

This sample can be found under :file:`samples/bluetooth/ipsp` in the
Zephyr tree.

Testing with a Linux host
=========================

To test IPSP please take a look at samples/net/README, in addition to running
echo-client you must enable 6LowPAN module in Linux with the
following commands:

.. code-block:: console

   $ modprobe bluetooth_6lowpan
   $ echo 1 > /sys/kernel/debug/bluetooth/6lowpan_enable

Then to connect:

.. code-block:: console

   $ echo "connect <bdaddr> <type>" > /sys/kernel/debug/bluetooth/6lowpan_control

Once connected a dedicated interface will be created, usually bt0, which can
then be used as following:

.. code-block:: console

   $ echo-client -i bt0 <ip>
