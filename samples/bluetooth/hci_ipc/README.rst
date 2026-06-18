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

Build and flash the sample as follows, replacing ``<board>`` with your target
board (e.g. :zephyr:board:`nrf5340dk` using the ``nrf5340dk/nrf5340/cpunet``
target):

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/hci_ipc
   :board: <board>
   :goals: build flash
   :compact:

To test this sample, you need a separate device/CPU that acts as Bluetooth
HCI IPC peer.
This sample is compatible with the HCI IPC driver provided by
Zephyr's Bluetooth :ref:`bt_hci_drivers` core. See the
:kconfig:option:`CONFIG_BT_HCI_IPC` configuration option for more information.

You might need to adjust the Kconfig configuration of this sample to make it
compatible with the peer application. For example, :kconfig:option:`CONFIG_BT_MAX_CONN`
must be equal to the maximum number of connections supported by the peer application.

Refer to :zephyr:code-sample-category:`bluetooth` for general information about Bluetooth samples.
