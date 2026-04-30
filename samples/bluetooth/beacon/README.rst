.. zephyr:code-sample:: bluetooth_beacon
   :name: Beacon
   :relevant-api: bluetooth

   Advertise an Eddystone URL using GAP Broadcaster role.

Overview
********

A simple application demonstrating the GAP Broadcaster role functionality by
advertising an Eddystone URL (the Zephyr website).

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/beacon
   :board: <board>
   :goals: build flash
   :compact:

After flashing, use an Eddystone-compatible scanner app (e.g. nRF Connect) to observe
the advertised Eddystone URL beacon pointing to the Zephyr Project website.

Building with Bluetooth LE controller coexistence support
=========================================================

On boards where the Bluetooth LE controller must share the 2.4 GHz band with another
radio (e.g. 802.15.4/Thread/Zigbee), build with the coexistence configuration:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/beacon
   :board: nrf52840dk/nrf52840
   :goals: build flash
   :gen-args: -DCONF_FILE=prj-coex.conf
   :compact:
