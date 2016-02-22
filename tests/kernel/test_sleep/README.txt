Title: Nanokernel Sleep and Wakeup APIs

Description:

This test verifies that the nanokernel sleep and wakeup APIs operate as
expected.

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

tc_start() - Test Nanokernel Sleep and Wakeup APIs

Nanokernel objects initialized
Test fiber started: id = 0x001030b8
Helper fiber started: id = 0x001028e8
Testing normal expiration of fiber_sleep()
Testing fiber_sleep() + fiber_fiber_wakeup()
Testing fiber_sleep() + isr_fiber_wakeup()
Testing fiber_sleep() + task_fiber_wakeup()
Testing nanokernel task_sleep()
===================================================================
PROJECT EXECUTION SUCCESSFUL
