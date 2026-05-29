.. _queues:

Queues
######

A Queue in Zephyr is a kernel object that implements a traditional queue, allowing
threads and ISRs to add and remove data items of any size. The queue is similar
to a FIFO and serves as the underlying implementation for both :ref:`k_fifo
<fifos_v2>` and :ref:`k_lifo <lifos_v2>`. For more information on usage see
:ref:`k_fifo <fifos_v2>`.


Configuration Options
*********************

Related configuration options:

* None

API Reference
*************

.. doxygengroup:: queue_apis
