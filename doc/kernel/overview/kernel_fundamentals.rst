.. _kernel_fundamentals:

Kernel Fundamentals
###################

This section provides a high-level overview of the concepts and capabilities
of the Zephyr kernel.

Organization
************

The central elements of the Zephyr kernel are its *microkernel* and underlying
*nanokernel*. The kernel also contains a variety of auxiliary subsystems,
including a library of device drivers and networking software.

Applications can be developed using both the microkernel and the nanokernel,
or using the nanokernel only.

The nanokernel is a high-performance, multi-threaded execution environment
with a basic set of kernel features. The nanokernel is ideal for systems
with sparse memory (the kernel itself requires as little as 2 KB!) or only
simple multi-threading requirements (such as a set of interrupt
handlers and a single background task). Examples of such systems include:
embedded sensor hubs, environmental sensors, simple LED wearables, and
store inventory tags.

The microkernel supplements the capabilities of the nanokernel to provide
a richer set of kernel features. The microkernel is suitable for systems
with heftier memory (50 to 900 KB), multiple communication devices
(like WIFI and Bluetooth Low Energy), and multiple data processing tasks.
Examples of such systems include: fitness wearables, smart watches, and
IoT wireless gateways.

Related sections:

* :ref:`common_kernel_services`
* :ref:`nanokernel`
* :ref:`microkernel`

Multi-threading
***************

The Zephyr kernel supports multi-threaded processing for three types
of execution contexts.

* A **task context** is a preemptible thread, normally used to perform
  processing that is lengthy or complex. Task scheduling is priority-based,
  so that the execution of a higher priority task preempts the execution
  of a lower priority task. The kernel also supports an optional round-robin
  time slicing capability so that equal priority tasks can execute in turn,
  without the risk of any one task monopolizing the CPU.

* A **fiber context** is a lightweight and non-preemptible thread, typically
  used for device drivers and performance critical work. Fiber scheduling is
  priority-based, so that a higher priority fiber is scheduled for execution
  before a lower priority fiber; however, once a fiber is scheduled it remains
  scheduled until it performs an operation that blocks its own execution.
  Fiber execution takes precedence over task execution, so tasks execute only
  when no fiber can be scheduled.

* The **interrupt context** is a special kernel context used to execute
  :abbr:`ISRs Interrupt Service Routines`. The interrupt context takes
  precedence over all other contexts, so tasks and fibers execute only
  when no ISR needs to run. (See below for more on interrupt handling.)

The Zephyr microkernel does not limit the number of tasks or fibers used
in an application; however, an application that uses only the nanokernel
is limited to a single task.

Related sections:

* :ref:`Nanokernel Fiber Services <nanokernel_fibers>`
* :ref:`Microkernel Task Services <microkernel_tasks>`

Interrupts
**********

The Zephyr kernel supports the handling of hardware interrupts and software
interrupts by interrupt handlers, also known as ISRs. Interrupt handling takes
precedence over task and fiber processing, so that an ISR preempts the currently
scheduled task or fiber whenever it needs to run. The kernel also supports nested
interrupt handling, allowing a higher priority ISR to interrupt the execution of
a lower priority ISR, should the need arise.

The nanokernel supplies ISRs for a few interrupt sources (IRQs), such as the
hardware timer device and system console device. The ISRs for all other IRQs
are supplied by either device drivers or application code. Each ISR can
be registered with the kernel at compile time, but can also be registered
dynamically once the kernel is up and running. Zephyr supports ISRs that
are written entirely in C, but also permits the use of assembly language.

In situations where an ISR cannot complete the processing of an interrupt in a
timely manner by itself, the kernel's synchronization and data passing mechanisms
can hand off the remaining processing to a fiber or task. The microkernel provides
a *task IRQ* object type that streamlines the handoff to a task in a manner that
does not require the device driver or application code to supply an ISR at all.

Related sections:

* :ref:`Nanokernel Interrupt Services <nanokernel_interrupts>`
* :ref:`Microkernel Interrupt Services <microkernel_task_irqs>`

Clocks and Timers
*****************

Kernel clocking is based on time units called :dfn:`ticks` which have a
configurable duration. A 64-bit *system clock* counts the number of ticks
that have elapsed since the kernel started executing.

Zephyr also supports a higher-resolution *hardware clock*, which can be used
to measure durations requiring sub-tick interval precision.

