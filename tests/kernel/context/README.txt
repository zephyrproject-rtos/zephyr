Title: Context and IRQ APIs

Description:

This test verifies that the kernel CPU and context APIs operate as expected.


APIs tested in this test set
============================

k_thread_create
  - start a helper thread to help with k_yield() tests
  - start a thread to test thread related functionality

k_yield
  - Called by a higher priority thread when there is another thread
  - Called by an equal priority thread when there is another thread
  - Called by a lower priority thread when there is another thread

k_current_get
  - Called from an ISR (interrupted a task)
  - Called from an ISR (interrupted a thread)
  - Called from a task
  - Called from a thread

k_is_in_isr
  - Called from an ISR that interrupted a task
  - Called from an ISR that interrupted a thread
  - Called from a task
  - Called from a thread

k_cpu_idle
  - Tickless Kernel: CPU to be woken up by a kernel timer (k_timer)
  - Non-tickless kernel:
    CPU to be woken up by tick timer.  Thus, after each call, the tick count
    should have advanced by one tick.

irq_lock
  - 1. Count the number of calls to _tick_get_32() before a tick expires.
  - 2. Once determined, call _tick_get_32() many more times than that
       with interrupts locked.  Check that the tick count remains unchanged.

irq_unlock
  - Continuation irq_lock: unlock interrupts, loop and verify the tick
    count changes.

irq_offload
  - Used when triggering an ISR to perform ISR context work.

irq_enable
irq_disable
  - Use these routines to disable and enable timer interrupts so that they can
    be tested in the same way as irq_lock() and irq_unlock().

---------------------------------------------------------------------------

Building and Running Project:

This project outputs to the console.  It can be built and executed
on QEMU as follows:

    make run

---------------------------------------------------------------------------

Troubleshooting:

Problems caused by out-dated project information can be addressed by
issuing one of the following commands then rebuilding the project:

    make clean          # discard results of previous builds
                        # but keep existing configuration info
or
    make pristine       # discard results of previous builds
                        # and restore pre-defined configuration info

---------------------------------------------------------------------------

Sample Output:

tc_start() - Test kernel CPU and thread routines
Initializing kernel objects
Testing k_cpu_idle()
Testing interrupt locking and unlocking
Testing irq_disable() and irq_enable()
Testing some kernel context routines
Testing k_current_get() from an ISR and task
Testing k_is_in_isr() from an ISR
Testing k_is_in_isr() from a preemptible thread
Spawning a thread from a task
Thread to test k_current_get() and k_is_in_isr()
Thread to test k_yield()
Testing k_busy_wait()
Thread busy waiting for 20000 usecs
Thread busy waiting completed
Testing k_sleep()
 thread sleeping for 50 milliseconds
 thread back from sleep
Testing k_thread_create() without cancellation
 thread (q order: 2, t/o: 500) is running
 got thread (q order: 2, t/o: 500) as expected
 thread (q order: 3, t/o: 750) is running
 got thread (q order: 3, t/o: 750) as expected
 thread (q order: 0, t/o: 1000) is running
 got thread (q order: 0, t/o: 1000) as expected
 thread (q order: 6, t/o: 1250) is running
 got thread (q order: 6, t/o: 1250) as expected
 thread (q order: 1, t/o: 1500) is running
 got thread (q order: 1, t/o: 1500) as expected
 thread (q order: 4, t/o: 1750) is running
 got thread (q order: 4, t/o: 1750) as expected
 thread (q order: 5, t/o: 2000) is running
 got thread (q order: 5, t/o: 2000) as expected
Testing k_thread_create() with cancellations
 cancelling [q order: 0, t/o: 1000, t/o order: 0]
 thread (q order: 3, t/o: 750) is running
 got (q order: 3, t/o: 750, t/o order 1) as expected
 thread (q order: 0, t/o: 1000) is running
 got (q order: 0, t/o: 1000, t/o order 2) as expected
 cancelling [q order: 3, t/o: 750, t/o order: 3]
 cancelling [q order: 4, t/o: 1750, t/o order: 4]
 thread (q order: 4, t/o: 1750) is running
 got (q order: 4, t/o: 1750, t/o order 5) as expected
 cancelling [q order: 6, t/o: 1250, t/o order: 6]
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL
