.. zephyr:code-sample:: wifi-ble-provisioning
   :name: Wi-Fi provisioning over BLE
   :relevant-api: bluetooth wifi_mgmt wifi_credentials

   Receive Wi-Fi credentials over a BLE GATT service and connect to
   the access point.

Overview
********

On first boot, the device advertises a custom GATT service that lets
a BLE central write the Wi-Fi SSID and pre-shared key. On a control
command the credentials are stored via the
:ref:`lib_wifi_credentials` library and the device joins the Wi-Fi
network using
:c:macro:`NET_REQUEST_WIFI_CONNECT_STORED`. On subsequent boots, the
stored credentials are used to reconnect automatically.

State machine
=============

.. code-block:: none

   IDLE --(SSID + PSK + CONNECT)--> CONNECTING --> CONNECTED
                                         \
                                          --> FAILED

The control command ``ERASE`` clears stored credentials and returns
to ``IDLE``.

GATT service
============

All characteristics share the custom 128-bit base UUID
``8b84xxxx-f6fa-4e33-9a0c-e14d9f35f201`` where the short identifier
replaces ``xxxx``:

=========  ======  ==============  ==========================================
Char       UUID    Properties      Payload
=========  ======  ==============  ==========================================
SSID       0002    write           UTF-8, up to 32 bytes
Password   0003    write           UTF-8, up to 64 bytes
Security   0004    write           1 byte (optional, see table below)
Control    0005    write           1 byte: 0x01 = CONNECT, 0x02 = ERASE
Status     0006    read, notify    1 byte enum (see ``gatt_svc.h``)
=========  ======  ==============  ==========================================

Security type values match :c:enum:`wifi_security_type`:

======  ============================================================
Byte    Meaning
======  ============================================================
0       Open (no password)
1       WPA2-PSK
2       WPA2-PSK-SHA256
3       WPA3-SAE
6       WPA-Auto-Personal (WPA2/WPA3 transition)
0xFF    Auto-detect: open if PSK empty, otherwise WPA2-PSK (default)
======  ============================================================

If the Security characteristic is not written, the device falls back
to ``0xFF`` (auto-detect).

Requirements
************

A board with both Wi-Fi and Bluetooth LE support.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/net/wifi/ble_provisioning
   :board: esp32c6_devkitc/esp32c6/hpcore
   :goals: build flash
   :compact:

After flashing, the device starts advertising as
``Zephyr Wi-Fi Provisioning`` (configurable through
:kconfig:option:`CONFIG_BT_DEVICE_NAME`).

Testing with nRF Connect Mobile
===============================

#. Install `nRF Connect for Mobile`_ on Android or iOS.
#. Open the app, tap **Scan**, and connect to the advertised device.
#. Expand the custom service (base UUID
   ``8b840001-f6fa-4e33-9a0c-e14d9f35f201``).
#. Write the SSID as UTF-8 bytes to the ``0002`` characteristic.
#. Write the password to the ``0003`` characteristic.
#. (Optional) Write the Security byte to the ``0004`` characteristic.
#. (Optional) Enable notifications on the Status ``0006``
   characteristic.
#. Write ``0x01`` to the Control ``0005`` characteristic to request a
   Wi-Fi connect.
#. Watch the serial console and the Status notifications
   transitioning through ``CONNECTING`` (2) and ``CONNECTED`` (3).

To erase stored credentials, write ``0x02`` to the Control
characteristic.

.. _nRF Connect for Mobile:
   https://www.nordicsemi.com/Products/Development-tools/nRF-Connect-for-mobile

Security
********

Three BLE security modes are selectable via a Kconfig choice:

**None** (:kconfig:option:`CONFIG_WIFI_BLE_PROV_SECURITY_NONE`, default)
   No pairing. Credentials travel in plaintext. Evaluation only.

**Encrypted link** (:kconfig:option:`CONFIG_WIFI_BLE_PROV_SECURITY_ENCRYPT`)
   Writes require an encrypted link. Pairing happens without user
   interaction; a passive eavesdropper cannot recover the
   credentials.

**Authenticated pairing** (:kconfig:option:`CONFIG_WIFI_BLE_PROV_SECURITY_AUTH`)
   Writes require authenticated LESC pairing with encryption. The
   device prints a 6-digit passkey on the console that the central
   must enter.

Stored credentials remain plaintext in NVS. For credential-at-rest
protection, switch the backend to
:kconfig:option:`CONFIG_WIFI_CREDENTIALS_BACKEND_PSA` or use vendor
NVS encryption.

.. note::

   Switching security modes reuses the same NVS partition. Stored BLE
   bonds from a previous build may log ``set-value failure`` warnings
   on boot; erase flash (``west flash --erase``) when switching.

Sample Kconfig options
**********************

* :kconfig:option:`CONFIG_BT_DEVICE_NAME` -- Advertised device name.
* :kconfig:option:`CONFIG_WIFI_BLE_PROV_KEEP_BLE_AFTER_CONNECT` --
  Keep advertising after Wi-Fi is connected.
* :kconfig:option:`CONFIG_WIFI_BLE_PROV_SECURITY_NONE` /
  :kconfig:option:`CONFIG_WIFI_BLE_PROV_SECURITY_ENCRYPT` /
  :kconfig:option:`CONFIG_WIFI_BLE_PROV_SECURITY_AUTH` --
  BLE security mode for provisioning writes.

Sample output
*************

.. code-block:: console

   *** Booting Zephyr OS build v4.4.0 ***
   <inf> wifi_ble_prov: Wi-Fi BLE provisioning starting
   <inf> wifi_ble_prov: security: none (plaintext)
   <inf> gatt_svc: status -> 0
   <inf> wifi_ble_prov: no stored credentials; waiting for provisioning over BLE
   <inf> gatt_svc: advertising as 'Zephyr Wi-Fi Provisioning'
   <inf> wifi_ble_prov: connected: 4F:0D:14:E8:76:98 (random)
   <inf> wifi_prov: connecting using stored credentials
   <inf> gatt_svc: status -> 2
   <inf> wifi_prov: Wi-Fi connected
   <inf> gatt_svc: status -> 3