The nanokernel allows a fiber or thread to perform time-based processing
based on the system clock. This can be done either by using a nanokernel API
that supports a *timeout* argument, or by using a *timer* object that can
be set to expire after a specified number of ticks.

The microkernel also allows tasks to perform time-based processing using
timeouts and timers. Microkernel timers have additional capabilities
not provided by nanokernel timers, such as a periodic expiration mode.

Related sections:

* :ref:`kernel_clocks`
* :ref:`Nanokernel Timer Services <nanokernel_timers>`
* :ref:`Microkernel Timers Services <microkernel_timers>`

Synchronization
***************

The Zephyr kernel provides four types of objects that allow different
contexts to synchronize their execution.

The microkernel provides the object types listed below. These types
are intended for tasks, with limited ability to be used by fibers and ISRs.

* A :dfn:`semaphore` is a counting semaphore, which indicates how many units
  of a particular resource are available.

* An :dfn:`event` is a binary semaphore, which simply indicates if a particular
  resource is available or not.

* A :dfn:`mutex` is a reentrant mutex with priority inversion protection. It is
  similar to a binary semaphore, but contains additional logic to ensure that
  only the owner of the associated resource can release it and to expedite the
  execution of a lower priority thread that holds a resource needed by a
  higher priority thread.

The nanokernel provides the object type listed below. This type
is intended for fibers, with only limited ability to be used by tasks and ISRs.

* A :dfn:`nanokernel semaphore` is a counting semaphore that indicates
  how many units of a particular resource are available.

Each type has specific capabilities and limitations that affect suitability
of use in a given situation. For more details, see the documentation for each
specific object type.

Related sections:

* :ref:`Microkernel Synchronization Services <microkernel_synchronization>`
* :ref:`Nanokernel Synchronization Services <nanokernel_synchronization>`

Data Passing
************

The Zephyr kernel provides six types of objects that allow different
contexts to exchange data.

The microkernel provides the object types listed below. These types are
designed to be used by tasks, and cannot be used by fibers and ISRs.

* A :dfn:`microkernel FIFO` is a queuing mechanism that allows tasks to exchange
  fixed-size data items in an asychronous :abbr:`First In, First Out (FIFO)`
  manner.

* A :dfn:`mailbox` is a queuing mechanism that allows tasks to exchange
  variable-size data items in a synchronous, "first in, first out" manner.
  Mailboxes also support asynchronous exchanges, and allow tasks to exchange
  messages both anonymously and non-anonymously using the same mailbox.

* A :dfn:`pipe` is a queuing mechanism that allows a task to send a stream
  of bytes to another task. Both asynchronous and synchronous exchanges
  can be supported by a pipe.

The nanokernel provides the object types listed below. These types are
primarily designed to be used by fibers, and have only a limited ability
to be used by tasks and ISRs.

* A :dfn:`nanokernel FIFO` is a queuing mechanism that allows contexts to exchange
  variable-size data items in an asychronous, first-in, first-out manner.

* A :dfn:`nanokernel LIFO` is a queuing mechanism that allows contexts to exchange
  variable-size data items in an asychronous, last-in, first-out manner.

* A :dfn:`nanokernel stack` is a queuing mechanism that allows contexts to exchange
  32-bit data items in an asynchronous first-in, first-out manner.

Each of these types has specific capabilities and limitations that affect
suitability for use in a given situation. For more details, see the
documentation for each specific object type.

Related sections:

* :ref:`Microkernel Data Passing Services <microkernel_data>`
* :ref:`Nanokernel Data Passing Services <nanokernel_data>`

Dynamic Memory Allocation
*************************

The Zephyr kernel requires all system resources to be defined at compile-time,
and therefore provides only limited support for dynamic memory allocation.
This support can be used in place of C standard library calls like
:c:func:`malloc()` and :c:func:`free()`, albeit with certain differences.

The microkernel provides two types of objects that allow tasks to dynamically
allocate memory blocks. These object types cannot be used by fibers or ISRs.

* A :dfn:`memory map` is a memory region that supports the dynamic allocation and
  release of memory blocks of a single fixed size. An application can have
  multiple memory maps, whose block size and block capacity can be configured
  individually.

