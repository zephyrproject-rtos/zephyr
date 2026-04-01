.. zephyr:code-sample:: bluetooth_pbap_pse
   :name: Bluetooth Classic PBAP PSE
   :relevant-api: bluetooth

Overview
********

This sample implements a PBAP PSE (Phone Book Server Equipment) demo.

Flow
====

1. Initialize Bluetooth.
2. BR/EDR inquiry (device discovery), pick the strongest RSSI device.
3. Create ACL connection to the selected device.
4. SDP discover PBAP PSE and obtain RFCOMM channel and/or L2CAP PSM.
5. Prefer L2CAP PSM transport; fallback to RFCOMM if no PSM.
6. PBAP connect, then perform:

   * PullPhoneBook
   * PullvCardListing
   * PullvCardEntry

Building and Running
********************

Build (example):

.. code-block:: console

   west build -b <board> samples/bluetooth/classic/pbap_pce

Notes
*****

* Many phones require pairing/bonding and user permission to access contacts.
* If authentication is required, extend the sample to add OBEX auth challenge/response.
