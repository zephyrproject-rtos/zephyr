.. _lifos_v2:

LIFOs
#####

A :dfn:`lifo` is a kernel object that implements a traditional
last in, first out (LIFO) queue, allowing threads and ISRs
to add and remove data items of any size.

.. contents::
    :local:
    :depth: 2

Concepts
********

Any number of lifos can be defined. Each lifo is referenced
by its memory address.

A lifo has the following key properties:

* A **queue** of data items that have been added but not yet removed.
  The queue is implemented as a simple linked list.

A lifo must be initialized before it can be used. This sets its queue to empty.

LIFO data items must be aligned on a 4-byte boundary, as the kernel reserves
the first 32 bits of an item for use as a pointer to the next data item in the
queue. Consequently, a data item that holds N bytes of application data
requires N+4 bytes of memory. There are no alignment or reserved space
requirements for data items if they are added with
:cpp:func:`k_lifo_alloc_put()`, instead additional memory is temporarily
allocated from the calling thread's resource pool.

A data item may be **added** to a lifo by a thread or an ISR.
The item is given directly to a waiting thread, if one exists;
otherwise the item is added to the lifo's queue.
There is no limit to the number of items that may be queued.

A data item may be **removed** from a lifo by a thread. If the lifo's queue
is empty a thread may choose to wait for a data item to be given.
Any number of threads may wait on an empty lifo simultaneously.
When a data item is added, it is given to the highest priority thread
that has waited longest.

.. note::
    The kernel does allow an ISR to remove an item from a lifo, however
    the ISR must not attempt to wait if the lifo is empty.

Implementation
**************

Defining a LIFO
===============

A lifo is defined using a variable of type :c:type:`struct k_lifo`.
It must then be initialized by calling :cpp:func:`k_lifo_init()`.

The following defines and initializes an empty lifo.

.. code-block:: c

    struct k_lifo my_lifo;

    k_lifo_init(&my_lifo);

Alternatively, an empty lifo can be defined and initialized at compile time
by calling :c:macro:`K_LIFO_DEFINE`.

The following code has the same effect as the code segment above.

.. code-block:: c

    K_LIFO_DEFINE(my_lifo);

Writing to a LIFO
=================

A data item is added to a lifo by calling :cpp:func:`k_lifo_put()`.

The following code builds on the example above, and uses the lifo
to send data to one or more consumer threads.

.. code-block:: c

    struct data_item_t {
        void *lifo_reserved;   /* 1st word reserved for use by lifo */
        ...
    };

    struct data_item_t tx data;

    void producer_thread(int unused1, int unused2, int unused3)
    {
        while (1) {
            /* create data item to send */
            tx_data = ...

            /* send data to consumers */
            k_lifo_put(&my_lifo, &tx_data);

            ...
        }
    }

A data item can be added to a lifo with :cpp:func:`k_lifo_alloc_put()`.
With this API, there is no need to reserve space for the kernel's use in
the data item, instead additional memory will be allocated from the calling
thread's resource pool until the item is read.

Reading from a LIFO
===================

A data item is removed from a lifo by calling :cpp:func:`k_lifo_get()`.

The following code builds on the example above, and uses the lifo
to obtain data items from a producer thread,
which are then processed in some manner.

.. code-block:: c

    void consumer_thread(int unused1, int unused2, int unused3)
    {
        struct data_item_t  *rx_data;

        while (1) {
            rx_data = k_lifo_get(&my_lifo, K_FOREVER);

            /* process lifo data item */
            ...
        }
    }

Suggested Uses
**************

Use a lifo to asynchronously transfer data items of arbitrary size
in a "last in, first out" manner.

Configuration Options
*********************

Related configuration options:

* None.

APIs
****

The following lifo APIs are provided by :file:`kernel.h`:

* :c:macro:`K_LIFO_DEFINE`
* :cpp:func:`k_lifo_init()`
* :cpp:func:`k_lifo_put()`
* :cpp:func:`k_lifo_get()`
