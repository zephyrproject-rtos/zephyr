Title: Context and IRQ APIs

Description:

This test verifies that the nanokernel CPU and context APIs operate as expected.

---------------------------------------------------------------------------

Building and Running Project:

This nanokernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make qemu

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

tc_start() - Test Nanokernel CPU and thread routines
Initializing nanokernel objects
Testing nano_cpu_idle()
Testing interrupt locking and unlocking
Testing irq_disable() and irq_enable()
Testing sys_thread_self_get() from an ISR and task
Testing sys_execution_context_type_get() from an ISR
Testing sys_execution_context_type_get() from a task
Spawning a fiber from a task
Fiber to test sys_thread_self_get() and sys_execution_context_type_get
Fiber to test fiber_yield()
Testing sys_thread_busy_wait()
 fiber busy waiting for 20000 usecs (2 ticks)
 fiber busy waiting completed
Testing fiber_sleep()
 fiber sleeping for 5 ticks
 fiber back from sleep
Testing fiber_delayed_start() without cancellation
 fiber (q order: 2, t/o: 50) is running
 got fiber (q order: 2, t/o: 50) as expected
 fiber (q order: 3, t/o: 75) is running
 got fiber (q order: 3, t/o: 75) as expected
 fiber (q order: 0, t/o: 100) is running
 got fiber (q order: 0, t/o: 100) as expected
 fiber (q order: 6, t/o: 125) is running
 got fiber (q order: 6, t/o: 125) as expected
 fiber (q order: 1, t/o: 150) is running
 got fiber (q order: 1, t/o: 150) as expected
 fiber (q order: 4, t/o: 175) is running
 got fiber (q order: 4, t/o: 175) as expected
 fiber (q order: 5, t/o: 200) is running
 got fiber (q order: 5, t/o: 200) as expected
Testing fiber_delayed_start() with cancellations
 cancelling [q order: 0, t/o: 100, t/o order: 0]
 fiber (q order: 3, t/o: 75) is running
 got (q order: 3, t/o: 75, t/o order 1074292) as expected
 fiber (q order: 0, t/o: 100) is running
 got (q order: 0, t/o: 100, t/o order 1074292) as expected
 cancelling [q order: 3, t/o: 75, t/o order: 3]
 cancelling [q order: 4, t/o: 175, t/o order: 4]
 fiber (q order: 4, t/o: 175) is running
 got (q order: 4, t/o: 175, t/o order 1074292) as expected
 cancelling [q order: 6, t/o: 125, t/o order: 6]
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL
