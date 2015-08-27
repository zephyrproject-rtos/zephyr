.. _nanokernel_fibers:

Fiber Services
##############

A fiber is an execution thread and a lightweight alternative to a task. It can
use nanokernel objects but not microkernel objects. A runnable fiber will
preempt the execution of any task but it will not preempt the execution of
another fiber.


Defining Fibers
***************

A fiber is defined as a routine that takes two 32-bit values as
arguments and returns a void within the application, for example:

.. code-block:: c

   void fiber ( int arg1, int arg2 );

.. note::

   A pointer can be passed to a fiber as one of the parameters but it
   must be cast to a 32-bit integer.

Unlike a nanokernel task, a fiber cannot be defined within the project
file.

Fibers can be written in assembly. How to code a fiber in assembly is
beyond the scope of this document.


Starting a Fiber
****************

A nanokernel fiber must be explicitly started by calling
:cpp:func:`fiber_fiber_start()` or :cpp:func:`task_fiber_start()` to create
and start a fiber. The function :cpp:func:`fiber_fiber_start()` creates
and starts a fiber from another fiber, while
:cpp:func:`task_fiber_start()` does so from a task. Both APIs use the
parameters *parameter1* and *parameter2* as *arg1* and *arg2* given to
the fiber . The full documentation on these APIs can be found in the
:ref:`in-code_apis`.

When :cpp:func:`task_fiber_start()` is called from a task, the new fiber
will be immediately ready to run. The background task immediately stops
execution, yielding to the new fiber until the fiber calls a blocking
service that de-schedules it. If the fiber performs a return from the
routine in which it started, the fiber is terminated, and its stack can
then be reused or de-allocated.


Fiber Stack Definition
**********************

The fiber stack is used for local variables and for calling functions or
subroutines. Additionally, the first locations on the stack are used by
the kernel for the context control structure. Allocate or declare the
fiber stack prior to calling :cpp:func:`fiber_fiber_start()`. A fiber
stack can be any sort of buffer, as long as it is aligned on a 4-byte
boundary. The recommended way of setting up a stack is using a char array
and tagging the stack variable with the __stack attribute:

.. code-block::cpp

   char __stack my_fiber_stack[256];

The size of the fiber stack can be set freely. It is recommended to
start with a stack much larger than you think you need, say 1 KB for a
simple fiber, and then reduce it after testing the functionality of the
fiber to optimize memory usage. The number of local variables and of
function calls with large local variables determine the required stack
size.


Stopping a Fiber
****************

There are no APIs to stop or suspend a fiber. Only one API can influence
the scheduling of a fiber, :cpp:func:`fiber_yield()`. When a fiber yields
itself, the nanokernel checks for another runnable fiber of the same or
higher priority. If a fiber of the same priority or higher is found, a
context switch occurs. If no other fibers are ready to execute, or if
all the runnable fibers have a lower priority than the currently
running fiber, the nanokernel does not perform any scheduling allowing
the running fiber to continue. A task or an ISR cannot call
:cpp:func:`fiber_yield()`.

If a fiber executes lengthy computations that will introduce an
unacceptable delay in the scheduling of other fibers, it should yield
by placing a :cpp:func:`fiber_yield()` call within the loop of a
computational cannot call :cpp:func:`fiber_yield()`.

Fiber Scheduling Model
######################

The fibers in the Zephyr Kernel are priority-scheduled. When several fibers
are ready to run, they run in the order of their priority. When more
than one fiber of the same priority is ready to run, they are ordered
by the time that each became runnable. Each fiber runs until it is
unscheduled by an invoked kernel service or until it terminates. Using
prioritized fibers, avoiding interrupts, and considering the interrupts
worst case arrival rate and cost allows the kernel to use a simple
rate-monotonic analysis techniques with the nanokernel. Using this
technique an application can meet its deadlines.

When an external event, handled by an ISR, marks a fiber runnable, the
scheduler inserts the fiber into the list of runnable fibers based on
its priority. The worst case delay after that point is the sum of the
maximum execution times between un-scheduling points of the earlier
runnable fibers of higher or equal priority.

The nanokernel provides three mechanisms to reduce the worst-case delay
for responding to an external event:


Moving Computation Processing to a Task
***************************************

Move the processing to a task to minimize the amount of computation that
is performed at the fiber level. This reduces the scheduling delay for
fibers because a task is preempted when an ISR makes a fiber that
handles the external event runnable.


Moving Code to Handle External Event to ISR
*******************************************

Move the code to handle the external event into an ISR. The ISR is
executed immediately after the event is recognized, without waiting for
the other fibers in the queue to be unscheduled.

Adding Yielding Points to Fibers
********************************

Add yielding points to fibers with :cpp:func:`fiber_yield()`. This service
un-schedules a fiber and places it at the end of the ready fiber list
of fibers with that priority. It allows other fibers at the same
priority to get to the head of the queue faster. If a fiber executes
code that will take some time, periodically call
:cpp:func:`fiber_yield()`. Multi-threading using blocking fibers is
effective in coding hard real-time applications.

Usage
*****

Example: Starting a Fiber from a Task
=====================================

This code shows how the currently executing task can start multiple fibers,
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

+-----------------------------------+-----------------------------------------+
| Call                              | Description                             |
+-----------------------------------+-----------------------------------------+
| :c:func:`fiber_yield()`           | Yields CPU to higher priority and       |
|                                   | equal priority fibers.                  |
+-----------------------------------+-----------------------------------------+
| :c:func:`fiber_sleep()`           | Yields CPU for a specified time period. |
+-----------------------------------+-----------------------------------------+
| :c:func:`fiber_abort()`           | Terminates fiber execution.             |
+-----------------------------------+-----------------------------------------+

The following APIs affecting a specified fiber are provided
by :file:`microkernel.h` and by :file:`nanokernel.h`:

+------------------------------------------------+----------------------------+
| Call                                           | Description                |
+------------------------------------------------+----------------------------+
| | :c:func:`task_fiber_start()`                 | Spawns a new fiber.        |
| | :c:func:`fiber_fiber_start()`                |                            |
| | :c:func:`fiber_start()`                      |                            |
+------------------------------------------------+----------------------------+
| | :c:func:`task_fiber_delayed_start()`         | Spawns a new fiber after   |
| | :c:func:`fiber_fiber_delayed_start()`        | a specified time period.   |
| | :c:func:`fiber_delayed_start()`              |                            |
+------------------------------------------------+----------------------------+
| | :c:func:`task_fiber_delayed_start_cancel()`  | Cancels spawning of a      |
| | :c:func:`fiber_fiber_delayed_start_cancel()` | new fiber, if not already  |
| | :c:func:`fiber_delayed_start_cancel()`       | started.                   |
+------------------------------------------------+----------------------------+
