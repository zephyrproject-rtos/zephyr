.. _lifecycle_v2:

Lifecycle
#########

A :dfn:`thread` is a kernel object that is used for application processing
that is too lengthy or too complex to be performed by an ISR.

.. contents::
    :local:
    :depth: 2

Concepts
********

Any number of threads can be definedby an application. Each thread is
referenced by its memory address.

A thread has the following key properties:

* A **thread region**, which is the area of memory used by the thread
  and for its stack. The **size** of the thread region can be tailored
  to meet the specific needs of the thread.

* An **entry point function**, which is invoked when the thread is started.
  Up to 3 **argument values** can be passed to this function.

* A **scheduling priority**, which instructs the kernel's scheduler how to
  allocate CPU time to the thread. (See "Thread Scheduling".)

* A **start delay**, which specifies how long the kernel should wait before
  starting the thread.

Thread Spawning
===============

A thread must be spawned before it can be used. The kernel initializes
both the thread data structure portion and the stack portion of
the thread's thread region.

Specifying a start delay of :c:macro:`K_NO_WAIT` instructs the kernel
to start thread execution immediately. Alternatively, the kernel can be
instructed to delay execution of the thread by specifying a timeout
value -- for example, to allow device hardware used by the thread
to become available.

The kernel allows a delayed start to be cancelled before the thread begins
executing. A cancellation request has no effect if the thread has already
started. A thread whose delayed start was successfully cancelled must be
re-spawned before it can be used.

Thread Termination
==================

Once a thread is started it typically executes forever. However, a thread may
synchronously end its execution by returning from its entry point function.
This is known as **termination**.

A thread that terminates is responsible for releasing any shared resources
it may own (such as mutexes and dynamically allocated memory)
prior to returning, since the kernel does *not* reclaim them automatically.

.. note::
    The kernel does not currently make any claims regarding an application's
    ability to respawn a thread that terminates.

Thread Aborting
===============

A thread may asynchronously end its execution by **aborting**. The kernel
automatically aborts a thread if the thread triggers a fatal error condition,
such as dereferencing a null pointer.

A thread can also be aborted by another thread (or by itself)
by calling :c:func:`k_thread_abort()`. However, it is typically preferable
to signal a thread to terminate itself gracefully, rather than aborting it.

As with thread termination, the kernel does not reclaim shared resources
owned by an aborted thread.

.. note::
    The kernel does not currently make any claims regarding an application's
    ability to respawn a thread that aborts.

Thread Suspension
=================

A thread can be prevented from executing for an indefinite period of time
if it becomes **suspended**. The function :c:func:`k_thread_suspend()`
can be used to suspend any thread, including the calling thread.
Suspending a thread that is already suspended has no additional effect.

Once suspended, a thread cannot be scheduled until another thread calls
:c:func:`k_thread_resume()` to remove the suspension.

.. note::
   A thread can prevent itself from executing for a specified period of time
   using :c:func:`k_sleep()`. However, this is different from suspending
   a thread since a sleeping thread becomes executable automatically when the
   time limit is reached.

Implementation
**************

Spawning a Thread
=================

A thread is spawned by defining its thread region and then calling
:cpp:func:`k_thread_spawn()`. The thread region is an array of bytes
whose size must equal :c:func:`sizeof(struct k_thread)` plus the size
of the thread's stack. The thread region must be defined using the
:c:macro:`__stack` attribute to ensure it is properly aligned.

The thread spawning function returns the thread's memory address,
which can be saved for later reference. Alternatively, the address of
the thread can be obtained by casting the address of the thread region
to type :c:type:`struct k_thread *`.

The following code spawns a thread that starts immediately.

.. code-block:: c

    #define MY_THREAD_SIZE 500
    #define MY_PRIORITY 5

    extern void my_entry_point(void *, void *, void *);

    char __noinit __stack my_thread_area[MY_THREAD_SIZE];

    struct k_thread *my_thread_ptr;

    my_thread_ptr = k_thread_spawn(my_thread_area, MY_THREAD_SIZE,
                                   my_entry_point, 0, 0, 0,
                                   MY_PRIORITY, 0, K_NO_WAIT);

Alternatively, a thread can be spawned at compile time by calling
:c:macro:`K_THREAD_DEFINE()`. Observe that the macro defines the thread
region automatically, as well as a variable containing the thread's address.

The following code has the same effect as the code segment above.

.. code-block:: c

    K_THREAD_DEFINE(my_thread_ptr, my_thread_area, MY_THREAD_SIZE,
                                   my_entry_point, 0, 0, 0,
                                   MY_PRIORITY, 0, K_NO_WAIT);

.. note::
   NEED TO FIGURE OUT HOW WE'RE GOING TO HANDLE THE FLOATING POINT OPTIONS!

Terminating a Thread
====================

A thread terminates itself by returning from its entry point function.

The following code illustrates the ways a thread can terminate.

.. code-block:: c

    void my_entry_point(int unused1, int unused2, int unused3)
    {
        while (1) {
            ...
	    if (<some condition>) {
	        return; /* thread terminates from mid-entry point function */
	    }
	    ...
        }

        /* thread terminates at end of entry point function */
    }


Suggested Uses
**************

Use threads to handle processing that cannot be handled in an ISR.

Use separate threads to handle logically distinct processing operations
that can execute in parallel.

Configuration Options
*********************

Related configuration options:

* None.

APIs
****

The following thread APIs are are provided by :file:`kernel.h`:

* :cpp:func:`k_thread_spawn()`
* :cpp:func:`k_thread_cancel()`
* :cpp:func:`k_thread_abort()`
* :cpp:func:`k_thread_suspend()`
* :cpp:func:`k_thread_resume()`
