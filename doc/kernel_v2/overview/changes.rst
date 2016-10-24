.. _changes_v2:

Changes from Version 1 Kernel
#############################

The version 2 kernel incorporates numerous changes
that improve ease of use for developers.
Some of the major benefits of these changes are:

* elimination of separate microkernel and nanokernel build types,
* elimination of the MDEF in microkernel-based applications,
* simplifying and streamlining the kernel API,
* easing restrictions on the use of kernel objects,
* reducing memory footprint by merging duplicated services, and
* improving performance by reducing context switching.

.. note::
    To ease the transition of existing applications and other Zephyr subsystems
    to the new kernel model, the revised kernel will continue to support
    the version 1 "legacy" APIs and MDEF for a limited period of time,
    after which support will be removed.

The changes introduced by the version 2 kernel are too numerous to fully
describe here; readers are advised to consult the individual sections of the
Kernel Primer to familiarize themselves with the way the version 2 kernel
operates. However, the most significant changes are summarized below.

Application Design
******************

The microkernel and nanokernel portions of Zephyr have been merged into
a single entity, which is simply referred to as "the kernel". Consequently,
there is now only a single way to design and build Zephyr applications.

The MDEF has been eliminated. All kernel objects are now defined directly
in code.

Multi-threading
***************

The task and fiber context types have been merged into a single type,
known as a "thread". Setting a thread's priority to a negative priority
makes it a "cooperative thread", which operates in a fiber-like manner;
setting it to a non-negative priority makes it a "preemptive thread",
which operates in a task-like manner.

It is now possible to pass up to 3 arguments to a thread's entry point.
(The version 1 kernel allowed 2 arguments to be passed to a fiber
and allowed no arguments to be passed to a task.)

The kernel now spawns both a "main thread" and an "idle thread" during
startup. (The version 1 kernel spawned only a single thread.)

The kernel's main thread performs system initialization and then invokes
:cpp:func:`main()`. If no :cpp:func:`main()` is defined by the application,
the main thread terminates.

System initialization code can now perform blocking operations,
during which time the kernel's idle thread executes.

Kernel APIs
***********

Kernel APIs now use a **k_** or **K_** prefix. There are no longer distinct
APIs for invoking a service from an task, a fiber, and an ISR.

Kernel APIs now return 0 to indicate success and a non-zero error code
to indicate the reason for failure. (The version 1 kernel supported only
two error codes, rather than an unlimited number of them.)

Kernel APIs now specify timeout intervals in milliseconds, rather than
in system clock ticks. (This change makes things more intuitive for most
developers. However, the kernel still implements timeouts using the
tick-based system clock.)

Kernel objects can now be used by both task-like threads and fiber-like
threads. (The version 1 kernel did not permit fibers to use microkernel
objects, and could result in undesirable busy-waiting when nanokernel
objects were used by tasks.)

Kernel objects now typically allow multiple threads to wait on a given
object. (The version 1 kernel restricted waiting on certain types of
kernel object to a single thread.)

Kernel object APIs now always execute in the context of the invoking thread.
(The version 1 kernel required microkernel object APIs to context switch
the thread to the microkernel server fiber, followed by another context
switch back to the invoking thread.)

Clocks and Timers
*****************

The nanokernel timer and microkernel timer object types have been merged
into a single type.

Synchronization
***************

The nanokernel semaphore and microkernel semaphore object types have been
merged into a single type. The new type can now be used as a binary semaphore,
as well as a counting semaphore.

The microkernel event object type is now presented as a relative to Unix-style
signalling. Due to improvements to the implementation of semaphores, events
are now less efficient to use for basic synchronization than semaphores;
consequently, events should now be reserved for scenarios requiring the use
of a callback function.

Data Passing
************

The microkernel FIFO object type has been renamed to "message queue",
to avoid confusion with the nanokernel FIFO object type.

The microkernel mailbox object type no longer supports the explicit message
priority concept. Messages are now implicitly ordered based on the priority
of the sending thread.

The microkernel mailbox object type now supports the sending of asynchronous
messages using a message buffer. (The version 1 kernel only supported
asynchronous messages using a message block.)

The microkernel memory map object has been renamed to "memory slab", to better
reflect its management of equal-size memory blocks.

Task Groups
***********

There is no k_thread_group_xxx() equivalent to the legacy task_group_xxx()
APIs as task groups are being phased out. Use of the legacy task_group_xxx()
APIs are limited to statically defined threads.
