.. _bluetooth_api:

Bluetooth API
#############

.. contents::
   :depth: 2
   :local:
   :backlinks: top

This is the full set of available Bluetooth APIs. It's important to note
that the set that will in practice be available for the application
depends on the exact Kconfig options that have been chosen, since most
of the Bluetooth functionality is build-time selectable. E.g. any
connection-related APIs require :option:`CONFIG_BT_CONN` and any
BR/EDR (Bluetooth Classic) APIs require :option:`CONFIG_BT_BREDR`.

.. comment
   not documenting
   .. doxygengroup:: bluetooth
   .. doxygengroup:: bt_test_cb

Bluetooth Mesh Profile
**********************

.. doxygengroup:: bt_mesh
   :project: Zephyr

Bluetooth Mesh Access Layer
===========================

.. doxygengroup:: bt_mesh_access
   :project: Zephyr

Bluetooth Mesh Configuration Client Model
=========================================

.. doxygengroup:: bt_mesh_cfg_cli
   :project: Zephyr

Bluetooth Mesh Configuration Server Model
=========================================

.. doxygengroup:: bt_mesh_cfg_srv
   :project: Zephyr

Bluetooth Mesh Health Client Model
==================================

.. doxygengroup:: bt_mesh_health_cli
   :project: Zephyr

Bluetooth Mesh Health Server Model
==================================

.. doxygengroup:: bt_mesh_health_srv
   :project: Zephyr

Bluetooth Mesh Provisioning
===========================

.. doxygengroup:: bt_mesh_prov
   :project: Zephyr

Bluetooth Mesh Proxy
====================

.. doxygengroup:: bt_mesh_proxy
   :project: Zephyr

Connection Management
*********************

.. doxygengroup:: bt_conn
   :project: Zephyr

Cryptography
************

.. doxygengroup:: bt_crypto
   :project: Zephyr

Data Buffers
************

.. doxygengroup:: bt_buf
   :project: Zephyr

Generic Access Profile (GAP)
****************************

.. doxygengroup:: bt_gap
   :project: Zephyr

Generic Attribute Profile (GATT)
********************************

.. doxygengroup:: bt_gatt
   :project: Zephyr

HCI RAW channel
***************

HCI RAW channel API is intended to expose HCI interface to the remote entity.
The local Bluetooth controller gets owned by the remote entity and host
Bluetooth stack is not used. RAW API provides direct access to packets which
are sent and received by the Bluetooth HCI driver.

.. doxygengroup:: hci_raw
   :project: Zephyr

HCI Drivers
***********

.. doxygengroup:: bt_hci_driver
   :project: Zephyr

Hands Free Profile (HFP)
************************

.. doxygengroup:: bt_hfp
   :project: Zephyr

Logical Link Control and Adaptation Protocol (L2CAP)
****************************************************

.. doxygengroup:: bt_l2cap
   :project: Zephyr

Serial Port Emulation (RFCOMM)
******************************

.. doxygengroup:: bt_rfcomm
   :project: Zephyr

Service Discovery Protocol (SDP)
********************************

.. doxygengroup:: bt_sdp
   :project: Zephyr

Universal Unique Identifiers (UUIDs)
************************************

.. doxygengroup:: bt_uuid
   :project: Zephyr
