.. _nanokernel_fibers:

Fiber Services
##############

Concepts
********

A fiber is a lightweight, non-preemptible thread of execution that implements
a portion of an application's processing. It is is normally used when writing
device drivers and other performance critical work.

Fibers can be used by microkernel applications, as well as by nanokernel
applications. However, fibers can only interact with microkernel object types
to a very limited degree; for more information see
:ref:`Microkernel Fiber Services <microkernel_fibers>`.

An application can use any number of fibers. Each fiber is anonymous, and
cannot be directly referenced by other fibers or tasks once it has started
executing. The properties that must be specified when a fiber is spawned
include:

* a memory region that is used for its stack and for other execution context
  information,
* a function that is invoked when the fiber starts executing,
* a pair of arguments that are passed to that entry point function,
* a priority that is used by the nanokernel scheduler, and
* a set of options that apply to the fiber.

The kernel may automatically spawn zero or more system fibers during system
initialization. The specific set of fibers spawned depends on the kernel
capabilities that have been configured by the application and by the
platform configuration used to build the application image.

Fiber Lifecycle
===============

A fiber can be spawned by another fiber, by a task, or by the kernel itself
during system initialization. A fiber typically becomes executable immediately;
however it is possible to delay the scheduling of a newly spawned fiber
for a specified time period---for example, to allow device hardware which
the fiber uses to become available. The kernel also supports a delayed start
cancellation capability, which prevents a newly spawned fiber from executing
if the fiber becomes unnecessary before its full delay period is reached.

Once a fiber is started it normally executes forever. A fiber may terminate
itself gracefully by simply returning from its entry point function. If it
does, it is the fiber's responsibility to release any system resources it may
own (such as a nanokernel semaphore being used in a mutex-like manner) prior
to returning, since the kernel does *not* attempt to reclaim them so they can
be reused.

A fiber may also terminate non-gracefully by *aborting*. The kernel
automatically aborts a fiber when it generates a fatal error condition,
such as dereferencing a null pointer. A fiber can also explicitly abort itself
using :cpp:func:`fiber_abort()`. As with graceful fiber termination, the kernel
does not attempt to reclaim system resources owned by the fiber.

.. note::
   The kernel does not currently make any claims regarding an application's
   ability to restart a terminated fiber.

Fiber Scheduling
================

The nanokernel's scheduler selects which of the system's threads is allowed
to execute; this thread is known as the *current context*. The nanokernel's
scheduler permits threads to execute only when there is no ISR that needs
to execute, since ISR execution takes precedence.

When executing threads, the nanokernel's scheduler gives fiber execution
precedence over task execution. The scheduler preempts task execution
whenever a fiber needs to execute, but never preempts the execution of a fiber
to allow another fiber to execute---even if it is a higher priority fiber.

The kernel automatically takes care of saving an executing fiber's CPU register
values when it performs a context switch to a different fiber, a task, or
an ISR, and restores these values when the fiber later resumes execution.

Fiber State
-----------

A fiber has an implicit *state* that determines whether or not it can be
scheduled for execution. The state records all factors that can prevent
the fiber from executing, such as:

* the fiber has not been spawned
* the fiber is waiting for it needs (e.g. a semaphore, a timeout, ...)
* the fiber has terminated

A fiber whose state has no factors that prevent its execution is said to be
*executable*.

Fiber Priorities
----------------

The kernel supports a virtually unlimited number of fiber priority levels,
ranging from 0 (highest priority) to 2^31-1 (lowest priority). Negative
priority levels must not be used.

A fiber's original priority cannot be altered up or down after it has been
spawned.

Fiber Scheduling Algorithm
--------------------------

The nanokernel's scheduler selects the highest priority executable fiber
to be the current context, if possible. If multiple executable fibers
of that priority are available the scheduler chooses the one that has been
waiting longest.

If no executable fibers exist the scheduler selects the current task
to be the current context. In a nanokernel application the current task is
the background task, while in a microkernel application it is the current task
selected by the microkernel's scheduler. The current task is always executable.

Once a fiber becomes the current context it remains scheduled for execution
by the nanokernel until one of the following occurs:

* The fiber is supplanted by another thread because it calls a kernel API
  that blocks its own execution. (For example, the fiber attempts to take
  a nanokernel semaphore that is unavailable.)

* The fiber terminates itself by returning from its entry point function.

* The fiber aborts itself by performing an operation that causes a fatal error,
  or by calling :cpp:func:`fiber_abort()`.

Once the current task becomes the current context it remains scheduled for
execution by the nanokernel until is supplanted by a fiber.

.. note::
   The current task is never directly supplanted by another task, since the
   microkernel scheduler uses the microkernel server fiber to initiate a
   change from one microkernel task to another.

Cooperative Time Slicing
------------------------

