.. _device_drivers:

Device Drivers and Device Model
###############################

Introduction
************
The Zephyr kernel supports a variety of device drivers. The specific set of
device drivers available for an application's board configuration varies
according to the associated hardware components and device driver software.

The Zephyr device model provides a consistent device model for configuring the
drivers that are part of a system. The device model is responsible
for initializing all the drivers configured into the system.

Each type of driver (UART, SPI, I2C) is supported by a generic type API.

In this model the driver fills in the pointer to the structure containing the
function pointers to its API functions during driver initialization. These
structures are placed into the RAM section in initialization level order.

Standard Drivers
****************

Device drivers which are present on all supported board configurations
are listed below.

* **Interrupt controller**: This device driver is used by the kernel's
  interrupt management subsystem.

* **Timer**: This device driver is used by the kernel's system clock and
  hardware clock subsystem.

* **Serial communication**: This device driver is used by the kernel's
  system console subsystem.

* **Random number generator**: This device driver provides a source of random
  numbers.

  .. important::

    Certain implementations of this device driver do not generate sequences of
    values that are truly random.

Synchronous Calls
*****************

Zephyr provides a set of device drivers for multiple boards. Each driver
should support an interrupt-based implementation, rather than polling, unless
the specific hardware does not provide any interrupt.

High-level calls accessed through devices's specific API, such as i2c.h
or spi.h, are usually intended as synchronous. Thus, these calls should be
blocking.

Due to the fact that Zephyr provides two types of execution contexts (task
and fiber) on a microkernel, drivers need to act accordingly. For example, a
nanokernel semaphore cannot be used when the context is a task, so the
:file:`include/device.h` exposes a helper API to handle this case transparently;
no other means ought to be used instead.

Zephyr API exposes 1 type and 3 inline functions to solve this issue.

``device_sync_call_t``

   This type provides a nanokernel semaphore, always present in both nanokernel
   and microkernel use cases. It is meant to be used within a fiber context.
   Then, and only on a microkernel type, it will provide a kernel semaphore
   meant to be used within a task context. A boolean marker is used to remember
   the caller's context when running a microkernel.

:cpp:func:`device_sync_call_init()`

   This function initializes the ``device_sync_call_t`` type semaphores and the
   marker. This function should be used only once in the device driver's instance
   lifetime. Thus, the driver's initialization function is the best place for
   calling it.

:cpp:func:`device_sync_call_wait()`

   This function will block - that is, it will perform a "take wait" - on the
   relevant semaphore. The exposed driver's API function can then be used as a
   blocking function until the relevant semaphore is released by a ``give``.
   This is therefore used to start a synchronous call, and waits until being
   signaled for synchronization.

:cpp:func:`device_sync_call_complete()`

   This function releases the relevant semaphore and thus will unlock the blocking
   function. Most frequently will it be called in the driver's ISR handler. It is
   used to signal the completion of the synchronous call (error or success).

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
