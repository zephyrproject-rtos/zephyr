.. _spinlocks:

Spinlocks
#########

.. contents::
  :local:
  :depth: 2

A spinlock is the lowest-level mutual-exclusion primitive in Zephyr.
It guards a brief critical section so that only one execution context can access a shared resource
at a time. A context that finds the lock already held "spins", busy waiting until the lock becomes
available, rather than blocking and yielding the CPU. This is why spinlocks are only appropriate for
short critical sections.

Acquiring a spinlock also masks interrupts on the local CPU for as long as the lock is held. This is
what makes a spinlock safe to share between threads and interrupt handlers: while a context holds
the lock, no ISR on the same CPU can preempt it and no other CPU can enter the critical section, so
neither can observe or corrupt the protected data mid-update.
For these reasons, :c:struct:`k_spinlock` is the primary synchronization primitive used within the
Zephyr kernel itself, protecting access to its core data structures.

Application code may use it directly as well but it is generally recommended to use a higher-level
synchronization primitive such as a ``k_mutex`` or ``k_sem`` instead.

Usage
*****

Declare a :c:struct:`k_spinlock` for each independent resource you need to protect. Acquire it with
:c:func:`k_spin_lock`, which returns a :c:type:`k_spinlock_key_t` that must be passed back to
:c:func:`k_spin_unlock` to release the lock:

.. code-block:: c

   static struct k_spinlock lock;

   void update_shared_state(void)
   {
           k_spinlock_key_t key = k_spin_lock(&lock);

           /* critical section: exclusive access */

           k_spin_unlock(&lock, key);
   }

The :c:macro:`K_SPINLOCK` helper acquires the lock for the duration of the
enclosed block and releases it automatically on exiting the block.

.. code-block:: c

   static struct k_spinlock lock;

   void update_shared_state(void)
   {
           K_SPINLOCK(&lock) {
               /* critical section: exclusive access */
           }
   }

Follow these rules when using a spinlock:

* Keep the critical section as short as possible. Interrupts are masked on the
  local CPU while the lock is held, and other CPUs may be spinning on it.
* Never perform a blocking or sleeping operations while holding a spinlock.
* Do not acquire a spinlock recursively. A context that already holds a lock
  must not try to take it again, or it will deadlock. Nesting **distinct**
  spinlocks is allowed, but must follow a consistent lock ordering to avoid
  deadlock.

.. _spinlocks_smp:

Behavior on SMP systems
***********************

On multiprocessor systems the spinlock does more than mask local interrupts. It
also atomically validates that a shared lock variable has been modified before
returning to the caller, "spinning" on the check if needed to wait for another
CPU to exit the lock. The default Zephyr implementation of :c:func:`k_spin_lock`
and :c:func:`k_spin_unlock` is built on top of the pre-existing
:c:struct:`atomic_` layer (itself usually implemented using compiler
intrinsics), though facilities exist for architectures to define their own for
performance reasons.

This is the key difference from the legacy :c:func:`irq_lock` API. That API was
naturally recursive: the lock was global, so it was legal to acquire a nested
lock inside of a critical section. Spinlocks are separable: you can have many
locks for separate subsystems or data structures, preventing CPUs from
contending on a single global resource. But that means that spinlocks must not
be used recursively. A validation layer is available to detect and report bugs
like this.

When used on a uniprocessor system, the data component of the spinlock (the
atomic lock variable) is unnecessary and elided. Except for the recursive
semantics above, spinlocks in single-CPU contexts produce identical code to
legacy IRQ locks. In fact the entirety of the Zephyr core kernel has now been
ported to use spinlocks exclusively.

The default spinlock implementation is built on a single atomic variable and
does not guarantee fairness between contending CPUs: it is possible for one CPU
to repeatedly win the contention, in pathological cases starving the others.
Where this matters, enabling :kconfig:option:`CONFIG_TICKET_SPINLOCKS` switches
to a ticket-based implementation that grants a contended lock to requesting CPUs
in first-come, first-served order, at the cost of a slightly larger lock object.

.. _spinlock_pool_api:

Spinlock pools
**************

A spinlock pool allows many objects to share a fixed number of spinlocks.
This reduces the memory required compared with embedding one spinlock in every object, while
permitting operations on objects assigned to different spinlocks to run concurrently. Inversely, it
also increases throughput by reducing contention on a single lock when many objects are accessed
concurrently.

Define a pool at file scope with :c:macro:`SPINLOCK_POOL_DEFINE`.

**How big should a pool be?**

The larger the pool, the fewer collisions there will be between objects, but the more static memory is used.
Given your applications runtime characteristics, you can tune the pool size to balance memory
usage and concurrency.
As peak concurrent locking entities are ,generally, bounded by the number of CPUs, a pool should be
between 2 and 4 times the number of CPUs. This to maximize concurrency while keeping the memory
footprint reasonable. After 2-4 times the number of CPUs, the drop off in collision rate will
sharply become less significant, while the memory footprint will continue to grow linearly.

**Note:**
On a uniprocessor a spinlock only masks local interrupts, and no two
contexts can ever be inside the pools critical sections at the same time, so a
pool provides no concurrency benefit.


Use :c:macro:`spinlock_find` to select the spinlock associated with an object identity:

.. code-block:: c

   SPINLOCK_POOL_DEFINE(device_locks, 16);

   void update_device(struct device_data *data)
   {
           struct k_spinlock *lock = spinlock_find(device_locks, data);
           k_spinlock_key_t key = k_spin_lock(lock);

           update_device_state(data);

           k_spin_unlock(lock, key);
   }

The lookup derives an index from the objects address. Different addresses can map to the same
spinlock, this is an expected collision and only reduces concurrency. The identity should normally
be a stable, naturally aligned object address.

Spinlock pools should not be used for objects that may access another object in the same pool while
holding a lock. This can lead to a deadlock if the two objects happen to map to the same spinlock.

Increasing the pool size reduces collisions at the cost of static memory. It does not guarantee that
each spinlock occupies a separate cache line, so adjacent spinlocks can still experience cache-line
contention on SMP systems.

See :ref:`smp_arch` for how spinlocks fit into Zephyr's SMP support.

API Reference
*************

.. doxygengroup:: spinlock_apis
