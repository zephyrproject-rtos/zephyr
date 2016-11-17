.. _event_logger:

Event Logging APIs
##################

.. contents::
   :depth: 1
   :local:
   :backlinks: top

Event Logger
************

An event logger is an object that can record the occurrence of significant
events, which can be subsequently extracted and reviewed.

.. doxygengroup:: event_logger
   :project: Zephyr
   :content-only:

Kernel Event Logger
*******************

The kernel event logger records the occurrence of significant kernel events,
which can be subsequently extracted and reviewed.
(See :ref:`kernel_event_logger_v2`.)

.. doxygengroup:: kernel_event_logger
   :project: Zephyr
   :content-only:
