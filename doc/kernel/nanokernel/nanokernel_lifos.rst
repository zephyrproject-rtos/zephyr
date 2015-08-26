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

Usage
*****

Example: Initializing a Nanokernel LIFO
=======================================

This code initializes a nanokernel LIFO, marking it as empty without any
waiters.

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
