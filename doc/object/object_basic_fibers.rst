Fibers
######

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
:c:func:`fiber_fiber_start()` or :c:func:`task_fiber_start()` to create
and start a fiber. The function :c:func:`fiber_fiber_start()` creates
and starts a fiber from another fiber, while
:c:func:`task_fiber_start()` does so from a task. Both APIs use the
parameters *parameter1* and *parameter2* as *arg1* and *arg2* given to
the fiber . The full documentation on these APIs can be found in the
:ref:`code`.

When :c:func:`task_fiber_start()`is called from a task, the new fiber
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
fiber stack prior to calling :c:func:`fiber_fiber_start()`. A fiber
stack can be any sort of buffer. In this example the fiber stack is
defined as an array of 32-bit integers:

.. code-block::cpp

   int32_t process_stack[256];

The size of the fiber stack can be set freely. It is recommended to
start with a stack much larger than you think you need, say 1 KB for a
simple fiber, and then reduce it after testing the functionality of the
fiber to optimize memory usage. The number of local variables and of
function calls with large local variables determine the required stack
size.


Stopping a Fiber
****************

There are no APIs to stop or suspend a fiber. Only one API can influence
the scheduling of a fiber, :c:func:`fiber_yield()`. When a fiber yields
itself, the nanokernel checks for another runnable fiber of the same or
higher priority. If a fiber of the same priority or higher is found, a
context switch occurs. If no other fibers are ready to execute, or if
all the runnable fibers have a lower priority than the currently
running fiber, the nanokernel does not perform any scheduling allowing
the running fiber to continue. A task or an ISR cannot call
:c:func:`fiber_yield()`.

If a fiber executes lengthy computations that will introduce an
unacceptable delay in the scheduling of other fibers, it should yield
by placing a :c:func:`fiber_yield()` call within the loop of a
computational cannot call :c:func:`fiber_yield()`.

Scheduling Fibers
*****************

The fibers in the Zephyr OS are priority-scheduled. When several fibers
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
=======================================

Move the processing to a task to minimize the amount of computation that
is performed at the fiber level. This reduces the scheduling delay for
fibers because a task is preempted when an ISR makes a fiber that
handles the external event runnable.


Moving Code to Handle External Event to ISR
===========================================

Move the code to handle the external event into an ISR. The ISR is
executed immediately after the event is recognized, without waiting for
the other fibers in the queue to be unscheduled.

Adding Yielding Points to Fibers
================================

Add yielding points to fibers with :c:func:`fiber_yield()`. This service
un-schedules a fiber and places it at the end of the ready fiber list
of fibers with that priority. It allows other fibers at the same
priority to get to the head of the queue faster. If a fiber executes
code that will take some time, periodically call
:c:func:`fiber_yield()`. Multi-threading using blocking fibers is
effective in coding hard real-time applications.
