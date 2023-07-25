.. _bluetooth-hci-rpmsg-sample:

Bluetooth: HCI RPMsg
####################

Overview
********

This sample exposes :ref:`bluetooth_controller` support
to another device or CPU using RPMsg transport which is
a part of `OpenAMP <https://github.com/OpenAMP/open-amp/>`__.

Requirements
************

* A board with :ref:`ipm_api` driver and Bluetooth LE support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/hci_rpmsg`
in the Zephyr tree.

To use this application, you need a board with a Bluetooth controller
and IPM drivers.
You can then build this application and flash it onto your board in
the usual way. See :ref:`boards` for board-specific building and
programming information.

To test this sample, you need a separate device/CPU that acts as Bluetooth
HCI RPMsg peer.
This sample is compatible with the HCI RPMsg driver provided by
Zephyr's Bluetooth :ref:`bt_hci_drivers` core. See the
:kconfig:option:`CONFIG_BT_RPMSG` configuration option for more information.

You might need to adjust the Kconfig configuration of this sample to make it
compatible with the peer application. For example, :kconfig:option:`CONFIG_BT_MAX_CONN`
must be equal to the maximum number of connections supported by the peer application.

Refer to :ref:`bluetooth-samples` for general information about Bluetooth samples.
