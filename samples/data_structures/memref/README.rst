.. zephyr:code-sample:: memref
   :name: Memref Shared Memory References
   :relevant-api: memref

   Share reference counted allocations between a producer and multiple consumers.

Overview
********

This sample demonstrates the :ref:`memref` API on top of a ``k_heap`` backend.

The producer allocates messages with :c:func:`memref_alloc` and broadcasts them to
multiple consumers. Each consumer makes a choice to keep the message or drop it. If a
consumer keeps the message, it claims ownership of the reference with :c:func:`memref_ref` and
then releases its own reference with :c:func:`memref_unref` once it has processed the message.

The sample aims to demonstrate the how to easily manage complex memory ownership through the memref
API. The producer does not need to know how many consumers there are, and the consumers do not need
to know how many other consumers there are. The memref API handles all of the reference counting and
memory management for you.

Building and Running
********************

To build and run this sample on a supported board:

.. code-block:: console

   west build -b <your_board> samples/data_structures/memref
   west flash

Replace ``<your_board>`` with your actual board name (e.g., ``native_sim``).
