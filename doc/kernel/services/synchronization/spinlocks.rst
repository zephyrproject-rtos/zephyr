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
the lock, no ISR on the same CPU can preempt it and no other CPU can take the same lock, so neither
can observe or corrupt the protected data mid-update.
For these reasons, :c:struct:`k_spinlock` is the primary synchronization primitive used within the
Zephyr kernel itself, protecting access to its core data structures.

Application code may use it directly as well, but it is generally recommended to use a higher-level
synchronization primitive such as a :c:struct:`k_mutex` or :c:struct:`k_sem` instead.

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

        K_SPINLOCK(&lock) {
                /* critical section: exclusive access */
        }

The block must either run to its end or be left with :c:macro:`K_SPINLOCK_BREAK`.
Leaving it with a plain ``break``, ``goto`` or ``return`` skips the release and
leaks the lock:

.. code-block:: c

   K_SPINLOCK(&lock) {
           if (nothing_to_do) {
                   K_SPINLOCK_BREAK;
           }

           /* critical section: exclusive access */
   }

Acquiring without spinning
==========================

:c:func:`k_spin_trylock` makes a single attempt to take the lock and reports
failure instead of waiting, which is useful when the caller has other work to do
or must not stall. On success it stores the key, which is released exactly as for
:c:func:`k_spin_lock`:

.. code-block:: c

   k_spinlock_key_t key;

   if (k_spin_trylock(&lock, &key) == 0) {
           /* critical section: exclusive access */
           k_spin_unlock(&lock, key);
   } else {
           /* lock is held elsewhere, do something else */
   }

Rules
=====

Follow these rules when using a spinlock:

* Keep the critical section as short as possible. Interrupts are masked on the
  local CPU while the lock is held, and other CPUs may be spinning on it.
* Never perform a blocking or sleeping operation while holding a spinlock.
* Do not acquire a spinlock recursively. A context that already holds a lock
  must not try to take it again, or it will deadlock. Nesting **distinct**
  spinlocks is allowed, but must follow a consistent lock ordering to avoid
  deadlock.

Spinlocks on uniprocessor systems
*********************************

In a kernel built with :kconfig:option:`CONFIG_SMP` disabled, a spinlock does not actually spin.
With only one CPU there is nothing to contend with, so :c:func:`k_spin_lock` reduces to masking
interrupts on the local CPU.
This prevents both ISRs and context switches from observing the protected data mid-update, which is
all that is needed for mutual exclusion on a uniprocessor.

Spinlock validation
*******************

A validation layer, enabled with :kconfig:option:`CONFIG_SPIN_VALIDATE`, is available to detect
various misuses of spinlocks. It can detect the following:

* Recursive acquisition of a spinlock
* Release of a spinlock that is not held by the current context
* Out-of-order release of spinlocks (an outer lock released while an inner lock is still held)
* Context switching while a spinlock is held, or while interrupts are masked by a nested
  :c:func:`irq_lock`

On a uniprocessor system, only the recursive acquisition check is meaningful. The other misuses
cannot occur, since there is no contention and interrupts are masked while the lock is held.

Fair spinlocks
**************

The default spinlock implementation is built on a single ``atomic_t`` variable and does not
guarantee fairness between contending CPUs:
It is possible for one CPU to repeatedly win the contention, thus starving the others.
Where this matters, enabling :kconfig:option:`CONFIG_TICKET_SPINLOCKS` switches to a ticket-based
implementation that grants a contended lock to requesting CPUs in FIFO order at the cost of a
slightly larger lock object.

Suggested uses
**************

Use a spinlock to protect a short critical section that is shared between
threads and interrupt handlers, or between CPUs on an SMP system.

Prefer another primitive when it fits better:

* :c:struct:`k_mutex` for a critical section that may be long, may block, or may
  sleep. Only threads can take a mutex.
* :c:struct:`k_sem` for signalling between contexts, and for counting access to a
  pool of resources.
* :ref:`atomic services <atomic_v2>` when the shared state is a single word that
  can be updated with one atomic operation, in which case no lock is needed.

API reference
*************

.. doxygengroup:: spinlock_apis
