.. _bluetooth_api:

Bluetooth API
#############

.. contents::
   :depth: 1
   :local:
   :backlinks: top

This is the full set of available Bluetooth APIs. It's important to note
that the set that will in practice be available for the application
depends on the exact Kconfig options that have been chosen, since most
of the Bluetooth functionality is build-time selectable. E.g. any
connection-related APIs require :option:`CONFIG_BLUETOOTH_CONN` and any
BR/EDR (Bluetooth Classic) APIs require :option:`CONFIG_BLUETOOTH_BREDR`.

Generic Access Profile (GAP)
****************************

.. doxygengroup:: bt_gap
   :project: Zephyr
   :content-only:

Connection Management
*********************

.. doxygengroup:: bt_conn
   :project: Zephyr
   :content-only:

Generic Attribute Profile (GATT)
********************************

.. doxygengroup:: bt_gatt
   :project: Zephyr
   :content-only:

Universal Unique Identifiers (UUIDs)
************************************

.. doxygengroup:: bt_uuid
   :project: Zephyr
   :content-only:

Logical Link Control and Adaptation Protocol (L2CAP)
****************************************************

.. doxygengroup:: bt_l2cap
   :project: Zephyr
   :content-only:

Serial Port Emulation (RFCOMM)
******************************

.. doxygengroup:: bt_rfcomm
   :project: Zephyr
   :content-only:

Data Buffers
************

.. doxygengroup:: bt_buf
   :project: Zephyr
   :content-only:

Persistent Storage
******************

.. doxygengroup:: bt_storage
   :project: Zephyr
   :content-only:

HCI Drivers
***********

.. doxygengroup:: bt_hci_driver
   :project: Zephyr
   :content-only:

HCI RAW channel
***************

HCI RAW channel API is intended to expose HCI interface to the remote entity.
The local Bluetooth controller gets owned by the remote entity and host
Bluetooth stack is not used. RAW API provides direct access to packets which
are sent and received by the Bluetooth HCI driver.

.. doxygengroup:: hci_raw
   :project: Zephyr
   :content-only:
