.. _uds_next:

USB device core support
#######################

The new USB device support is experimental and under development. It consists
of low level USB device controller (UDC) driver API, USB device stack (core)
and different functions implementing specific classes. The device stack and
driver API bring multiple device support for the (rare) case that a board has
multiple USB device controllers. Device stack has support for multiple
configurations and a class instance can be added or removed at runtime to a
configuration. The stack provides a specific API for implementing the
functions (classes). This takes over the configuration of the class interfaces
and endpoints, and also the communication with the stack and driver API.
The stack can be enabled by the :kconfig:option:`CONFIG_USB_DEVICE_STACK_NEXT`.

The first time there will be only one sample for the new USB support,
:ref:`usb_shell-app`, with all available USB support shell commands.
The sample is mainly to help test the capabilities of the stack and correct
implementation of the USB controller drivers.

USB device stack core API reference
***********************************

.. doxygengroup:: usbd_api

UDC driver API reference
************************

The new USB device controller (UDC) driver API implements the low level layer
to interface with USB device controller.
UDC driver API is experimental and is subject to change without notice, it
is described in :zephyr_file:`include/drivers/usb/udc.h`.

.. doxygengroup:: udc_api
