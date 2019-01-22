.. _kernel_apis:

Kernel APIs
###########

This section contains APIs for the kernel's core services,
as described in the :ref:`kernel`.

.. important::
    Unless otherwise noted these APIs can be used by threads, but not by ISRs.

.. contents::
   :depth: 1
   :local:
   :backlinks: top

.. comment
   not documenting
   .. doxygengroup:: kernel_apis


Profiling
*********

.. doxygengroup:: profiling_apis
   :project: Zephyr


Kernel Version
**************
Kernel version handling and APIs related to kernel version being used.

.. doxygengroup:: version_apis
   :project: Zephyr
   :content-only:

Memory Domain
*************

A memory domain contains some number of memory partitions. Threads can
specify the range and attribute (access permission) for memory partitions
in a memory domain. Threads in the same memory domain have the
same access permissions to the memory partitions belong to the
memory domain.
(See :ref:`memory_domain`.)

.. doxygengroup:: mem_domain_apis
   :project: Zephyr
