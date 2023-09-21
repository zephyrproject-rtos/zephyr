.. _udc_api:

USB device controller (UDC) driver API
######################################

The USB device controller driver API is described in
:zephyr_file:`include/zephyr/drivers/usb/udc.h` and referred to
as the ``UDC driver`` API.

UDC driver API is experimental and is subject to change without notice.
It is a replacement for :ref:`usb_dc_api`. If you wish to port an existing
driver to UDC driver API, or add a new driver, please use
:zephyr_file:`drivers/usb/udc/udc_skeleton.c` as a starting point.

API reference
*************

.. doxygengroup:: udc_api
