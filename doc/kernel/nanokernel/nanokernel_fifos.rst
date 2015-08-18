.. _nanokernel_fifos:

Nanokernel FIFOs
################

Definition
**********

The FIFO object is defined in :file:`kernel/nanokernel/nano_fifo.c`.
This is a linked list of memory that allows the caller to store data of
any size. The data is stored in first-in-first-out order.

Function
********

Multiple fibers can wait on the same FIFO object. Data is passed to
the first fiber that waited on the FIFO, and then to the background
task if no fibers are waiting. Through this mechanism the FIFO object
can synchronize or communicate between more than two contexts through
its API. Any ISR, fiber, or task can attempt to get data from a FIFO
without waiting on the data to be stored.

.. note::

   The FIFO object reserves the first WORD in each stored memory
   block as a link pointer to the next item. The size of the WORD
   depends on the platform and can be 16 bit, 32 bit, etc.

APIs
****

The following APIs for a nanokernel FIFO are provided by :file:`nanokernel.h.`

+------------------------------------------------+------------------------------------+
| Call                                           | Description                        |
+================================================+====================================+
| :c:func:`nano_fifo_init()`                     | Initializes a FIFO.                |
+------------------------------------------------+------------------------------------+
| | :c:func:`nano_task_fifo_put()`               | Adds item to a FIFO.               |
| | :c:func:`nano_fiber_fifo_put()`              |                                    |
| | :c:func:`nano_isr_fifo_put()`                |                                    |
| | :c:func:`nano_fifo_put()`                    |                                    |
+------------------------------------------------+------------------------------------+
| | :c:func:`nano_task_fifo_get()`               | Removes item from a FIFO, or fails |
| | :c:func:`nano_fiber_fifo_get()`              | and continues if it is empty.      |
| | :c:func:`nano_isr_fifo_get()`                |                                    |
| | :c:func:`nano_fifo_get()`                    |                                    |
+------------------------------------------------+------------------------------------+
| | :c:func:`nano_task_fifo_get_wait()`          | Removes item from a FIFO, or waits |
| | :c:func:`nano_fiber_fifo_get_wait()`         | for an item if it is empty.        |
| | :c:func:`nano_fifo_get_wait()`               |                                    |
+------------------------------------------------+------------------------------------+
| | :c:func:`nano_task_fifo_get_wait_timeout()`  | Removes item from a FIFO, or waits |
| | :c:func:`nano_fiber_fifo_get_wait_timeout()` | for an item for a specified time   |
| | :c:func:`nano_fifo_get_wait_timeout()`       | period if it is empty.             |
+------------------------------------------------+------------------------------------+
