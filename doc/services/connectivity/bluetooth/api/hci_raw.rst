.. _bt_hci_raw:


HCI RAW channel
###############

Overview
********

HCI RAW channel API is intended to expose HCI interface to the remote entity.
The local Bluetooth controller gets owned by the remote entity and host
Bluetooth stack is not used. RAW API provides direct access to packets which
are sent and received by the Bluetooth HCI driver.

API Reference
*************

.. doxygengroup:: hci_raw
