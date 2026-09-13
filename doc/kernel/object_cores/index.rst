.. _object_cores_api:

Object Cores
############

Object cores are a kernel debugging tool that can be used to both identify and
perform operations on registered objects.

.. contents::
    :local:
    :depth: 2

Object Core Concepts
********************

Each instance of an object embeds an object core field named ``obj_core``. An
object core links the object to its object type, and each object type lets
debugging tools enumerate the objects of that type. Object types are linked
together via a singly linked list. Together, this allows debugging tools to
traverse all the objects in the system.

An object type enumerates its objects from two places:

* Its permanent objects, which are statically defined instances such as those
  created with :c:macro:`K_SEM_DEFINE`. They are walked in place from their
  iterable section and are never registered at run time.
* A bounded registry of the objects initialized at run time, for instance a
  semaphore in a driver's data structure initialized with :c:func:`k_sem_init`.
  The registry only references the objects; it never stores anything inside
  them.

Because of that, an object may be initialized in any storage, initialized
again in place, or discarded without any object core call: the registry
cannot be corrupted by an object that no longer exists. Some rules follow
from this model:

* An object located in the running thread's stack or in the interrupt stack is
  not registered: its life ends with the stack frame. The object type counts
  such objects in its ``skipped`` field. Other stacks are not recognized, such
  as the privileged stack a user thread's system calls run on or an exception
  stack specific to an architecture; an object there is registered and stays
  reported until its storage is reused.
* When the registry is full, further objects are not registered and the object
  type counts them in its ``dropped`` field. The registry size is set with
  :kconfig:option:`CONFIG_OBJ_CORE_MAX_DYNAMIC_OBJECTS`.
* An object that was discarded without being unregistered stays reported until
  its storage is reused: the walk recognizes the reused storage by the missing
  type tag and drops the entry. The kernel unregisters objects itself when it
  releases them: on thread abort, in :c:func:`k_object_free`,
  :c:func:`k_timer_cleanup`, :c:func:`k_msgq_cleanup` and
  :c:func:`k_stack_cleanup`, and, with
  :kconfig:option:`CONFIG_OBJ_CORE_EVICT_ON_FREE`, when memory is returned to a
  heap or a memory slab. Code that ends an object's life in another way may
  call :c:func:`k_obj_core_unlink` so that the object stops being reported at
  once.

Object cores have been integrated into the following kernel objects:

* :ref:`Condition Variables <condvar>`
* :ref:`Events <events>`
* :ref:`FIFOs <fifos_v2>` and :ref:`LIFOs <lifos_v2>`
* :ref:`Mailboxes <mailboxes_v2>`
* :ref:`Memory Slabs <memory_slabs_v2>`
* :ref:`Message Queues <message_queues_v2>`
* :ref:`Mutexes <mutexes_v2>`
* :ref:`Pipes <pipes_v2>`
* :ref:`Queues <queues>`
* :ref:`Semaphores <semaphores_v2>`
* :ref:`Threads <threads_v2>`
* :ref:`Timers <timers_v2>`
* :ref:`System Memory Blocks <sys_mem_blocks>`

Developers are free to integrate them if desired into other objects within
their projects.

Object Core Statistics Concepts
*******************************
A variety of kernel objects allow for the gathering and reporting of statistics.
Object cores provide a uniform means to retrieve that information via object
core statistics. When enabled, the object type contains a pointer to a
statistics descriptor that defines the various operations that have been
enabled for interfacing with the object's statistics. Additionally, the object
core contains a pointer to the "raw" statistical information associated with
that object. Raw data is the raw, unmanipulated data associated with the
statistics. Queried data may be "raw", but it may also have been manipulated in
some way by calculation (such as determining an average).

The following table indicates both what objects have been integrated into the
object core statistics as well as the structures used for both "raw" and
"queried" data.

=====================  ============================== ==============================
Object                 Raw Data Type                  Query Data Type
=====================  ============================== ==============================
struct mem_slab        struct mem_slab_info            struct sys_memory_stats
struct sys_mem_blocks  struct sys_mem_blocks_info      struct sys_memory_stats
struct k_thread        struct k_cycle_stats            struct k_thread_runtime_stats
struct _cpu            struct k_cycle_stats            struct k_thread_runtime_stats
struct z_kernel        struct k_cycle_stats[num CPUs]  struct k_thread_runtime_stats
=====================  ============================== ==============================

Implementation
**************

Defining a New Object Type
==========================

An object type is a global variable of type :c:struct:`k_obj_type`. When the
object struct has statically defined instances in an iterable section, the
type is defined at build time with :c:macro:`K_OBJ_TYPE_DEFINE` and those
instances become its permanent objects. The following code shows how a new
object type can be defined for use with object cores and object core
statistics.

.. code-block:: c

    /* Unique object type ID */

    #define K_OBJ_TYPE_MY_NEW_TYPE  K_OBJ_TYPE_ID_GEN("UNIQ")

    struct my_obj_type_raw_info {
        ...
    };

    struct my_obj_type_query_stats {
        ...
    };

    struct my_new_obj {
        ...
        struct k_obj_core obj_core;
        struct my_obj_type_raw_info  info;
    };

    struct k_obj_core_stats_desc my_obj_type_stats_desc = {
        .raw_size = sizeof(struct my_obj_type_raw_stats),
        .query_size = sizeof(struct my_obj_type_query_stats),
        .raw = my_obj_type_stats_raw,
        .query = my_obj_type_stats_query,
        .reset = my_obj_type_stats_reset,
        .disable = NULL,    /* Stats gathering is always on */
        .enable = NULL,     /* Stats gathering is always on */
    };

    K_OBJ_TYPE_DEFINE_STATS(my_obj_type, my_new_obj, K_OBJ_TYPE_MY_NEW_TYPE,
                            &my_obj_type_stats_desc, info);

