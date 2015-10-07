.. _nanokernel_fifos:

Nanokernel FIFOs
################

Concepts
********

The nanokernel's FIFO object type is an implementation of a traditional
first in, first out queue. It is mainly intended for use by fibers.

A nanokernel FIFO allows data items of any size tasks to be sent and received
asynchronously. The FIFO uses a linked list to hold data items that have been
sent but not yet received.

FIFO data items must be aligned on a 4-byte boundary, as the kernel reserves
the first 32 bits of each item for use as a pointer to the next data item
in the FIFO's linked list. Consequently, a data item that holds N bytes
of application data requires N+4 bytes of memory.

Any number of nanokernel FIFOs can be defined. Each FIFO is a distinct
variable of type :cpp:type:`struct nano_fifo`, and is referenced using a
pointer to that variable. A FIFO must be initialized before it can be used to
send or receive data items.

Items can be added to a nanokernel FIFO in a non-blocking manner by any
context type (i.e. ISR, fiber, or task).

Items can be removed from a nanokernel FIFO in a non-blocking manner by any
context type; if the FIFO is empty the :c:macro:`NULL` return code
indicates that no item was removed. Items can also be removed from a
nanokernel FIFO in a blocking manner by a fiber or task; if the FIFO is empty
the thread waits for an item to be added.

Any number of threads may wait on an empty nanokernel FIFO simultaneously.
When a data item becomes available it is given to the fiber that has waited
longest, or to a waiting task if no fiber is waiting.

.. note::
   A task that waits on an empty nanokernel FIFO does a busy wait. This is
   not an issue for a nanokernel application's background task; however, in
   a microkernel application a task that waits on a nanokernel FIFO remains
   the current task. In contrast, a microkernel task that waits on a
   microkernel data passing object ceases to be the current task, allowing
   other tasks of equal or lower priority to do useful work.

   If multiple tasks in a microkernel application wait on the same nanokernel
   FIFO, higher priority tasks are given data items in preference to lower
   priority tasks. However, the order in which equal priority tasks are given
   data items is unpredictible.

Purpose
*******

Use a nanokernel FIFO to asynchronously transfer data items of arbitrary size
in a "first in, first out" manner.

Usage
*****

Example: Initializing a Nanokernel FIFO
=======================================

This code establishes an empty nanokernel FIFO.

.. code-block:: c

   struct nano_fifo  signal_fifo;

   nano_fifo_init(&signal_fifo);

Example: Writing to a Nanokernel FIFO from a Fiber
==================================================

This code uses a nanokernel FIFO to send data to one or more consumer fibers.

.. code-block:: c

   struct data_item_t {
      void *fifo_reserved;   /* 1st word reserved for use by FIFO */
      ...
   };

   struct data_item_t  tx_data;

   void producer_fiber(int unused1, int unused2)
   {
       ARG_UNUSED(unused1);
       ARG_UNUSED(unused2);

       while (1) {
           /* create data item to send (e.g. measurement, timestamp, ...) */
           tx_data = ...

           /* send data to consumers */
           nano_fiber_fifo_put(&signal_fifo, &tx_data);

           ...
       }
   }

Example: Reading from a Nanokernel FIFO
=======================================

This code uses a nanokernel FIFO to obtain data items from a producer fiber,
which are then processed in some manner. This design supports the distribution
of data items to multiple consumer fibers, if desired.

.. code-block:: c

   void consumer_fiber(int unused1, int unused2)
   {
       struct data_item_t  *rx_data;

       ARG_UNUSED(unused1);
       ARG_UNUSED(unused2);

       while (1) {
           rx_data = nano_fiber_fifo_get_wait(&signal_fifo);

           /* process FIFO data */
           ...
       }
   }

APIs
****

The following APIs for a nanokernel FIFO are provided by :file:`nanokernel.h`:

:cpp:func:`nano_fifo_init()`
   Initializes a FIFO.

:cpp:func:`nano_task_fifo_put()`, :cpp:func:`nano_fiber_fifo_put()`,
:cpp:func:`nano_isr_fifo_put()`, :cpp:func:`nano_fifo_put()`
   Add an item to a FIFO.

:cpp:func:`nano_task_fifo_get()`, :cpp:func:`nano_fiber_fifo_get()`,
:cpp:func:`nano_isr_fifo_get()`, :cpp:func:`nano_fifo_get()`
   Remove an item from a FIFO, or fails and continues if it is empty.

:cpp:func:`nano_task_fifo_get_wait()`, :cpp:func:`nano_fiber_fifo_get_wait()`,
:cpp:func:`nano_fifo_get_wait()`
   Remove an item from a FIFO, or waits for an item if it is empty.

:cpp:func:`nano_task_fifo_get_wait_timeout()`,
:cpp:func:`nano_fiber_fifo_get_wait_timeout()`,
:cpp:func:`nano_fifo_get_wait_timeout()`
   Remove an item from a FIFO, or waits for an item for a specified time
   period if it is empty.