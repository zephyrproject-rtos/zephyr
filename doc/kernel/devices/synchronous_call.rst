.. _synchrounous_call:

Synchronous call within interrupt based device drivers
######################################################

Zephyr provides a set of device drivers for various platforms. As a preemptive
OS, those are meant to be implemented through an interrupt based mode and not
a polling mode unless the driven hardware does not provide any interrupt
enabled mechanism.

The higher level calls accessed through devices's specific API, such as i2c.h
or spi.h, are meant to be, most of the time, synchronous. Thus these calls
should be blocking.

However, and due to the fact Zephyr provides 2 types of executions context on
a microkernel, drivers need to act accordingly. A nanokernel semaphore cannot
be used if the context is a task for instance. Thus, include/device.h exposes
an helper API to handle this case transparently, and no other means should be
used instead.

Usage of device_sync_call_* API
*******************************

1 type and 3 inline functions are exposed to solve this issue.

The device_sync_call_t type
===========================

It provides a nanokernel semaphore, always present in both built types:
nanokernel and microkernel. It is meant to be used within a fiber context.
Then, and only on a microkernel type, it will provide a kernel semaphore
meant to be used within a task context. A boolean marker is used to remember
the caller's context when running a microkernel.

device_sync_call_init()
=======================

It initializes the device_sync_call_t type: its semaphores and the marker.
Such function should be used only once in the device driver's instance life
time. Thus the driver's initialization function is the best place for calling
it.

device_sync_call_wait()
=======================

This functions will block - i.e. will perform a "take wait" - on the relevant
semaphore. This will make the exposed driver's API function using it a blocking
function, until the relevant semaphore is released by a give. This is therefore
used to start a synchronous call, until being signaled for synchronization.

device_sync_call_complete()
===========================

This function is used to release the relevant semaphore and thus will unlock
the blocking function. This is thus being called in the driver's ISR handler
mostly. This is used to signal the completion of the synchronous call, whatever
fate would be (error or success).
