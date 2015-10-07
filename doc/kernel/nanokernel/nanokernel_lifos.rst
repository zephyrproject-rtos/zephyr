.. _nanokernel_lifos:

Nanokernel LIFOs
################

Concepts
********

The nanokernel's LIFO object type is an implementation of a traditional
last in, first out queue. It is mainly intended for use by fibers.

A nanokernel LIFO allows data items of any size tasks to be sent and received
asynchronously. The LIFO uses a linked list to hold data items that have been
sent but not yet received.

LIFO data items must be aligned on a 4-byte boundary, as the kernel reserves
the first 32 bits of each item for use as a pointer to the next data item
in the LIFO's linked list. Consequently, a data item that holds N bytes
of application data requires N+4 bytes of memory.

Any number of nanokernel LIFOs can be defined. Each LIFO is a distinct
variable of type :cpp:type:`struct nano_lifo`, and is referenced using a
pointer to that variable. A LIFO must be initialized before it can be used to
send or receive data items.

Items can be added to a nanokernel LIFO in a non-blocking manner by any
context type (i.e. ISR, fiber, or task).

Items can be removed from a nanokernel LIFO in a non-blocking manner by any
context type; if the LIFO is empty the :c:macro:`NULL` return code
indicates that no item was removed. Items can also be removed from a
nanokernel LIFO in a blocking manner by a fiber or task; if the LIFO is empty
the thread waits for an item to be added.

Any number of threads may wait on an empty nanokernel LIFO simultaneously.
When a data item becomes available it is given to the fiber that has waited
longest, or to a waiting task if no fiber is waiting.

.. note::
   A task that waits on an empty nanokernel LIFO does a busy wait. This is
   not an issue for a nanokernel application's background task; however, in
   a microkernel application a task that waits on a nanokernel LIFO remains
   the current task. In contrast, a microkernel task that waits on a
   microkernel data passing object ceases to be the current task, allowing
   other tasks of equal or lower priority to do useful work.

   If multiple tasks in a microkernel application wait on the same nanokernel
   LIFO, higher priority tasks are given data items in preference to lower
   priority tasks. However, the order in which equal priority tasks are given
   data items is unpredictable.

Purpose
*******

Use a nanokernel LIFO to asynchronously transfer data items of arbitrary size
in a "last in, first out" manner.

Usage
*****

Example: Initializing a Nanokernel LIFO
=======================================

This code establishes an empty nanokernel LIFO.

.. code-block:: c

   struct nano_lifo  signal_lifo;

   nano_lifo_init(&signal_lifo);

Example: Writing to a Nanokernel LIFO from a Fiber
==================================================

This code uses a nanokernel LIFO to send data to a consumer fiber.

.. code-block:: c

   struct data_item_t {
       void *lifo_reserved;   /* 1st word reserved for use by LIFO */
       ...
   };

   struct data_item_t  tx data;

   void producer_fiber(int unused1, int unused2)
   {
       ARG_UNUSED(unused1);
       ARG_UNUSED(unused2);

       while (1) {
           /* create data item to send */
           tx_data = ...

           /* send data to consumer */
           nano_fiber_lifo_put(&signal_lifo, &tx_data);

           ...
       }
   }

Example: Reading from a Nanokernel LIFO
=======================================

This code uses a nanokernel LIFO to obtain data items from a producer fiber,
which are then processed in some manner.

.. code-block:: c

   void consumer_fiber(int unused1, int unused2)
   {
       struct data_item_t  *rx_data;

       ARG_UNUSED(unused1);
       ARG_UNUSED(unused2);

       while (1) {
           rx_data = nano_fiber_lifo_get_wait(&signal_lifo);
           /* process LIFO data */
           ...
       }
   }

APIs
****

The following APIs for a nanokernel LIFO are provided by :file:`nanokernel.h`:

:cpp:func:`nano_lifo_init()`
   Initializes a LIFO.

:cpp:func:`nano_task_lifo_put()`, :cpp:func:`nano_fiber_lifo_put()`,
:cpp:func:`nano_isr_lifo_put()`
   Add an item to a LIFO.

:cpp:func:`nano_task_lifo_get()`, :cpp:func:`nano_fiber_lifo_get()`,
:cpp:func:`nano_isr_lifo_get()`
   Remove an item from a LIFO, or fails and continues if it is empty.

:cpp:func:`nano_task_lifo_get_wait()`, :cpp:func:`nano_fiber_lifo_get_wait()`
   Remove an item from a LIFO, or waits for an item if it is empty.

:cpp:func:`nano_task_lifo_get_wait_timeout()`,
:cpp:func:`nano_fiber_lifo_get_wait_timeout()`
   Remove an item from a LIFO, or waits for an item for a specified time
   period if it is empty.