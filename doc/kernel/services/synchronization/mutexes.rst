.. _mutexes_v2:

Mutexes
#######

A :dfn:`mutex` is a kernel object that implements a traditional
reentrant mutex. A mutex allows multiple threads to safely share
an associated hardware or software resource by ensuring mutually exclusive
access to the resource.

.. contents::
    :local:
    :depth: 2

Concepts
********

Any number of mutexes can be defined (limited only by available RAM). Each mutex
is referenced by its memory address.

A mutex has the following key properties:

* A **lock count** that indicates the number of times the mutex has be locked
  by the thread that has locked it. A count of zero indicates that the mutex
  is unlocked.

* An **owning thread** that identifies the thread that has locked the mutex,
  when it is locked.

A mutex must be initialized before it can be used. This sets its lock count
to zero.

A thread that needs to use a shared resource must first gain exclusive rights
to access it by **locking** the associated mutex. If the mutex is already locked
by another thread, the requesting thread may choose to wait for the mutex
to be unlocked.

After locking a mutex, the thread may safely use the associated resource
for as long as needed; however, it is considered good practice to hold the lock
for as short a time as possible to avoid negatively impacting other threads
that want to use the resource. When the thread no longer needs the resource
it must **unlock** the mutex to allow other threads to use the resource.

Any number of threads may wait on a locked mutex simultaneously.
When the mutex becomes unlocked it is then locked by the highest-priority
thread that has waited the longest.

.. note::
    Mutex objects are *not* designed for use by ISRs.

Reentrant Locking
=================

A thread is permitted to lock a mutex it has already locked.
This allows the thread to access the associated resource at a point
in its execution when the mutex may or may not already be locked.

A mutex that is repeatedly locked by a thread must be unlocked an equal number
of times before the mutex becomes fully unlocked so it can be claimed
by another thread.

Priority Inheritance
====================

The thread that has locked a mutex is eligible for :dfn:`priority inheritance`.
This means the kernel will *temporarily* elevate the thread's priority
if a higher priority thread begins waiting on the mutex. This allows the owning
thread to complete its work and release the mutex more rapidly by executing
at the same priority as the waiting thread. Once the mutex has been unlocked,
the unlocking thread resets its priority to the level it had before locking
that mutex.

.. note::
    The :kconfig:option:`CONFIG_PRIORITY_CEILING` configuration option limits
    how high the kernel can raise a thread's priority due to priority
    inheritance. The default value of 0 permits unlimited elevation.

The owning thread's base priority is saved in the mutex when it obtains the
lock. Each time a higher priority thread waits on a mutex, the kernel adjusts
the owning thread's priority. When the owning thread releases the lock (or if
the high priority waiting thread times out), the kernel restores the thread's
base priority from the value saved in the mutex.

This works well for priority inheritance as long as only one locked mutex is
involved. However, if multiple mutexes are involved, sub-optimal behavior will
be observed if the mutexes are not unlocked in the reverse order to which the
owning thread's priority was previously raised. Consequently it is recommended
that a thread lock only a single mutex at a time when multiple mutexes are
shared between threads of different priorities.

Implementation
**************

Defining a Mutex
================

A mutex is defined using a variable of type :c:struct:`k_mutex`.
It must then be initialized by calling :c:func:`k_mutex_init`.

The following code defines and initializes a mutex.

.. code-block:: c

    struct k_mutex my_mutex;

    k_mutex_init(&my_mutex);

Alternatively, a mutex can be defined and initialized at compile time
by calling :c:macro:`K_MUTEX_DEFINE`.

The following code has the same effect as the code segment above.

.. code-block:: c

    K_MUTEX_DEFINE(my_mutex);

Locking a Mutex
===============

A mutex is locked by calling :c:func:`k_mutex_lock`.

The following code builds on the example above, and waits indefinitely
for the mutex to become available if it is already locked by another thread.

.. code-block:: c

    k_mutex_lock(&my_mutex, K_FOREVER);

The following code waits up to 100 milliseconds for the mutex to become
available, and gives a warning if the mutex does not become available.

.. code-block:: c

    if (k_mutex_lock(&my_mutex, K_MSEC(100)) == 0) {
        /* mutex successfully locked */
    } else {
        printf("Cannot lock XYZ display\n");
    }

Unlocking a Mutex
=================

A mutex is unlocked by calling :c:func:`k_mutex_unlock`.

The following code builds on the example above,
and unlocks the mutex that was previously locked by the thread.

.. code-block:: c

    k_mutex_unlock(&my_mutex);

Suggested Uses
**************

Use a mutex to provide exclusive access to a resource, such as a physical
device.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_PRIORITY_CEILING`

API Reference
*************

.. doxygengroup:: mutex_apis

Futex API Reference
*******************

k_futex is a lightweight mutual exclusion primitive designed to minimize
kernel involvement. Uncontended operation relies only on atomic access
to shared memory. k_futex are tracked as kernel objects and can live in
user memory so that any access bypasses the kernel object permission
management mechanism.

.. doxygengroup:: futex_apis

User Mode Mutex API Reference
*****************************

sys_mutex behaves almost exactly like k_mutex, with the added advantage
that a sys_mutex instance can reside in user memory. When user mode isn't
enabled, sys_mutex behaves like k_mutex.

.. doxygengroup:: user_mutex_apis
