.. _nanokernel_data:

Data Passing Services
#####################

This section contains the information about all data passing services available in the nanokernel.

Nanokernel FIFO
***************

Definition
==========


The FIFO object is defined in :file:`kernel/nanokernel/nano_fifo.c`.
This is a linked list of memory that allows the caller to store data of
any size. The data is stored in first-in-first-out order.

Function
========


Multiple processes can wait on the same FIFO object. Data is passed to
the first fiber that waited on the FIFO, and then to the background
task if no fibers are waiting. Through this mechanism the FIFO object
can synchronize or communicate between more than two contexts through
its API. Any ISR, fiber, or task can attempt to get data from a FIFO
without waiting on the data to be stored.

.. note::

   The FIFO object reserves the first WORD in each stored memory
   block as a link pointer to the next item. The size of the WORD
   depends on the platform and can be 16 bit, 32 bit, etc.

Application Program Interfaces
==============================

+--------------------------------+----------------------------------------------------------------+
| Context                        | Interfaces                                                     |
+================================+================================================================+
| **Initialization**             | :c:func:`nano_fifo_init()`                                     |
+--------------------------------+----------------------------------------------------------------+
| **Interrupt Service Routines** | :c:func:`nano_isr_fifo_get()`, :c:func:`nano_isr_fifo_put()`   |
+--------------------------------+----------------------------------------------------------------+
| **Fibers**                     | :c:func:`nano_fiber_fifo_get()`,                               |
|                                | :c:func:`nano_fiber_fifo_get_wait()`,                          |
|                                | :c:func:`nano_fiber_fifo_put()`                                |
+--------------------------------+----------------------------------------------------------------+
| **Tasks**                      | :c:func:`nano_task_fifo_get()`,                                |
|                                | :c:func:`nano_task_fifo_get_wait()`,                           |
|                                | :c:func:`nano_task_fifo_put()`                                 |
+--------------------------------+----------------------------------------------------------------+

Nanokernel LIFO Object
**********************

Definition
==========

The LIFO is defined in :file:`kernel/nanokernel/nano_lifo.c`. It
consists of a linked list of memory blocks that uses the first word in
each block as a next pointer. The data is stored in last-in-first-out
order.

Function
========

When a message is added to the LIFO, the data is stored at the head of
the list. Messages taken off the LIFO object are taken from the head.

The LIFO object requires the first 32-bit word to be empty in order to
maintain the linked list.

The LIFO object does not store information about the size of the
messages.

The LIFO object remembers one waiting context. When a second context
starts waiting for data from the same LIFO object, the first context
remains waiting and never reaches the runnable state.

Application Program Interfaces
==============================

+--------------------------------+----------------------------------------------------------------+
| Context                        | Interfaces                                                     |
+================================+================================================================+
| **Initialization**             | :c:func:`nano_lifo_init()`                                     |
+--------------------------------+----------------------------------------------------------------+
| **Interrupt Service Routines** | :c:func:`nano_isr_lifo_get()`,                                 |
|                                | :c:func:`nano_isr_lifo_put()`                                  |
+--------------------------------+----------------------------------------------------------------+
| **Fibers**                     | :c:func:`nano_fiber_lifo_get()`,                               |
|                                | :c:func:`nano_fiber_lifo_get_wait()`,                          |
|                                | :c:func:`nano_fiber_lifo_put()`                                |
+--------------------------------+----------------------------------------------------------------+
| **Tasks**                      | :c:func:`nano_task_lifo_get()`,                                |
|                                | :c:func:`nano_task_lifo_get_wait()`,                           |
|                                | :c:func:`nano_task_lifo_put()`                                 |
+--------------------------------+----------------------------------------------------------------+