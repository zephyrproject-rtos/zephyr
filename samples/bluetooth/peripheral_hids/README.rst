.. zephyr:code-sample:: ble_peripheral_hids
   :name: HID Peripheral
   :relevant-api: bt_gatt bluetooth

   Implement a Bluetooth HID peripheral (generic mouse)

Overview
********

Similar to the :zephyr:code-sample:`ble_peripheral` sample, except that this
application specifically exposes the HID GATT Service. The report map used is
for a generic mouse.

In the default configuration the sample uses passkey authentication (displays a
code on the peripheral and requires that to be entered on the host during
pairing) and requires an authenticated link to access the GATT characteristics.
To disable authentication and just use encrypted channels instead, build the
sample with ``CONFIG_SAMPLE_BT_USE_AUTHENTICATION=n``.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/peripheral_hids
   :board: <board>
   :goals: build flash
   :compact:

After flashing, the device will advertise as a HID peripheral. Pair it with a host
(a passkey will be displayed on the peripheral's console and must be entered on the host).
Once paired, the device should be recognized as a generic mouse by the operating system.
