.. _nanokernel_lifos:

Nanokernel LIFOs
################

Definition
**********

The LIFO is defined in :file:`kernel/nanokernel/nano_lifo.c`. It
consists of a linked list of memory blocks that uses the first word in
each block as a next pointer. The data is stored in last-in-first-out
order.

Function
********

When a message is added to the LIFO, the data is stored at the head of
the list. Messages taken off the LIFO object are taken from the head.

The LIFO object requires the first 32-bit word to be empty in order to
maintain the linked list.

The LIFO object does not store information about the size of the
messages.

The LIFO object remembers one waiting context. When a second context
starts waiting for data from the same LIFO object, the first context
remains waiting and never reaches the runnable state.

APIs
****

The following APIs for a nanokernel LIFO are provided by :file:`nanokernel.h.`

+------------------------------------------------+------------------------------------+
| Call                                           | Description                        |
+================================================+====================================+
| :c:func:`nano_lifo_init()`                     | Initializes a LIFO.                |
+------------------------------------------------+------------------------------------+
| | :c:func:`nano_task_lifo_put()`               | Adds item to a LIFO.               |
| | :c:func:`nano_fiber_lifo_put()`              |                                    |
| | :c:func:`nano_isr_lifo_put()`                |                                    |
+------------------------------------------------+------------------------------------+
| | :c:func:`nano_task_lifo_get()`               | Removes item from a LIFO, or fails |
| | :c:func:`nano_fiber_lifo_get()`              | and continues if it is empty.      |
| | :c:func:`nano_isr_lifo_get()`                |                                    |
+------------------------------------------------+------------------------------------+
| | :c:func:`nano_task_lifo_get_wait()`          | Removes item from a LIFO, or waits |
| | :c:func:`nano_fiber_lifo_get_wait()`         | for an item if it is empty.        |
+------------------------------------------------+------------------------------------+
| | :c:func:`nano_task_lifo_get_wait_timeout()`  | Removes item from a LIFO, or waits |
| | :c:func:`nano_fiber_lifo_get_wait_timeout()` | for an item for a specified time   |
| |                                              | period if it is empty.             |
+------------------------------------------------+------------------------------------+
