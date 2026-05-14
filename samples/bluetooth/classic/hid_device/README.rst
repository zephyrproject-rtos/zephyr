.. zephyr:code-sample:: bluetooth_hid_device
   :name: Bluetooth: HID Device (Mouse)
   :relevant-api: bt_hid_device

   Demonstrate a Bluetooth Classic HID Device acting as a mouse.

Overview
********

This sample implements a Bluetooth Classic HID Device (mouse) that
advertises itself via SDP and waits for an incoming HID Host connection.
Once connected, it periodically sends mouse input reports simulating
circular cursor movement.

The sample demonstrates:

- Registering HID Device callbacks and SDP service record
- Handling Get/Set Report and Get/Set Protocol requests
- Sending periodic input reports on the interrupt channel
- Boot Protocol and Report Protocol mode support
- Suspend/Resume handling

Requirements
************

- A board with Bluetooth BR/EDR support
- A Bluetooth HID Host (e.g., a PC or phone) to connect to the device

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/classic/hid_device
   :board: <board>
   :goals: build flash
   :compact:

After flashing, the device will initialize Bluetooth, register the HID
service, and become discoverable. Pair with it from a HID Host to see
the mouse cursor move in a circle.
