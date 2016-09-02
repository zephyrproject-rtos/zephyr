.. _system_threads_v2:

System Threads
##############

A :dfn:`system thread` is a thread that is spawned by the kernel itself
to perform essential work. An application can sometimes utilize a system
thread to perform work, rather than spawning an additional thread.

.. contents::
    :local:
    :depth: 2

Concepts
********

The kernel spawns the following system threads.

**Main thread**
    This thread performs kernel initialization, then calls the application's
    :cpp:func:`main()` function. If the application does not supply a
    :cpp:func:`main()` function, the main thread terminates once initialization
    is complete.

    By default, the main thread uses the highest configured preemptible thread
    priority (i.e. 0). If the kernel is not configured to support preemptible
    threads, the main thread uses the lowest configured cooperative thread
    priority (i.e. -1).

**Idle thread**
    This thread executes when there is no other work for the system to do.
    If possible, the idle thread activates the board's power management support
    to save power; otherwise, the idle thread simply performs a "do nothing"
    loop.

    The idle thread always uses the lowest configured thread priority.
    If this makes it a cooperative thread, the idle thread repeatedly
    yields the CPU to allow the application's other threads to run when
    they need to.

.. note::
    Additional system threads may also be spawned, depending on the kernel
    and board configuration options specified by the application.

Implementation
**************

Offloading Work to the System Workqueue
=======================================

NEED TO COME UP WITH A BETTER EXAMPLE, SUCH AS AN ISR HANDING OFF WORK
TO THE SYSTEM WORKQUEUE THREAD BECAUSE IT TAKES TOO LONG.

.. code-block:: c

    /* TBD */

Suggested Uses
**************

Use the main thread to perform thread-based processing in an application
that only requires a single thread, rather than defining an additional
application-specific thread.

Use the system workqueue to defer complex interrupt-related processing
from an ISR to a cooperative thread. This allows the interrupt-related
processing to be done promptly without compromising the system's ability
to respond to subsequent interrupts, and does not require the application
to define an additional thread to do the processing.

Configuration Options
*********************

Related configuration options:

* :option:`CONFIG_MAIN_THREAD_PRIORITY`
* :option:`CONFIG_MAIN_STACK_SIZE`

APIs
****

[Add workqueue APIs?]