A type whose objects are not in an iterable section is initialized at run
time instead, before any of its objects. A permanent array of objects can be
registered as the type's range so that its objects need no registry entry.

.. code-block:: c

    struct k_obj_type  my_obj_type;
    struct my_new_obj  my_objects[8];

    void my_obj_type_init(void)
    {
        z_obj_type_init(&my_obj_type, K_OBJ_TYPE_MY_NEW_TYPE,
                        offsetof(struct my_new_obj, obj_core));
        z_obj_type_init_range(&my_obj_type, my_objects,
                              &my_objects[ARRAY_SIZE(my_objects)],
                              sizeof(struct my_new_obj), false);
        k_obj_type_stats_init(&my_obj_type, &my_obj_type_stats_desc);
    }

Initializing a New Object Core
==============================

Kernel objects that have already been integrated into the object core framework
automatically have their object cores initialized when the object is
initialized. However, developers that wish to add their own objects into the
framework need to both initialize the object core and register it. Registering
an object that belongs to the type's permanent range has no effect, as it is
reported from the range. The following code builds on the example above and
initializes the object core.

.. code-block:: c

    void my_new_obj_init(struct my_new_obj *new_obj)
    {
        ...
        k_obj_core_init(K_OBJ_CORE(new_obj), &my_obj_type);
        k_obj_core_link(K_OBJ_CORE(new_obj));
        k_obj_core_stats_register(K_OBJ_CORE(new_obj), &new_obj->raw_stats,
                                  sizeof(struct my_obj_type_raw_info));
    }

Walking a List of Object Cores
==============================

Two routines exist for walking the object cores of an object type. These are
:c:func:`k_obj_type_walk_locked` and :c:func:`k_obj_type_walk_unlocked`. Both
visit the permanent objects of the type first and the registered objects next.
The following code builds upon the example above and prints the addresses of
all the objects of that new object type.

.. code-block:: c

    int walk_op(struct k_obj_core *obj_core, void *data)
    {
        uint8_t *ptr;

        ptr = obj_core;
        ptr -= obj_core->type->obj_core_offset;

        printk("%p\n", ptr);

        return 0;
    }

    void print_object_addresses(void)
    {
        struct k_obj_type *obj_type;

        /* Find the object type */

        obj_type = k_obj_type_find(K_OBJ_TYPE_MY_NEW_TYPE);

        /* Walk the list of objects */

        k_obj_type_walk_unlocked(obj_type, walk_op, NULL);
    }

Object Core Statistics Querying
===============================

The following code builds on the examples above and shows how an object
integrated into the object core statistics framework can both retrieve queried
data and reset the stats associated with the object.

.. code-block:: c

    struct my_new_obj my_obj;

    ...

    void my_func(void)
    {
        struct my_obj_type_query_stats  my_stats;
        int  status;

        my_obj_type_init(&my_obj);

        ...

        status = k_obj_core_stats_query(K_OBJ_CORE(&my_obj),
                                        &my_stats, sizeof(my_stats));
        if (status != 0) {
            /* Failed to get stats */
            ...
        } else {
            k_obj_core_stats_reset(K_OBJ_CORE(&my_obj));
        }

        ...
    }

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_OBJ_CORE`
* :kconfig:option:`CONFIG_OBJ_CORE_MAX_DYNAMIC_OBJECTS`
* :kconfig:option:`CONFIG_OBJ_CORE_EVICT_ON_FREE`
* :kconfig:option:`CONFIG_OBJ_CORE_CONDVAR`
* :kconfig:option:`CONFIG_OBJ_CORE_EVENT`
* :kconfig:option:`CONFIG_OBJ_CORE_FIFO`
* :kconfig:option:`CONFIG_OBJ_CORE_LIFO`
* :kconfig:option:`CONFIG_OBJ_CORE_MAILBOX`
* :kconfig:option:`CONFIG_OBJ_CORE_MEM_SLAB`
* :kconfig:option:`CONFIG_OBJ_CORE_MSGQ`
* :kconfig:option:`CONFIG_OBJ_CORE_MUTEX`
* :kconfig:option:`CONFIG_OBJ_CORE_PIPE`
* :kconfig:option:`CONFIG_OBJ_CORE_QUEUE`
* :kconfig:option:`CONFIG_OBJ_CORE_SEM`
* :kconfig:option:`CONFIG_OBJ_CORE_STACK`
* :kconfig:option:`CONFIG_OBJ_CORE_THREAD`
* :kconfig:option:`CONFIG_OBJ_CORE_TIMER`
* :kconfig:option:`CONFIG_OBJ_CORE_SYS_MEM_BLOCKS`
* :kconfig:option:`CONFIG_OBJ_CORE_STATS`
* :kconfig:option:`CONFIG_OBJ_CORE_STATS_MEM_SLAB`
* :kconfig:option:`CONFIG_OBJ_CORE_STATS_THREAD`
* :kconfig:option:`CONFIG_OBJ_CORE_STATS_SYSTEM`
* :kconfig:option:`CONFIG_OBJ_CORE_STATS_SYS_MEM_BLOCKS`

API Reference
*************

.. doxygengroup:: obj_core_apis
.. doxygengroup:: obj_core_stats_apis
