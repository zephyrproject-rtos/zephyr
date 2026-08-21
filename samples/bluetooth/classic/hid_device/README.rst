.. zephyr:code-sample:: bluetooth_hid_device
   :name: Bluetooth: HID Device (Mouse)
   :relevant-api: bt_hid_device

   Demonstrate a Bluetooth Classic HID Device acting as a mouse.

Overview
********

This sample implements a Bluetooth Classic HID Device (mouse) that
advertises itself via SDP and waits for an incoming HID Host connection.
Once connected, it sends mouse input reports in response to board button
presses, using the Zephyr :ref:`input <input>` subsystem.

The sample demonstrates:

- Registering HID Device callbacks and the HID SDP service record
- Handling Get_Report, Set_Report and Set_Protocol requests
- Sending input reports on the interrupt channel from button events
- Boot Protocol and Report Protocol mode support
- Suspend/Exit-Suspend and Virtual Cable Unplug handling

Get_Protocol is answered by the host stack and needs no application
callback.

The board buttons are mapped to mouse actions as follows:

===========  ================
Button       Action
===========  ================
``sw0``      Left button
``sw1``      Right button
``sw2``      Move cursor +X
``sw3``      Move cursor +Y
===========  ================

Requirements
************

- A board with Bluetooth BR/EDR support and at least four buttons exposed
  through the input subsystem as ``sw0`` .. ``sw3``. When built for
  :ref:`native_sim <native_sim>` the bundled overlay defines these buttons
  on the emulated GPIO controller.
- A Bluetooth HID Host (e.g., a PC or phone) to connect to the device

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/classic/hid_device
   :board: <board>
   :goals: build flash
   :compact:

After flashing, the device will initialize Bluetooth, register the HID
service, and become discoverable. Pair with it from a HID Host, then press
the board buttons to generate mouse clicks and cursor movement on the host.

On :ref:`native_sim <native_sim>`, where there are no physical buttons, the
button GPIOs can be driven from an SDL window by adding a
``zephyr,gpio-emul-sdl`` child to the emulated GPIO controller (see
:zephyr_file:`samples/subsys/display/lvgl/boards/native_sim.overlay` for the
pattern) and enabling ``CONFIG_GPIO_EMUL_SDL``. Combined with a USB HCI dongle
as the controller and a second dongle running a Bluetooth HID Host, this drives
the full send path end to end.
