.. _usb_device_stack_next:

New USB device support
######################

Overview
********

USB device support consists of the USB device controller (UDC) drivers
, :ref:`udc_api`, and USB device stack, :ref:`usbd_api`.
The :ref:`udc_api` provides a generic and vendor independent interface to USB
device controllers, and although, there a is clear separation between these
layers, the purpose of :ref:`udc_api` is to serve new Zephyr's USB device stack
exclusively.

The new device stack supports multiple device controllers, meaning that if a
SoC has multiple controllers, they can be used simultaneously. Full and
high-speed device controllers are supported. It also provides support for
registering multiple function or class instances to a configuration at runtime,
or changing the configuration later. It has built-in support for several USB
classes and provides an API to implement custom USB functions.

The new USB device support is considered experimental and will replace
:ref:`usb_device_stack`.

Built-in functions
==================

The USB device stack has built-in USB functions. Some can be used directly in
the user application through a special API, such as HID or Audio class devices,
while others use a general Zephyr RTOS driver API, such as MSC and CDC class
implementations. The *Identification string* identifies a class or function
instance (n) and is used as an argument to the :c:func:`usbd_register_class`.

+-----------------------------------+-------------------------+-------------------------+
| Class or function                 | User API (if any)       | Identification string   |
+===================================+=========================+=========================+
| USB Audio 2 class                 | :ref:`uac2_device`      | uac2_(n)                |
+-----------------------------------+-------------------------+-------------------------+
| USB CDC ACM class                 | :ref:`uart_api`         | cdc_acm_(n)             |
+-----------------------------------+-------------------------+-------------------------+
| USB CDC ECM class                 | Ethernet device         | cdc_ecm_(n)             |
+-----------------------------------+-------------------------+-------------------------+
| USB Mass Storage Class (MSC)      | :ref:`disk_access_api`  | msc_(n)                 |
+-----------------------------------+-------------------------+-------------------------+
| USB Human Interface Devices (HID) | :ref:`usbd_hid_device`  | hid_(n)                 |
+-----------------------------------+-------------------------+-------------------------+
| Bluetooth HCI USB transport layer | :ref:`bt_hci_raw`       | bt_hci_(n)              |
+-----------------------------------+-------------------------+-------------------------+

Samples
=======

* :zephyr:code-sample:`usb-hid-keyboard`

* :zephyr:code-sample:`uac2-explicit-feedback`

Samples ported to new USB device support
----------------------------------------

To build a sample that supports both the old and new USB device stack, set the
configuration ``-DCONF_FILE=usbd_next_prj.conf`` either directly or via
``west``.

* :ref:`bluetooth-hci-usb-sample`

* :zephyr:code-sample:`usb-cdc-acm`

* :zephyr:code-sample:`usb-cdc-acm-console`

* :zephyr:code-sample:`usb-mass`

* :zephyr:code-sample:`usb-hid-mouse`

* :zephyr:code-sample:`zperf` To build the sample for the new device support,
  set the configuration overlay file
  ``-DDEXTRA_CONF_FILE=overlay-usbd_next_ecm.conf`` and devicetree overlay file
  ``-DDTC_OVERLAY_FILE="usbd_next_ecm.overlay`` either directly or via ``west``.
