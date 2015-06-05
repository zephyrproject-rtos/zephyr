Title: test_context

Description:

This test verifies that the nanokernel CPU and context APIs operate as expected.

---------------------------------------------------------------------------

Building and Running Project:

This nanokernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make nanokernel.qemu

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

tc_start() - Test Nanokernel CPU and context routines
Initializing nanokernel objects
Testing nano_cpu_idle()
Testing interrupt locking and unlocking
Testing inline interrupt locking and unlocking
Testing irq_disable() and irq_enable()
Testing context_self_get() from an ISR and task
Testing context_type_get() from an ISR
Testing context_type_get() from a task
Spawning a fiber from a task
Fiber to test context_self_get() and context_type_get
Fiber to test fiber_yield()
Verifying exception handler installed
excHandlerExecuted: 1
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL
