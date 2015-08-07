.. _nanokernel_timers:

Timer Services
##############

Definition
**********

The timer objects is defined in :file:`kernel/nanokernel/nano_timer.c`
and implements digital counters that either increment or decrement at a
fixed frequency. Timers can be called from a task or fiber context.

Function
********

Only a fiber or task context can call timers. Timers can only be used in
a nanokernel if it is not part of a microkernel. Timers are optional in
nanokernel-only systems. The nanokernel timers are simple. The
:c:func:`nano_node_tick_delta()` routine is not reentrant and should
only be called from a single context, unless it is certain other
contexts are not using the elapsed timer.


APIs
****

+--------------------------------+----------------------------------------------------------------+
| Context                        | Interface                                                      |
+================================+================================================================+
| **Initialization**             | :c:func:`nano_timer_init()`                                    |
+--------------------------------+----------------------------------------------------------------+
| **Interrupt Service Routines** |                                                                |
+--------------------------------+----------------------------------------------------------------+
| **Fibers**                     | :c:func:`nano_fiber_timer_test()`,                             |
|                                | :c:func:`nano_fiber_timer_wait()`,                             |
|                                | :c:func:`nano_fiber_timer_start()`,                            |
|                                | :c:func:`nano_fiber_timer_stop()`                              |
+--------------------------------+----------------------------------------------------------------+
| **Tasks**                      | :c:func:`nano_task_timer_test()`,                              |
|                                | :c:func:`nano_task_timer_wait()`,                              |
|                                | :c:func:`nano_task_timer_start()`,                             |
|                                | :c:func:`nano_task_timer_stop()`                               |
+--------------------------------+----------------------------------------------------------------+