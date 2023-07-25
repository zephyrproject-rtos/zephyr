.. _coredump_device_api:

Coredump Device
###############

Overview
********

The coredump device is a pseudo-device driver with two types.A COREDUMP_TYPE_MEMCPY
type exposes device tree bindings for memory address/size values to be included in
any dump. And the driver exposes an API to add/remove dump memory regions at runtime.
A COREDUMP_TYPE_CALLBACK device requires exactly one entry in the memory-regions
array with a size of 0 and a desired size. The driver will statically allocate memory
of the desired size and provide an API to register a callback function to fill that
memory when a dump occurs.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_COREDUMP_DEVICE`

API Reference
*************

.. doxygengroup:: coredump_device_interface
