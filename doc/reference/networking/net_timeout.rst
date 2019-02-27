.. _net_timeout_interface:

Network Timeout
###############

.. contents::
    :local:
    :depth: 2

Overview
********

The ``k_delayed_work`` API has a 1 ms accuracy for a timeout value,
so the maximum timeout can be about 24 days. Some network timeouts
are longer than this, so the net_timeout API provides a generic timeout
mechanism that tracks such wraparounds and restarts the timeout as needed.

API Reference
*************

.. doxygengroup:: net_timeout
   :project: Zephyr
