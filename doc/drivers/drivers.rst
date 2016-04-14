.. _device_drivers:

Device Drivers and Device Model
###############################

Introduction
************
The Zephyr device model provides a consistent device model for configuring the
drivers that are part of the platform/system. The device model is responsible
for initializing all the drivers configured into the system.

Each type of driver (UART, SPI, I2C) is supported by a generic type API.

In this model the driver fills in the pointer to the structure containing the
function pointers to its API functions during driver initialization. These
structures are placed into the RAM section in initialization level order.


.. include:: synchronous_call.rst

Driver APIs
***********

The following APIs for device drivers are provided by :file:`device.h`. The APIs
are intended for use in device drivers only and should not be used in
applications.

:c:func:`DEVICE_INIT()`
   create device object and set it up for boot time initialization.

:c:func:`DEVICE_AND_API_INIT()`
   Create device object and set it up for boot time initialization.
   This also takes a pointer to driver API struct for link time
   pointer assignment.

:c:func:`DEVICE_NAME_GET()`
   Expands to the full name of a global device object.

:c:func:`DEVICE_GET()`
   Obtain a pointer to a device object by name.

:c:func:`DEVICE_DECLARE()`
   Declare a device object.