* A :dfn:`memory pool` is a memory region that supports the dynamic allocation and
  release of memory blocks of multiple fixed sizes. This allows more efficient
  use of available memory when an application requires blocks of different
  sizes. An application can have multiple memory pools, whose block sizes
  and block capacity can be configured individually.

The nanokernel does not provide any support for dynamic memory allocation.

For additional information see:

* :ref:`Microkernel Memory Maps <memory_maps>`
* :ref:`Microkernel Pools <memory_pools>`

Public and Private Microkernel Objects
**************************************

Microkernel objects, such as semaphores, mailboxes, or tasks,
can usually be defined as a public object or a private object.

* A :dfn:`public object` is one that is available for general use by all parts
  of the application. Any code that includes :file:`zephyr.h` can interact
  with the object by referencing the object's name.

* A :dfn:`private object` is one that is intended for use only by a specific
  part of the application, such as a single device driver or subsystem.
  The object's name is visible only to code within the file where the object
  is defined, hiding it from general use unless the code which defined the
  object takes additional steps to share the name with other files.

Aside from the way they are defined, and the resulting visibility of
the object's name, a public object and a private object of the same type
operate in exactly the same manner using the same set of APIs.

In most cases, the decision to make a given microkernel object a public
object or a private object is simply a matter of convenience. For example,
when defining a server-type subsystem that handles requests from multiple
clients, it usually makes sense to define public objects.

.. note::
   Nanokernel object types can only be defined as private objects. This means
   a nanokernel object must be defined using a global variable to allow it to
   be accessed by code outside the file in which the object is defined.


.. _microkernel_server:

Microkernel Server
******************

The microkernel performs most operations involving microkernel objects
using a special *microkernel server* fiber, called :c:func:`_k_server`.

When a task invokes an API associated with a microkernel object type,
such as :c:func:`task_fifo_put()`, the associated operation is not
carried out directly. Instead, the following sequence of steps typically
occurs:

#. The task creates a *command packet*, which contains the input parameters
   needed to carry out the desired operation.

#. The task queues the command packet on the microkernel server's
   *command stack*. The kernel then preempts the task and schedules
   the microkernel server.

#. The microkernel server dequeues the command packet from its command
   stack and performs the desired operation. All output parameters for the
   operation, such as the return code, are saved in the command packet.

#. When the operation is complete the microkernel server attempts
   to fetch a command packet from its now empty command stack
   and becomes blocked. The kernel then schedules the requesting task.

#. The task processes the command packet's output parameters to determine
   the results of the operation.

The actual sequence of steps may vary from the above guideline in some
instances. For example, if the operation causes a higher-priority task
to become runnable, the requesting task is not scheduled for execution by
the kernel until *after* the higher priority task is first scheduled.
In addition, a few operations involving microkernel objects do not require
the use of a command packet at all.

While this indirect execution approach may seem somewhat inefficient,
it actually has a number of important benefits:

* All operations performed by the microkernel server are inherently free
  from race conditions; operations are processed serially by a single fiber
  that cannot be preempted by tasks or other fibers. This means the
  microkernel server can manipulate any of the microkernel objects in the
  system during any operation without having to take additional steps
  to prevent interference by other contexts.

* Microkernel operations have minimal impact on interrupt latency;
  interrupts are never locked for a significant period to prevent race
  conditions.

* The microkernel server can easily decompose complex operations into two or
  more simpler operations by creating additional command packets and queueing
  them on the command stack.

* The overall memory footprint of the system is reduced; a task using microkernel
  objects only needs to provide stack space for the first step of the above sequence,
  rather than for all steps required to perform the operation.

For additional information see:

* :ref:`Microkernel Server Fiber <microkernel_server_fiber>`

Standard C Library
******************

The Zephyr kernel currently provides only the minimal subset of the
standard C library required to meet the kernel's own needs, primarily
in the areas of string manipulation and display.

Applications that require a more extensive C library can either submit
contributions that enhance the existing library or substitute
a replacement library.

Device Driver Library
*********************

The Zephyr kernel supports a variety of device drivers. The specific set of
device drivers available for an application's platform configuration varies
according to the associated hardware components and device driver software.

Device drivers which are present on all supported platform configurations
are listed below.

* **Interrupt controller**: This device driver is used by the nanokernel's
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

Networking Library
******************

[This section briefly outlines the networking subsystems.]
