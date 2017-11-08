.. _memory_domain:

Memory Domain
#############

The memory domain APIs are used by unprivileged threads to share data to
the threads in the same memory domain and protect sensitive data from threads
outside their domain. Memory domains are not only used for improving security,
but are also useful for debugging (unexpected access would cause an exception).

An alternative to using memory domains is the
:option:`CONFIG_APPLICATION_MEMORY` option, which will grant access to user
threads at boot to all global memory defined in object files that are not
part of the core kernel. This is useful for very simple applications which
will allow all threads to use global data defined within the application, but
each thread's stack is still protected from other user threads and there is
no access to private kernel data structures.

Since architectures generally have constraints on how many partitions can be
defined, and the size/alignment of each partition, users may need to group
related data together using linker sections.

.. contents::
    :local:
    :depth: 2

Concepts
********

A memory domain contains some number of memory partitions.
A memory partition is a memory region (might be RAM, peripheral registers,
or flash, for example) with specific attributes (access permission, e.g.
privileged read/write, unprivileged read-only, or execute never).
Memory partitions are defined by a set of underlying MPU regions
or MMU tables. A thread belongs to a single memory domain at
any point in time but a memory domain may contain multiple threads.
Threads in the same memory domain have the same access permissions
to the memory partitions belonging to the memory domain. New threads
will inherit any memory domain configuration from the parent thread.

Implementation
**************

Create a Memory Domain
======================

A memory domain is defined using a variable of type
:c:type:`struct k_mem_domain`. It must then be initialized by calling
:cpp:func:`k_mem_domain_init()`.

The following code defines and initializes an empty memory domain.

.. code-block:: c

    struct k_mem_domain app0_domain;

    k_mem_domain_init(&app0_domain, "app0_domain", 0, NULL);

Add Memory Partitions into a Memory Domain
==========================================

There are two ways to add memory partitions into a memory domain.

This first code sample shows how to add memory partitions while creating
a memory domain.

.. code-block:: c

    /* the start address of the MPU region needs to align with its size */
    u8_t __aligned(32) app0_buf[32];
    u8_t __aligned(32) app1_buf[32];

    K_MEM_PARTITION_DEFINE(app0_part0, app0_buf, sizeof(app0_buf),
                           K_MEM_PARTITION_P_RW_U_RW);

    K_MEM_PARTITION_DEFINE(app0_part1, app1_buf, sizeof(app1_buf),
                           K_MEM_PARTITION_P_RW_U_RO);

    struct k_mem_partition *app0_parts[] = {
        app0_part0,
        app0_part1
    };

    k_mem_domain_init(&app0_domain, ARRAY_SIZE(app0_parts), app0_parts);

This second code sample shows how to add memory partitions into an initialized
memory domain one by one.

.. code-block:: c

    /* the start address of the MPU region needs to align with its size */
    u8_t __aligned(32) app0_buf[32];
    u8_t __aligned(32) app1_buf[32];

    K_MEM_PARTITION_DEFINE(app0_part0, app0_buf, sizeof(app0_buf),
                           K_MEM_PARTITION_P_RW_U_RW);

    K_MEM_PARTITION_DEFINE(app0_part1, app1_buf, sizeof(app1_buf),
                           K_MEM_PARTITION_P_RW_U_RO);

    k_mem_domain_add_partition(&app0_domain, &app0_part0);
    k_mem_domain_add_partition(&app0_domain, &app0_part1);

.. note::
    The maximum number of memory partitions is limited by the maximum
    number of MPU regions or the maximum number of MMU tables.

Add Threads into a Memory Domain
================================

Adding threads into a memory domain grants threads permission to access
the memory partitions in the memory domain.

The following code shows how to add threads into a memory domain.

.. code-block:: c

    k_mem_domain_add_thread(&app0_domain, &app_thread_id);

Remove a Memory Partition from a Memory Domain
==============================================

The following code shows how to remove a memory partition from a memory
domain.

.. code-block:: c

    k_mem_domain_remove_partition(&app0_domain, &app0_part1);

The k_mem_domain_remove_partition() API finds the memory partition
that matches the given parameter and removes that partition from the
memory domain.

Remove a Thread from the Memory Domain
======================================

The following code shows how to remove a thread from the memory domain.

.. code-block:: c

    k_mem_domain_remove_thread(&app0_domain, &app_thread_id);

Destroy a Memory Domain
=======================

The following code shows how to destroy a memory domain.

.. code-block:: c

    k_mem_domain_destroy(&app0_domain);

Available Partition Attributes
==============================

When defining a partition, we need to set access permission attributes
to the partition. Since the access control of memory partitions relies on
either an MPU or MMU, the available partition attributes would be architecture
dependent.

The complete list of available partition attributes for a specific architecture
is found in the architecture-specific include file
``include/arch/<arch name>/arch.h``, (for example, ``include/arch/arm/arch.h``.)
Some examples of partition attributes are:

.. code-block:: c

    /* Denote partition is privileged read/write, unprivileged read/write */
    K_MEM_PARTITION_P_RW_U_RW
    /* Denote partition is privileged read/write, unprivileged read-only */
    K_MEM_PARTITION_P_RW_U_RO

Configuration Options
*********************

Related configuration options:

* :option:`CONFIG_MAX_DOMAIN_PARTITIONS`

APIs
****

The following memory domain APIs are provided by :file:`kernel.h`:

* :c:macro:`K_MEM_PARTITION_DEFINE`
* :cpp:func:`k_mem_domain_init()`
* :cpp:func:`k_mem_domain_destroy()`
* :cpp:func:`k_mem_domain_add_partition()`
* :cpp:func:`k_mem_domain_remove_partition()`
* :cpp:func:`k_mem_domain_add_thread()`
* :cpp:func:`k_mem_domain_remove_thread()`
