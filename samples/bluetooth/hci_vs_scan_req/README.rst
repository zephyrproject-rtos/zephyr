.. zephyr:code-sample:: bluetooth_hci_vs_scan_req
   :name: HCI Vendor-Specific Scan Request
   :relevant-api: bluetooth

   Use vendor-specific HCI commands to enable Scan Request events when using legacy advertising.

Overview
********

This simple application is a usage example to manage HCI VS commands to obtain
scan request events even using legacy advertisements, while may result in lower
RAM usage than using extended advertising.
This is quite important in applications in which the broadcaster role is added
to the central role, where the RAM saving can be bigger.
This sample implements only the broadcaster role; the peripheral role with
connection can also be added, depending on configuration choices.

Requirements
************

* A board with Bluetooth LE support
* A central device & monitor (e.g. nRF Connect) to check the advertiments and
  send scan requests.

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/hci_vs_scan_req`
in the Zephyr tree.

See :zephyr:code-sample-category:`bluetooth` samples for details.
