.. _bluetooth-hci-vs-scan-req-sample:

Bluetooth: HCI VS Scan Request
##############################

Overview
********

This simple application is a usage example to manage HCI VS commands to obtain
scan equest events even using legacy advertisements, while may result in lower
RAM usage than using extended advertising.
This is quite important in applications in which the broadcaster role is added
to the central role, where the RAM saving can be bigger.
This sample implements only the broadcaster role; the peripheral role with
connection can also be added, depending on configuration choices.

Requirements
************

* A board with BLE support
* A central device & monitor (e.g. nRF Connect) to check the advertiments and
  send scan requests.

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/hci_vs_scan_req`
in the Zephyr tree.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.
