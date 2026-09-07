.. zephyr:code-sample:: ble_peripheral_hogp
   :name: HID over GATT Profile Device (Peripheral)
   :relevant-api: bt_hids bt_bas bt_conn bluetooth input_interface

   Act as a Bluetooth LE mouse using the HID Service.

Overview
********

Application demonstrating the HID Device role of the HID over GATT Profile
(HOGP). The Device role is a composition of services that the application puts
together, so the sample shows what a HID Device consists of:

* the HID Service (:kconfig:option:`CONFIG_BT_HIDS`), registered with the
  standard mouse Report Map from the HID class helpers,
* the Battery Service (:kconfig:option:`CONFIG_BT_BAS`),
* the Device Information Service (:kconfig:option:`CONFIG_BT_DIS`) including
  the PnP ID characteristic (:kconfig:option:`CONFIG_BT_DIS_PNP`),

which HOGP makes mandatory for the role.

All HID Service characteristics require an encrypted link and the Device has to
be bondable (:kconfig:option:`CONFIG_BT_BONDABLE`), so the sample requests
security as soon as a Host connects and stores the bond in settings. The Host
pairs with the device before any report can be exchanged.

Once a Host subscribes to the Input Report, board button presses are notified as
mouse Input Reports through the Zephyr :ref:`input <input>` subsystem, the same
way the :zephyr:code-sample:`usb-hid-mouse` sample does. The HID events arriving
from the Host are logged.

The board buttons are mapped to mouse actions as follows:

===========  ================
Button       Action
===========  ================
``sw0``      Left button
``sw1``      Right button
``sw2``      Move cursor +X
``sw3``      Move cursor +Y
===========  ================

A mouse has to expose the Boot Mouse Input Report characteristic
(:kconfig:option:`CONFIG_BT_HIDS_BOOT_MOUSE`), which brings the Protocol Mode
characteristic with it, so a Boot Host that does not parse the Report Map is
supported as well. The same button events are notified on both the Report and
the Boot Mouse Input Report.

Requirements
************

* A board with Bluetooth LE support and at least four buttons exposed through
  the input subsystem as ``sw0`` .. ``sw3``. When built for
  :zephyr:board:`native_sim <native_sim>` the bundled overlay defines these
  buttons on the emulated GPIO controller. Nothing can press them there, so the
  input shell is enabled to report the key events instead:

  .. code-block:: console

     uart:~$ input report 1 11 1
     uart:~$ input report 1 11 0

  which is a left button press and release. ``sw0`` .. ``sw3`` are the key codes
  11, 2, 3 and 4. ``input report`` reports to the input subsystem directly, so it
  does not go through gpio-keys.
* A Host supporting HOGP (Linux with BlueZ, Windows, macOS, Android or iOS)

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/peripheral_hogp`
in the Zephyr tree.

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/peripheral_hogp
   :board: nrf52840dk/nrf52840
   :goals: build flash
   :compact:

After flashing, the device advertises as ``Zephyr HOGP Mouse``. Pair with it
from the Bluetooth settings of the Host; it is then reported as a mouse, and
pressing the board buttons moves the pointer and clicks.

See :zephyr:code-sample-category:`bluetooth` for details on the Bluetooth
sample structure.
