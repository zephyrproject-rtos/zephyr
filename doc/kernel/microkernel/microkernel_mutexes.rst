.. _microkernel_mutexes:

Mutexes
#######

Concepts
********

The microkernel's :dfn:`mutex` objects provide reentrant mutex
capabilities with basic priority inheritance.

Each mutex allows multiple tasks to safely share an associated
resource by ensuring mutual exclusivity while the resource is
being accessed by a task.

Any number of mutexes can be defined in a microkernel system.
Each mutex needs a **name** that uniquely identifies it. Typically,
the name should relate to the resource being shared, although this
is not a requirement.

A task that needs to use a shared resource must first gain exclusive
access by locking the associated mutex. If the mutex is already locked
by another task, the requesting task can wait for the mutex to be
unlocked.

After obtaining the mutex, the task may safely use the shared
resource for as long as needed. And when the task no longer needs
the resource, it must release the associated mutex to allow
other tasks to use the resource.

Any number of tasks may wait on a locked mutex. When more than one
task is waiting, the mutex locks the resource for the highest-priority
task that has waited the longest first; this is known as
:dfn:`priority-based waiting`. The order is decided when a task decides
to wait on the object: it is queued in priority order.

The task currently owning the mutex is also eligible for :dfn:`priority inheritance`.
Priority inheritance is the concept by which a task of lower priority gets its
priority *temporarily* elevated to the priority of the highest-priority
task that is waiting on a mutex held by the lower priority task. Thus, the
lower-priority task can complete its work and release the mutex as quickly
as possible. Once the mutex has been released, the lower-priority task resets
its task priority to the priority it had before locking that mutex.

.. note::

   The :option:`CONFIG_PRIORITY_CEILING` configuration option limits
   how high the kernel can raise a task's priority due to priority
   inheritance.  The default value of 0 permits unlimited elevation.

When two or more tasks wait on a mutex held by a lower priority task, the
kernel adjusts the owning task's priority each time a task begins waiting
(or gives up waiting). When the mutex is eventually released, the owning
task's priority correctly reverts to its original non-elevated priority.

The kernel does *not* fully support priority inheritance when a task holds
two or more mutexes simultaneously. This situation can result in the task's
priority not reverting to its original non-elevated priority when all mutexes
have been released. Preferably, a task holds only a single mutex when multiple
mutexes are shared between tasks of different priorities.

The microkernel also allows a task to repeatedly lock a mutex it has already
locked. This ensures that the task can access the resource at a point in its
execution when the resource may or may not already be locked. A mutex that is
repeatedly locked must be unlocked an equal number of times before the mutex
can release the resource completely.

Purpose
*******

Use mutexes to provide exclusive access to a resource, such as a physical
device.

Usage
*****

Defining a Mutex
================

The following parameters must be defined:

   *name*
          This specifies a unique name for the mutex.

Public Mutex
------------

Define the mutex in the application's MDEF file with the following syntax:

.. code-block:: console

   MUTEX name

For example, the file :file:`projName.mdef` defines a single mutex as follows:

.. code-block:: console

   % MUTEX  NAME
   % ===============
     MUTEX  DEVICE_X

A public mutex can be referenced by name from any source file that includes
the file :file:`zephyr.h`.

Private Mutex
-------------

Define the mutex in a source file using the following syntax:

.. code-block:: c

   DEFINE_MUTEX(name);

For example, the following code defines a private mutex named ``XYZ``.

.. code-block:: c

   DEFINE_MUTEX(XYZ);

The following syntax allows this mutex to be accessed from a different
source file:

.. code-block:: c

   extern const kmutex_t XYZ;


Example: Locking a Mutex with No Conditions
===========================================

This code waits indefinitely for the mutex to become available if the
mutex is in use.

.. code-block:: c

   task_mutex_lock(XYZ, TICKS_UNLIMITED);
   moveto(100,100);
   lineto(200,100);
   task_mutex_unlock(XYZ);


Example: Locking a Mutex with a Conditional Timeout
===================================================

This code waits for a mutex to become available for a specified
time, and gives a warning if the mutex does not become available
in the specified amount of time.

.. code-block:: c

   if (task_mutex_lock(XYZ, 100) == RC_OK)
    {
     moveto(100,100);
     lineto(200,100);
     task_mutex_unlock(XYZ);
    }
   else
    {
     printf("Cannot lock XYZ display\n");
    }


Example: Locking a Mutex with a No Blocking Condition
=====================================================

This code gives an immediate warning when a mutex is in use.

.. code-block:: c

   if (task_mutex_lock(XYZ, TICKS_NONE) == RC_OK);
    {
     do_something();
     task_mutex_unlock(XYZ); /* and unlock mutex*/
    }
   else
    {
     display_warning(); /* and do not unlock mutex*/
    }

APIs
****

Mutex APIs provided by :file:`microkernel.h`
============================================

:cpp:func:`task_mutex_lock()`
   Wait on a locked mutex for the period of time defined by the timeout
   parameter. Lock the mutex and increment the lock count if the mutex
   becomes available during that period.

:cpp:func:`task_mutex_unlock()`
   Decrement a mutex lock count, and unlock the mutex when the count
   reaches zero.

