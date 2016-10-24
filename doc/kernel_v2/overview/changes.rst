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

The task and fiber context types have been merged into a single type,
known as a "thread". Setting a thread's priority to a negative priority
makes it a "cooperative thread", which operates in a fiber-like manner;
setting it to a non-negative priority makes it a "preemptive thread",
which operates in a task-like manner.

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

The MDEF has been eliminated. Consequently, all kernel objects are now defined
directly in code.

Kernel APIs
***********

Most kernel APIs have been renamed or have had changes to their arguments
(or both) to make them more intuitive, and to improve consistency.
The **k_** and **K_** prefixes are now used by most kernel APIs.

A version 1 kernel operation that can be invoked from a task, a fiber,
or an ISR using distinct APIs is now invoked from a thread or an ISR
using a single common API.

Many kernel APIs now return 0 to indicate success and a non-zero error code
to indicate the reason for failure. (The version 1 kernel supported only
two error codes, rather than an unlimited number of them.)

Threads
*******

A task-like thread can now make itself temporarily non-preemptible
by locking the kernel's scheduler (rather than by locking interrupts).

It is now possible to pass up to 3 arguments to a thread's entry point.
(The version 1 kernel allowed 2 arguments to be passed to a fiber
and allowed no arguments to be passed to a task.)

It is now possible to delay the start of a statically-defined threads.
(The version 1 kernel only permitted delaying of fibers spawned at run time.)

A task can no longer specify an "task abort handler" function
that is invoked automatically when the task terminates or aborts.

An application can no longer use "task groups" to alter the operation
of a set of related tasks by invoking a single kernel API.
However, applications can provide their own APIs to achieve a similar effect.

The kernel now spawns both a "main thread" and an "idle thread" during
startup. (The version 1 kernel spawned only a single thread.)

The kernel's main thread performs system initialization and then invokes
:cpp:func:`main()`. If no :cpp:func:`main()` is defined by the application,
the main thread terminates.

System initialization code can now perform blocking operations,
during which time the kernel's idle thread executes.

Timing
******

Most kernel APIs now specify timeout intervals in milliseconds, rather than
in system clock ticks. This change makes things more intuitive for most
developers. However, the kernel still implements timeouts using the
tick-based system clock.

The nanokernel timer and microkernel timer object types have been merged
into a single type.

Memory Allocation
*****************

The microkernel memory map object has been renamed to "memory slab", to better
reflect its management of equal-size memory blocks.

It is now possible to specify the alignment used by the memory blocks
belonging to a memory slab or a memory pool.

It is now possible to define a memory pool directly in code.

It is now possible to allocate and free memory in a malloc()-like manner
from a heap data pool.

Synchronization
***************

The nanokernel semaphore and microkernel semaphore object types have been
merged into a single type. The new type can now be used as a binary semaphore,
as well as a counting semaphore.

An application can no longer use a "semaphore group" to allow a thread to wait
on multiple semaphores simultaneously. Until the kernel incorporates a
:cpp:func:`select()` or :cpp:func:`poll()` capability an application wishing
to wait on multiple semaphores must either test them individually in a
non-blocking manner or use an additional mechanism, such as an event object,
to signal the application that one of the semaphores is available.

The microkernel event object type is renamed to "alert" and is now presented as
a relative to Unix-style signalling. Due to improvements to the implementation
of semaphores, alerts are now less efficient to use for basic synchronization
than semaphores; consequently, alerts should now be reserved for scenarios
requiring the use of a callback function.

Data Passing
************

The microkernel FIFO object type has been renamed to "message queue",
to avoid confusion with the nanokernel FIFO object type.

It is now possible to specify the alignment used by the data items
stored in a message queue (aka microkernel FIFO).

The microkernel mailbox object type no longer supports the explicit message
priority concept. Messages are now implicitly ordered based on the priority
of the sending thread.

The microkernel mailbox object type now supports the sending of asynchronous
messages using a message buffer. (The version 1 kernel only supported
asynchronous messages using a message block.)

It is now possible to specify the alignment used by a microkernel pipe object's
buffer.
