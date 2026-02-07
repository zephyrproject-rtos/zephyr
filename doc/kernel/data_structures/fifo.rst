.. _fixed_size_fifo_api:

Fifo Data Structure
####################

.. contents::
  :local:
  :depth: 2

Overview
********

A FIFO (First In, First Out) data structure is a collection that maintains the order of elements
such that the first element added to the collection will be the first one to be removed. This
behavior is similar to a queue in real life, where the first person in line is the first to be
served.

The FIFO data structure is particularly useful in scenarios where order of processing is important,
such as task scheduling, buffering data streams, and managing resources in a fair manner.

The fixed-size FIFO is used whenever you need to decouple producers and consumers of discrete
sizes without worrying about partial reads or variable-length payloads. This
differentiation separates it from the ring_buffer data structure, which is a streaming data
structure of bytes.

Concurrency
===========

The fixed size fifo api do not provide any concurrency control. Depending on usage, applications
should protect the fifo structure with appropriate synchronization mechanisms (e.g., mutexes,
semaphores) to ensure thread safety when accessed from multiple threads.

Instantiation & Usage
*********************

A fixed-size FIFO can be declared either using the ``FIFO_DEFINE(name, item_size, item_capacity)``
macro or in runtime using the
``fifo_init(struct fifo *queue, void *data, size_t item_size, size_t item_capacity);`` function.

.. code-block:: c

    FIFO_INIT(&my_fifo, item_size, item_capacity, buffer_pointer);

    /* equivalent to */

   static struct fifo my_fifo;
   static uint8_t buffer[item_size * item_capacity];
   void init_fn (void) {
      fifo_init(&my_fifo, buffer, item_size, item_capacity);
      ....
   }

Once the FIFO is initialized, items can be added to the FIFO using the
``fifo_put()`` function and removed using the ``fifo_get()`` function. The FIFO maintains the order
of the items and ensures proper boundary checking of the underlying data buffer.

.. code-block:: c

    struct my_item item_to_add = { ... };
    struct my_item item_removed;

    /* Add an item to the FIFO */
    if (fifo_put(&my_fifo, &item_to_add) == 0) {
        // Item added successfully
    } else {
        // FIFO is full
    }

    /* Remove an item from the FIFO */
    if (fifo_get(&my_fifo, &item_removed) == 0) {
        // Item removed successfully
    } else {
        // FIFO is empty
    }

In addition to the standard data operations (fifo_put() and fifo_get()), the FIFO API provides a set
of utility functions for managing and inspecting the FIFO state.

* fifo_capacity() – Returns the total capacity of the FIFO.
* fifo_empty() – Returns true if the FIFO contains no items.
* fifo_full() – Returns true if the FIFO cannot accept more items.
* fifo_space() – Returns the number of free slots remaining.
* fifo_size() – Returns the number of items currently stored.
* fifo_reset() – Resets the FIFO to an empty state, without affecting the underlying buffer.

API Reference
*************
.. doxygengroup:: fixed_size_fifo_apis
