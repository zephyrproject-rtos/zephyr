.. microkernel_api:

Microkernel APIs
################

.. contents::
   :depth: 1
   :local:
   :backlinks: top

Events
******

The microkernel's :dfn:`event` objects are an implementation of traditional
binary semaphores.

For an overview and important information about Events, see :ref:`microkernel_events`.

------

.. doxygengroup:: microkernel_event
   :project: Zephyr
   :content-only:

FIFOs
*****

The microkernel's :dfn:`FIFO` object type is an implementation of a traditional
first in, first out queue.
A FIFO allows tasks to asynchronously send and receive fixed-size data items.

For an overview and important information about FIFOs, see :ref:`microkernel_fifos`.

------

.. doxygengroup:: microkernel_fifo
   :project: Zephyr
   :content-only:

Pipes
*****

The microkernel's :dfn:`pipe` object type is an implementation of a traditional
anonymous pipe. A pipe allows a task to send a byte stream to another task.

For an overview and important information about Pipes, see :ref:`microkernel_pipes`.

------

.. doxygengroup:: microkernel_pipe
   :project: Zephyr
   :content-only:

Mailboxes
*********

The microkernel's :dfn:`mailbox` object type is an implementation of a
traditional message queue that allows tasks to exchange messages.

For an overview and important information about Mailboxes, see :ref:`microkernel_mailboxes`.

------

.. doxygengroup:: microkernel_mailbox
   :project: Zephyr
   :content-only:

Memory Maps
***********

The microkernel's memory map objects provide dynamic allocation and
release of fixed-size memory blocks.

For an overview and important information about Memory Maps, see :ref:`microkernel_memory_maps`.

------

.. doxygengroup:: microkernel_memorymap
   :project: Zephyr
   :content-only:

Memory Pools
************

The microkernel's :dfn:`memory pool` objects provide dynamic allocation and
release of variable-size memory blocks.

For an overview and important information about Memory Pools, see :ref:`microkernel_memory_pools`.

------

.. doxygengroup:: microkernel_memorypool
   :project: Zephyr
   :content-only:

Mutexes
*******

The microkernel's :dfn:`mutex` objects provide reentrant mutex
capabilities with basic priority inheritance.  A mutex allows
tasks to safely share a resource by ensuring mutual exclusivity
while the resource is being accessed by a task.

For an overview and important information about Mutexes, see :ref:`microkernel_mutexes`.

------

.. doxygengroup:: microkernel_mutex
   :project: Zephyr
   :content-only:

Semaphores
**********

The microkernel's :dfn:`semaphore` objects are an implementation of traditional
counting semaphores.

For an overview and important information about Semaphores, see :ref:`microkernel_semaphores`.

------

.. doxygengroup:: microkernel_semaphore
   :project: Zephyr
   :content-only:

Timers
******

A :dfn:`microkernel timer` allows a task to determine whether or not a
specified time limit has been reached while the task is busy performing
other work. The timer uses the kernel's system clock, measured in
ticks, to monitor the passage of time.

For an overview and important information about Timers, see :ref:`microkernel_timers`.

------

.. doxygengroup:: microkernel_timer
   :project: Zephyr
   :content-only:

Tasks
*****

A task is a preemptible thread of execution that implements a portion of
an application's processing. It is normally used for processing that
is too lengthy or too complex to be performed by a fiber or an ISR.

For an overview and important information about Tasks, see :ref:`microkernel_tasks`.

------

.. doxygengroup:: microkernel_task
   :project: Zephyr
   :content-only:
