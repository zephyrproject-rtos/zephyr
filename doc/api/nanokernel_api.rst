.. nanokernel_api:

Nanokernel APIs
###############

.. contents::
   :depth: 1
   :local:
   :backlinks: top

Fibers
******

A :dfn:`fiber` is a lightweight, non-preemptible thread of execution that
implements a portion of an application's processing. Fibers are often
used in device drivers and for performance-critical work.

For an overview and important information about Fibers, see :ref:`nanokernel_fibers`.

------

.. doxygengroup:: nanokernel_fiber
   :project: Zephyr
   :content-only:

Tasks
******

A :dfn:`task` is a preemptible thread of execution that implements a portion of
an application's processing. It is normally used to perform processing that is
too lengthy or too complex to be performed by a fiber or an ISR.

For an overview and important information about Tasks, see :ref:`nanokernel_tasks`.

------

.. doxygengroup:: nanokernel_task
   :project: Zephyr
   :content-only:

Semaphores
**********

The nanokernel's :dfn:`semaphore` object type is an implementation of a
traditional counting semaphore. It is mainly intended for use by fibers.

For an overview and important information about Semaphores, see :ref:`nanokernel_synchronization`.

------

.. doxygengroup:: nanokernel_semaphore
   :project: Zephyr
   :content-only:

LIFOs
*****

The nanokernel's LIFO object type is an implementation of a traditional
last in, first out queue. It is mainly intended for use by fibers.

For an overview and important information about LIFOs, see :ref:`nanokernel_lifos`.

------

.. doxygengroup:: nanokernel_lifo
   :project: Zephyr
   :content-only:

FIFOs
*****

The nanokernel's FIFO object type is an implementation of a traditional
first in, first out queue. It is mainly intended for use by fibers.

For an overview and important information about FIFOs, see :ref:`nanokernel_fifos`.

------

.. doxygengroup:: nanokernel_fifo
   :project: Zephyr
   :content-only:

Ring Buffers
************

The ring buffer is an array-based
circular buffer, stored in first-in-first-out order. Concurrency control of
ring buffers is not implemented at this level.

For an overview and important information about ring buffers, see :ref:`nanokernel_ring_buffers`.

------

.. doxygengroup:: nanokernel_ringbuffer
   :project: Zephyr
   :content-only:

Stacks
******

The nanokernel's stack object type is an implementation of a traditional
last in, first out queue for a limited number of 32-bit data values.
It is mainly intended for use by fibers.

For an overview and important information about stacks, see :ref:`nanokernel_stacks`.

------

.. doxygengroup:: nanokernel_stack
   :project: Zephyr
   :content-only:

Timers
******

The nanokernel's :dfn:`timer` object type uses the kernel's system clock to
monitor the passage of time, as measured in ticks. It is mainly intended for use
by fibers.

For an overview and important information about timers, see :ref:`nanokernel_timers`.

------

.. doxygengroup:: nanokernel_timer
   :project: Zephyr
   :content-only:

Kernel Event Logger
*******************

The kernel event logger is a standardized mechanism to record events within the
Kernel while providing a single interface for the user to collect the data.

For an overview and important information about the kernel event logger API,
see :ref:`nanokernel_event_logger`.

------

.. doxygengroup:: nanokernel_event_logger
   :project: Zephyr
   :content-only:
