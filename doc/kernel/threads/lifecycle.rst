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

Any number of threads can be defined by an application. Each thread is
referenced by a :dfn:`thread id` that is assigned when the thread is spawned.

A thread has the following key properties:

* A **stack area**, which is a region of memory used for the thread's stack.
  The **size** of the stack area can be tailored to conform to the actual needs
  of the thread's processing. Special macros exist to create and work with
  stack memory regions.

* A **thread control block** for private kernel bookkeeping of the thread's
  metadata. This is an instance of type :c:type:`struct k_thread`.

* An **entry point function**, which is invoked when the thread is started.
  Up to 3 **argument values** can be passed to this function.

* A **scheduling priority**, which instructs the kernel's scheduler how to
  allocate CPU time to the thread. (See :ref:`scheduling_v2`.)

* A set of **thread options**, which allow the thread to receive special
  treatment by the kernel under specific circumstances.
  (See :ref:`thread_options_v2`.)

* A **start delay**, which specifies how long the kernel should wait before
  starting the thread.

* An **execution mode**, which can either be supervisor or user mode.
  By default, threads run in supervisor mode and allow access to
  privileged CPU instructions, the entire memory address space, and
  peripherals. User mode threads have a reduced set of privileges.
  This depends on the :option:`CONFIG_USERSPACE` option. See :ref:`usermode`.

.. _spawning_thread:

Thread Creation
===============

A thread must be created before it can be used. The kernel initializes
the thread control block as well as one end of the stack portion. The remainder
of the thread's stack is typically left uninitialized.

Specifying a start delay of :c:macro:`K_NO_WAIT` instructs the kernel
to start thread execution immediately. Alternatively, the kernel can be
instructed to delay execution of the thread by specifying a timeout
value -- for example, to allow device hardware used by the thread
to become available.

The kernel allows a delayed start to be canceled before the thread begins
executing. A cancellation request has no effect if the thread has already
started. A thread whose delayed start was successfully canceled must be
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
by calling :cpp:func:`k_thread_abort()`. However, it is typically preferable
to signal a thread to terminate itself gracefully, rather than aborting it.

As with thread termination, the kernel does not reclaim shared resources
owned by an aborted thread.

.. note::
    The kernel does not currently make any claims regarding an application's
    ability to respawn a thread that aborts.

Thread Suspension
=================

A thread can be prevented from executing for an indefinite period of time
if it becomes **suspended**. The function :cpp:func:`k_thread_suspend()`
can be used to suspend any thread, including the calling thread.
Suspending a thread that is already suspended has no additional effect.

Once suspended, a thread cannot be scheduled until another thread calls
:cpp:func:`k_thread_resume()` to remove the suspension.

.. note::
   A thread can prevent itself from executing for a specified period of time
   using :cpp:func:`k_sleep()`. However, this is different from suspending
   a thread since a sleeping thread becomes executable automatically when the
   time limit is reached.

.. _thread_options_v2:

Thread Options
==============

The kernel supports a small set of :dfn:`thread options` that allow a thread
to receive special treatment under specific circumstances. The set of options
associated with a thread are specified when the thread is spawned.

A thread that does not require any thread option has an option value of zero.
A thread that requires a thread option specifies it by name, using the
:literal:`|` character as a separator if multiple options are needed
(i.e. combine options using the bitwise OR operator).

The following thread options are supported.

:c:macro:`K_ESSENTIAL`
    This option tags the thread as an :dfn:`essential thread`. This instructs
    the kernel to treat the termination or aborting of the thread as a fatal
    system error.

    By default, the thread is not considered to be an essential thread.

:c:macro:`K_FP_REGS` and :c:macro:`K_SSE_REGS`
    These x86-specific options indicate that the thread uses the CPU's
    floating point registers and SSE registers, respectively. This instructs
    the kernel to take additional steps to save and restore the contents
    of these registers when scheduling the thread.
    (For more information see :ref:`float_v2`.)

    By default, the kernel does not attempt to save and restore the contents
    of these registers when scheduling the thread.

:c:macro:`K_USER`
    If :option:`CONFIG_USERSPACE` is enabled, this thread will be created in
    user mode and will have reduced privileges. See :ref:`usermode`. Otherwise
    this flag does nothing.

