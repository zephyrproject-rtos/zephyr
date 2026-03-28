.. _hwspinlock_api:

Hardware Spinlocks (HWSPINLOCK)
###############################

Overview
********

An HWSPINLOCK device is a peripheral used to protect shared resources across clusters in the system.
Each HWSPINLOCK instance is providing one or more spinlocks. The api is similar to regular zephyr spinlocks.

.. doxygengroup:: spinlock_apis

Because we also want to protect the spinlock resource to be used by multiple cores in the same
cluster, each HWSPINLOCK device include a regular zephyr spinlock, and use it to lock the access to
HWSPINLOCK hardware.

API Reference
*************

.. doxygengroup:: hwspinlock_interface
