.. _cdc_acm_shield:

Generic shields for CDC ACM UART
################################

Overview
********

This is a generic shield that provides devicetree and configuration overlays,
and configures USB device stack so that CDC ACM UART can be used as backend
for console, logging, and shell. It is mainly intended to be used with boards
that do not have a debug adapter or native UART, but do have a USB device
controller.
This approach allows us to avoid many identical overlays in samples and tests
directories (see :ref:`usb_device_cdc_acm` for more details).
It also simplifies the configuration of the above mentioned boards,
they can stay with the minimal configuration which minimizes the conflicts
especially with different in-tree samples.

These shields enable :kconfig:`CONFIG_USB_DEVICE_INITIALIZE_AT_BOOT` and
configure USB device stack so that it is automatically initialized.
This is important for the boards like :ref:`nrf52840dongle_nrf52840`,
otherwise in-tree samples, that do not enable USB device support, are
not usable. But it also means that in-tree samples, like :ref:`usb_cdc-acm`,
that initialize USB device support themselves cannot be used with these shields.
This is a good compromise which provides maximum coverage of usable samples for
these specific USB dongles.

Current supported chosen properties
===================================

+------------------------+---------------------+
| Chosen property        | Shield Designation  |
|                        |                     |
+========================+=====================+
| ``zephyr,console``     | ``cdc_acm_console`` |
+------------------------+---------------------+
| ``zephyr,shell-uart``  | ``cdc_acm_shell``   |
+------------------------+---------------------+
| ``zephyr,bt-c2h-uart`` | ``cdc_acm_bt_c2h``  |
+------------------------+---------------------+

Requirements
************

This shield can only be used with a board which provides a USB device
controller.

Programming
***********

Set ``-DSHIELD=cdc_acm_shell`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/shell/shell_module
   :board: nrf52840dongle_nrf52840
   :shield: cdc_acm_shell
   :goals: build

Or ``-DSHIELD=cdc_acm_console``, for example:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/threads
   :board: nrf52840dongle_nrf52840
   :shield: cdc_acm_console
   :goals: build

With ``-DSHIELD=cdc_acm_bt_c2h``, :ref:`bluetooth-hci-uart-sample` can use
CDC ACM UART as interface to the host:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/hci_uart
   :board: nrf52840dongle_nrf52840
   :shield: cdc_acm_bt_c2h
   :goals: build
