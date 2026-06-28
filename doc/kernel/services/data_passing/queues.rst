.. _queues:

Queues
######

.. design:: DESIGN-QUEUES Queues
   :fulfills: ZEP-SRS-20-1 ZEP-SRS-20-2 ZEP-SRS-20-3 ZEP-SRS-20-4 ZEP-SRS-20-5 ZEP-SRS-20-6 ZEP-SRS-20-7 ZEP-SRS-20-8 ZEP-SRS-20-9 ZEP-SRS-20-10 ZEP-SRS-20-11 ZEP-SRS-20-12 ZEP-SRS-20-13 ZEP-SRS-20-14


A Queue in Zephyr is a kernel object that implements a traditional queue, allowing
threads and ISRs to add and remove data items of any size. The queue is similar
to a FIFO and serves as the underlying implementation for both :ref:`k_fifo
<fifos_v2>` and :ref:`k_lifo <lifos_v2>`. For more information on usage see
:ref:`k_fifo <fifos_v2>`.

Cancelling a Wait
*****************

.. design:: DESIGN-QUEUE-CANCEL-WAIT Cancelling a Wait
   :fulfills: ZEP-SRS-20-15

A thread that is blocked waiting to retrieve an item from a queue can be released
without an item by calling :c:func:`k_queue_cancel_wait` from another thread or
an ISR. The first thread pending on the queue returns from its
:c:func:`k_queue_get` call with a ``NULL`` value, exactly as if its timeout had
expired. If the queue is instead being waited on through :c:func:`k_poll`, that
call returns ``-EINTR`` with the poll event in the cancelled state.

This mechanism is also available to the queue-based primitives through
:c:macro:`k_fifo_cancel_wait` and :c:macro:`k_lifo_cancel_wait`.

Configuration Options
*********************

Related configuration options:

* None

API Reference
*************

.. doxygengroup:: queue_apis
