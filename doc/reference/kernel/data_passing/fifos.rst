.. _fifos_v2:

FIFOs
#####

A :dfn:`FIFO` is a kernel object that implements a traditional
first in, first out (FIFO) queue, allowing threads and ISRs
to add and remove data items of any size.

.. contents::
    :local:
    :depth: 2

Concepts
********

Any number of FIFOs can be defined (limited only by available RAM). Each FIFO is
referenced by its memory address.

A FIFO has the following key properties:

* A **queue** of data items that have been added but not yet removed.
  The queue is implemented as a simple linked list.

A FIFO must be initialized before it can be used. This sets its queue to empty.

FIFO data items must be aligned on a word boundary, as the kernel reserves
the first word of an item for use as a pointer to the next data item in
the queue. Consequently, a data item that holds N bytes of application
data requires N+4 (or N+8) bytes of memory. There are no alignment or
reserved space requirements for data items if they are added with
:c:func:`k_fifo_alloc_put`, instead additional memory is temporarily
allocated from the calling thread's resource pool.

A data item may be **added** to a FIFO by a thread or an ISR.
The item is given directly to a waiting thread, if one exists;
otherwise the item is added to the FIFO's queue.
There is no limit to the number of items that may be queued.

A data item may be **removed** from a FIFO by a thread. If the FIFO's queue
is empty a thread may choose to wait for a data item to be given.
Any number of threads may wait on an empty FIFO simultaneously.
When a data item is added, it is given to the highest priority thread
that has waited longest.

.. note::
    The kernel does allow an ISR to remove an item from a FIFO, however
    the ISR must not attempt to wait if the FIFO is empty.

If desired, **multiple data items** can be added to a FIFO in a single operation
if they are chained together into a singly-linked list. This capability can be
useful if multiple writers are adding sets of related data items to the FIFO,
as it ensures the data items in each set are not interleaved with other data
items. Adding multiple data items to a FIFO is also more efficient than adding
them one at a time, and can be used to guarantee that anyone who removes
the first data item in a set will be able to remove the remaining data items
without waiting.

Implementation
**************

Defining a FIFO
===============

A FIFO is defined using a variable of type :c:struct:`k_fifo`.
It must then be initialized by calling :c:func:`k_fifo_init`.

The following code defines and initializes an empty FIFO.

.. code-block:: c

    struct k_fifo my_fifo;

    k_fifo_init(&my_fifo);

Alternatively, an empty FIFO can be defined and initialized at compile time
by calling :c:macro:`K_FIFO_DEFINE`.

The following code has the same effect as the code segment above.

.. code-block:: c

    K_FIFO_DEFINE(my_fifo);

Writing to a FIFO
=================

A data item is added to a FIFO by calling :c:func:`k_fifo_put`.

The following code builds on the example above, and uses the FIFO
to send data to one or more consumer threads.

.. code-block:: c

    struct data_item_t {
        void *fifo_reserved;   /* 1st word reserved for use by FIFO */
        ...
    };

    struct data_item_t tx_data;

    void producer_thread(int unused1, int unused2, int unused3)
    {
        while (1) {
            /* create data item to send */
            tx_data = ...

            /* send data to consumers */
            k_fifo_put(&my_fifo, &tx_data);

            ...
        }
    }

Additionally, a singly-linked list of data items can be added to a FIFO
by calling :c:func:`k_fifo_put_list` or :c:func:`k_fifo_put_slist`.

Finally, a data item can be added to a FIFO with :c:func:`k_fifo_alloc_put`.
With this API, there is no need to reserve space for the kernel's use in
the data item, instead additional memory will be allocated from the calling
thread's resource pool until the item is read.

Reading from a FIFO
===================

A data item is removed from a FIFO by calling :c:func:`k_fifo_get`.

The following code builds on the example above, and uses the FIFO
to obtain data items from a producer thread,
which are then processed in some manner.

.. code-block:: c

    void consumer_thread(int unused1, int unused2, int unused3)
    {
        struct data_item_t  *rx_data;

        while (1) {
            rx_data = k_fifo_get(&my_fifo, K_FOREVER);

            /* process FIFO data item */
            ...
        }
    }

Suggested Uses
**************

Use a FIFO to asynchronously transfer data items of arbitrary size
in a "first in, first out" manner.

Configuration Options
*********************

Related configuration options:

* None

API Reference
*************

.. doxygengroup:: fifo_apis
   :project: Zephyr
