.. _synchronous_call:

Synchronous Calls
*****************

Zephyr provides a set of device drivers for multiple platforms. Each driver
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
   marker. Such function should be used only once in the device driver's instance
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
   used to signal the completion of the synchronous call, whatever fate would be
   (error or success).
