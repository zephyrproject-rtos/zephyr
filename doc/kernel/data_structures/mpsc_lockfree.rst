.. _mpsc_lockfree:

Multi Producer Single Consumer Lock Free Queue
==============================================

A :dfn:`Multi Producer Single Consumer Lock Free Queue (MPSC)` is an lockfree
intrusive queue based on atomic pointer swaps as described by Dmitry Vyukov
at `1024cores <https://www.1024cores.net/home/lock-free-algorithms/queues/intrusive-mpsc-node-based-queue>`_.


API Reference
*************

.. doxygengroup:: mpsc_lockfree
