.. _osal:

OS Abstraction Layer (OSAL)
###########################

.. contents::
    :local:
    :depth: 2

Overview
========

External modules and binary blobs are often built against a foreign RTOS API
or an OS abstraction shim. When ported to Zephyr, each one re-implements the
same translations onto Zephyr kernel objects: allocate a control block, init
it, convert a millisecond timeout to a :c:type:`k_timeout_t`, bridge a
single-argument thread entry to Zephyr's three-argument signature, and so on.

The OS Abstraction Layer centralizes those translations behind a small
``osal_*`` API so an adapter shrinks to a thin name and return-code mapping.
It is enabled with :kconfig:option:`CONFIG_OSAL`.

.. note::

   OSAL is an integration aid for code that cannot call the Zephyr kernel API
   directly (for example, a vendor blob built against a different RTOS).
   Native Zephyr applications and drivers should call the kernel API
   (``k_*``) directly rather than going through this layer.

Scope
=====

The layer provides:

* Scheduler lock (pause preemption, nestable)
* Spinlock / critical section (SMP-safe, :c:struct:`k_spinlock` backed)
* Interrupt lock (local-CPU, key based, pre-kernel safe)
* Recursive critical section (SMP-safe, owner-CPU + nesting count)
* Mutex (recursive by default, like :c:struct:`k_mutex`)
* Counting/binary semaphore
* Fixed-item message queue (send, send-to-front, send-from-ISR, peek)
* Intrusive FIFO (pointer-passing, prepend, arbitrary remove)
* Event group (bit-level wait/notify, backed by :c:struct:`k_event`)
* Thread/task with heap-allocated stack, priority get/set, suspend/resume
* Heap (malloc/calloc/free)
* Memory pool (fixed-size blocks, backed by :c:struct:`k_mem_slab`)
* Time (delay, uptime, busy-wait, tick count/frequency, ISR check)
* Software timer (one-shot or periodic)

Timeouts are expressed in milliseconds. ``OSAL_NO_WAIT`` and ``OSAL_FOREVER``
map to :c:macro:`K_NO_WAIT` and :c:macro:`K_FOREVER`.

Configuration
=============

OSAL is not enabled directly by applications. A consumer's Kconfig selects
:kconfig:option:`CONFIG_OSAL`:

.. code-block:: kconfig

   config MY_MODULE_USE_OSAL
           bool
           default y if MY_MODULE
           select OSAL

:kconfig:option:`CONFIG_OSAL_DYNAMIC_ALLOC` (default ``y``) controls the
allocation policy:

* **Dynamic** (default): control blocks come from the system heap
  (``k_malloc``) and thread stacks from ``k_thread_stack_alloc``.
* **Static** (``CONFIG_OSAL_DYNAMIC_ALLOC=n``): control blocks come from a
  bounded, dedicated heap and threads from a fixed stack pool, so OSAL
  objects can be created without a system heap. The pools are sized by
  :kconfig:option:`CONFIG_OSAL_MAX_OBJECTS`,
  :kconfig:option:`CONFIG_OSAL_MAX_THREADS` and
  :kconfig:option:`CONFIG_OSAL_THREAD_STACK_SIZE`.

Writing an adapter
==================

An adapter maps a foreign RTOS API onto ``osal_*``. Most functions collapse
to a single line.

Foreign API (before), each call re-implementing the same Zephyr translation:

.. code-block:: c

   static void *my_mutex_create(void)
   {
           struct k_mutex *m = malloc(sizeof(struct k_mutex));

           if (m == NULL) {
                   return NULL;
           }
           k_mutex_init(m);
           return m;
   }

   static int my_mutex_lock(void *mutex)
   {
           return k_mutex_lock(mutex, K_FOREVER) == 0 ? 0 : -1;
   }

With the abstraction layer (after):

.. code-block:: c

   static void *my_mutex_create(void)
   {
           return osal_mutex_create();
   }

   static int my_mutex_lock(void *mutex)
   {
           return osal_mutex_lock(mutex, OSAL_FOREVER) == OSAL_OK ? 0 : -1;
   }

Limitations
===========

* The interrupt lock (``osal_irq_lock``) only masks interrupts on the local
  CPU and is not SMP-safe on its own; use the spinlock for data shared across
  cores.
* The message queue uses copy semantics (fixed-size items). For
  pointer-passing with prepend and arbitrary removal, use the intrusive FIFO.
* In static-allocation mode the memory pool's backing buffer is still
  obtained from the system heap, because its size is a runtime parameter;
  only the control blocks and thread stacks come from static pools.

API Reference
=============

.. doxygengroup:: osal
