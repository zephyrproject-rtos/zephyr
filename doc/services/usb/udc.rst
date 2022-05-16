.. _udc_api:

USB device controller driver
############################

The USB Device Controller Driver Layer implements the low level control routines
to deal directly with the hardware. All device controller drivers should
implement the APIs described in :zephyr_file:`include/zephyr/drivers/usb/usb_dc.h`.
This allows the integration of new USB device controllers to be done without
changing the upper layers.
With this API it is not possible to support more than one controller
instance at runtime.

API reference
*************

.. doxygengroup:: _usb_device_controller_api
