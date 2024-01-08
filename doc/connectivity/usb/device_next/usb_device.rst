.. _usb_device_stack_next:

New experimental USB device support
###################################

Overview
********

The new USB device support is experimental. It consists of :ref:`udc_api`
and :ref:`usbd_api`. The new device stack brings support for multiple device
controllers, support for multiple configurations, and dynamic registration of
class instances to a configuration at runtime. The stack also provides a specific
class API that should be used to implement the functions (classes).
It will replace :ref:`usb_device_stack`.

If you would like to play around with the new device support, or the new USB
support in general, please try :zephyr:code-sample:`usb-shell` sample. The sample is mainly to help
test the capabilities of the stack and correct implementation of the USB controller
drivers.

Supported USB classes
*********************

Bluetooth HCI USB transport layer
=================================

See :ref:`bluetooth-hci-usb-sample` sample for reference.
To build the sample for the new device support, set the configuration
``-DCONF_FILE=usbd_next_prj.conf`` either directly or via ``west``.

CDC ACM
=======

CDC ACM implementation has support for multiple instances.
Description from :ref:`usb_device_cdc_acm` also applies to the new implementation.
See :zephyr:code-sample:`usb-cdc-acm` sample for reference.
To build the sample for the new device support, set the configuration
``-DCONF_FILE=usbd_next_prj.conf`` either directly or via ``west``.

Mass Storage Class
==================

See :zephyr:code-sample:`usb-mass` sample for reference.
To build the sample for the new device support, set the configuration
``-DCONF_FILE=usbd_next_prj.conf`` either directly or via ``west``.

Networking
==========

At the moment only CDC ECM class is implemented and has support for multiple instances.
It provides a virtual Ethernet connection between the remote (USB host) and
Zephyr network support.

See :zephyr:code-sample:`zperf` for reference.
To build the sample for the new device support, set the configuration overlay file
``-DDEXTRA_CONF_FILE=overlay-usbd_next_ecm.conf`` and devicetree overlay file
``-DDTC_OVERLAY_FILE="usbd_next_ecm.overlay`` either directly or via ``west``.
