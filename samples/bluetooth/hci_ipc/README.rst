.. zephyr:code-sample:: bluetooth_hci_ipc
   :name: HCI IPC
   :relevant-api: hci_raw bluetooth

   Expose a Bluetooth controller to another device or CPU using the IPC subsystem.

Overview
********

This sample exposes :ref:`bluetooth_controller` support
to another device or CPU using IPC subsystem.

Requirements
************

* A board with IPC subsystem and Bluetooth LE support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/hci_ipc`
in the Zephyr tree.

To use this application, you need a board with a Bluetooth controller
and IPC support.
You can then build this application and flash it onto your board in
the usual way. See :ref:`boards` for board-specific building and
programming information.

To test this sample, you need a separate device/CPU that acts as Bluetooth
HCI IPC peer.
This sample is compatible with the HCI IPC driver provided by
Zephyr's Bluetooth :ref:`bt_hci_drivers` core. See the
:kconfig:option:`CONFIG_BT_HCI_IPC` configuration option for more information.

You might need to adjust the Kconfig configuration of this sample to make it
compatible with the peer application. For example, :kconfig:option:`CONFIG_BT_MAX_CONN`
must be equal to the maximum number of connections supported by the peer application.

Refer to :ref:`bluetooth-samples` for general information about Bluetooth samples.
