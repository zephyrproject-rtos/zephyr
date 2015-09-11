.. _microkernel_mutexes:

Mutexes
#######

Concepts
********

The microkernel's mutex objects provide reentrant mutex
capabilities with basic priority inheritance.

Each mutex allows multiple tasks to safely share an associated
resource by ensuring mutual exclusivity while the resource is
being accessed by a task.

Any number of mutexes can be defined in a microkernel system.
Each mutex has a name that uniquely identifies it. Typically,
the name should relate to the resource being shared, but this is
not a requirement.

A task that needs to use a shared resource must first gain
exclusive access by locking the associated mutex. When the mutex
is already locked by another task, the requesting task may
choose to wait for the mutex to be unlocked. After obtaining the lock,
a task can safely use the shared resource for as long as needed.
When the task no longer needs the resource, it must unlock the
associated mutex to allow other tasks to use the resource.

Any number of tasks may wait on a locked mutex simultaneously.
When there is more than one waiting task, the mutex locks the
resource for the highest priority task that has waited the longest
first.

Priority inheritance occurs whenever a high priority task waits
on a mutex that is locked by a lower priority task. The priority
of the lower priority task increases temporarily to the priority
of the highest priority task that is waiting on a mutex held by
the lower priority task. This allows the lower priority
task to complete its work and unlock the mutex as quickly as
possible. Once the mutex has been unlocked, the lower priority task
sets its task priority to the priority it had prior to locking that
mutex.

.. note::

   The :option:`PRIORITY_CEILING` configuration option limits how high
   the kernel can raise a task's priority due to priority inheritance.
   The default value of 0 permits unlimited elevation.

When two or more tasks wait on a mutex held by a lower priority task, the
kernel adjusts the owning task's priority each time a task begins waiting
(or gives up waiting). When the mutex is eventually released the owning
task's priority correctly reverts to its original non-elevated priority.

.. note::

   The kernel does *not* fully support priority inheritance when a task
   holds two or more mutexes simultaneously. This situation can result
   in the task's priority not reverting to its original non-elevated
   priority when all mutexes have been released. It is recommended that
   a task hold only a single mutex at a time when multiple mutexes are
   shared between tasks of different priorities.

The microkernel also allows a task to repeatedly lock a mutex it
already locked. This ensures that the task can access the resource
at a point in its execution when the resource may or may not
already be locked. A mutex that is repeatedly locked must be unlocked
an equal number of times before the mutex releases the resource
completely.

Purpose
*******
Use mutexes to provide exclusive access to a resource,
such as a physical device.


Usage
*****

Defining a Mutex
================

The following parameters must be defined:

   *name*
          This specifies a unique name for the mutex.


Public Mutex
------------

Define the mutex in the application's MDEF using the following syntax:

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

To utilize this mutex from a different source file use the following syntax:

.. code-block:: c

   extern const kmutex_t XYZ;


Example: Locking a Mutex with No Conditions
===========================================

This code waits indefinitely for the mutex to become available if the
mutex is in use.

.. code-block:: c

   task_mutex_lock_wait(XYZ);
   moveto(100,100);
   lineto(200,100);
   task_mutex_unlock(XYZ);


Example: Locking a Mutex with a Conditional Timeout
===================================================

This code waits for a mutex to become available for a specified
time, and gives a warning if the mutex does not become available
in the specified amount of time.

.. code-block:: c

   if (task_mutex_lock_wait_timeout(XYZ, 100) == RC_OK)
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

   if (task_mutex_lock(XYZ) == RC_OK);
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

The following Mutex APIs are provided by :file:`microkernel.h`.

+------------------------------------------+-----------------------------------+
| Call                                     | Description                       |
+==========================================+===================================+
| :c:func:`task_mutex_lock()`              | Locks a mutex, and increments     |
|                                          | the lock count.                   |
+------------------------------------------+-----------------------------------+
| :c:func:`task_mutex_lock_wait()`         | Waits on a locked mutex until it  |
|                                          | is unlocked, then locks the mutex |
|                                          | and increments the lock count.    |
+------------------------------------------+-----------------------------------+
| :c:func:`task_mutex_lock_wait_timeout()` | Waits on a locked mutex for       |
|                                          | the period of time defined by     |
|                                          | the timeout parameter. If the     |
|                                          | mutex becomes available during    |
|                                          | that period, the function         |
|                                          | locks the mutex, and              |
|                                          | increments the lock count.        |
|                                          | If the timeout expires, it        |
|                                          | returns RC_TIME.                  |
+------------------------------------------+-----------------------------------+
| :c:func:`task_mutex_unlock()`            | Decrements a mutex lock count,    |
|                                          | and unlocks the mutex when the    |
|                                          | count reaches zero.               |
+------------------------------------------+-----------------------------------+