Due to the non-preemptive nature of the nanokernel's scheduler, a fiber that
performs lengthy computations may cause an unacceptable delay in the scheduling
of other fibers, including higher priority and equal priority ones. To overcome
such problems the fiber can choose to voluntarily relinquish the CPU from time
to time to permit other fibers to execute.

A fiber can relinquish the CPU in two ways:

* Calling :cpp:func:`fiber_yield()` places the fiber back in the nanokernel
  scheduler's list of executable fibers and then invokes the scheduler.
  All executable fibers whose priority is higher or equal to that of the
  yielding fiber are then allowed to execute before the yielding fiber is
  rescheduled. If no such executable fibers exist, the scheduler immediately
  reschedules the yielding fiber without context switching.

* Calling :cpp:func:`fiber_sleep()` blocks the execution of the fiber for
  a specified time period. Executable fibers of all priorities are then
  allowed to execute, although there is no guarantee that fibers whose
  priority is lower than that of the sleeping task will actually be scheduled
  before the time period expires and the sleeping task becomes executable
  once again.

Fiber Options
=============

The kernel supports several *fiber options* that inform the kernel about
special treatment the fiber requires.

The set of kernel options associated with a fiber are specified when the fiber
is spawned. If the fiber uses multiple options they are separated using
:literal:`|`; i.e. the logical OR operator. A fiber that does not use any
options is spawned using an options value of 0.

The fiber options listed below are pre-defined by the kernel.

:c:macro:`USE_FP`
      Instructs the kernel to save the fiber's x87 FPU and MMX floating point
      context information during context switches.

:c:macro:`USE_SSE`
      Instructs the kernel to save the fiber's SSE floating point context
      information during context switches. (A fiber using this option
      implicitly uses the :c:macro:`USE_FP` option too.)

Usage
*****

Defining a Fiber
================

The following properties must be defined when spawning a fiber:

   *stack_name*
      This specifies the memory region used for the fiber's stack and for
      other execution context information. To ensure proper memory alignment,
      it should have the following form:

      .. code-block:: c

         char __stack <stack_name>[<stack_size>];

   *stack_size*
      This specifies the size of the *stack_name* memory region, in bytes.

   *entry_point*
      This specifies the name of the fiber's entry point function,
      which should have the following form:

      .. code-block:: c

         void <entry_point>(int arg1, int arg2)
         {
             /* fiber mainline processing */
             ...
             /* (optional) normal fiber termination */
             return;
         }

   *arguments*
      This specifies the two arguments passed to *entry_point* when the fiber
      begins executing. Non-integer arguments can be passed in by casting to
      an integer type.

   *priority*
      This specifies the scheduling priority of the fiber.

   *options*
      This specifies the fiber's options.

Example: Spawning a Fiber from a Task
=====================================

This code shows how the currently executing task can spawn multiple fibers,
each dedicated to processing data from a different communication channel.

.. code-block:: c

   #define COMM_STACK_SIZE    512
   #define NUM_COMM_CHANNELS  8

   struct descriptor {
       ...;
   };

   char __stack comm_stack[NUM_COMM_CHANNELS][COMM_STACK_SIZE];
   struct descriptor comm_desc[NUM_COMM_CHANNELS] = { ... };

   ...

   void comm_fiber(int desc_arg, int unused);
   {
       ARG_UNUSED(unused);

       struct descriptor  *desc = (struct descriptor *) desc_arg;

       while (1) {
           /* process packet of data from comm channel */

           ...
       }
   }

   void comm_main(void)
   {
       ...

       for (int i = 0; i < NUM_COMM_CHANNELS; i++) {
           task_fiber_start(&comm_stack[i][0], COMM_STACK_SIZE,
                            comm_fiber, (int) &comm_desc[i], 0,
                            10, 0);
       }

       ...
   }

APIs
****

The following APIs affecting the currently executing fiber are provided
by :file:`microkernel.h` and by :file:`nanokernel.h`:

:cpp:func:`fiber_yield()`
   Yields CPU to higher priority and equal priority fibers.

:cpp:func:`fiber_sleep()`
   Yields CPU for a specified time period.

:cpp:func:`fiber_abort()`
   Terminates fiber execution.

The following APIs affecting a specified fiber are provided
by :file:`microkernel.h` and by :file:`nanokernel.h`:

:c:func:`task_fiber_start()`, :cpp:func:`fiber_fiber_start()`,
:cpp:func:`fiber_start()`
   Spawn a new fiber.

:c:func:`task_fiber_delayed_start()`,
:cpp:func:`fiber_fiber_delayed_start()`,
:cpp:func:`fiber_delayed_start()`
   Spawn a new fiber after a specified time period.

:c:func:`task_fiber_delayed_start_cancel()`,
:cpp:func:`fiber_fiber_delayed_start_cancel()`,
:cpp:func:`fiber_delayed_start_cancel()`
   Cancel spawning of a new fiber, if not already started.