:c:macro:`K_INHERIT_PERMS`
    If :option:`CONFIG_USERSPACE` is enabled, this thread will inherit all
    kernel object permissions that the parent thread had, except the parent
    thread object.  See :ref:`usermode`.

Implementation
**************

Spawning a Thread
=================

A thread is spawned by defining its stack area and its thread control block,
and then calling :cpp:func:`k_thread_create()`. The stack area must be defined
using :c:macro:`K_THREAD_STACK_DEFINE` to ensure it is properly set up in
memory.

The thread spawning function returns its thread id, which can be used
to reference the thread.

The following code spawns a thread that starts immediately.

.. code-block:: c

    #define MY_STACK_SIZE 500
    #define MY_PRIORITY 5

    extern void my_entry_point(void *, void *, void *);

    K_THREAD_STACK_DEFINE(my_stack_area, MY_STACK_SIZE);
    struct k_thread my_thread_data;

    k_tid_t my_tid = k_thread_create(&my_thread_data, my_stack_area,
                                     K_THREAD_STACK_SIZEOF(my_stack_area),
                                     my_entry_point,
                                     NULL, NULL, NULL,
                                     MY_PRIORITY, 0, K_NO_WAIT);

Alternatively, a thread can be spawned at compile time by calling
:c:macro:`K_THREAD_DEFINE`. Observe that the macro defines
the stack area, control block, and thread id variables automatically.

The following code has the same effect as the code segment above.

.. code-block:: c

    #define MY_STACK_SIZE 500
    #define MY_PRIORITY 5

    extern void my_entry_point(void *, void *, void *);

    K_THREAD_DEFINE(my_tid, MY_STACK_SIZE,
                    my_entry_point, NULL, NULL, NULL,
                    MY_PRIORITY, 0, K_NO_WAIT);

User Mode Constraints
---------------------

This section only applies if :option:`CONFIG_USERSPACE` is enabled, and a user
thread tries to create a new thread. The :c:func:`k_thread_create()` API is
still used, but there are additional constraints which must be met or the
calling thread will be terminated:

* The calling thread must have permissions granted on both the child thread
  and stack parameters; both are tracked by the kernel as kernel objects.

* The child thread and stack objects must be in an uninitialized state,
  i.e. it is not currently running and the stack memory is unused.

* The stack size parameter passed in must be equal to or less than the
  bounds of the stack object when it was declared.

* The :c:macro:`K_USER` option must be used, as user threads can only create
  other user threads.

* The :c:macro:`K_ESSENTIAL` option must not be used, user threads may not be
  considered essential threads.

* The priority of the child thread must be a valid priority value, and equal to
  or lower than the parent thread.

Dropping Permissions
====================

If :option:`CONFIG_USERSPACE` is enabled, a thread running in supervisor mode
may perform a one-way transition to user mode using the
:cpp:func:`k_thread_user_mode_enter()` API. This is a one-way operation which
will reset and zero the thread's stack memory. The thread will be marked
as non-essential.

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

If CONFIG_USERSPACE is enabled, aborting a thread will additionally mark the
thread and stack objects as uninitialized so that they may be re-used.

Suggested Uses
**************

Use threads to handle processing that cannot be handled in an ISR.

Use separate threads to handle logically distinct processing operations
that can execute in parallel.

Configuration Options
*********************

Related configuration options:

* :option:`CONFIG_USERSPACE`

APIs
****

The following thread APIs are provided by :file:`kernel.h`:

* :c:macro:`K_THREAD_DEFINE`
* :cpp:func:`k_thread_create()`
* :cpp:func:`k_thread_abort()`
* :cpp:func:`k_thread_suspend()`
* :cpp:func:`k_thread_resume()`
* :c:macro:`K_THREAD_STACK_DEFINE`
* :c:macro:`K_THREAD_STACK_ARRAY_DEFINE`
* :c:macro:`K_THREAD_STACK_MEMBER`
* :c:macro:`K_THREAD_STACK_SIZEOF`
* :c:macro:`K_THREAD_STACK_BUFFER`
