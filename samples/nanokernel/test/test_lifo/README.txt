Title: test_lifo

Description:

This test verifies that the nanokernel LIFO APIs operate as expected.

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

tc_start() - Test Nanokernel LIFO
Nano objects initialized
Fiber waiting on an empty LIFO
Task waiting on an empty LIFO
Fiber to get LIFO items without waiting
Task to get LIFO items without waiting
ISR to get LIFO items without waiting
First pass
multiple-waiter fiber 0 receiving item...
multiple-waiter fiber 1 receiving item...
multiple-waiter fiber 2 receiving item...
multiple-waiter fiber 0 got correct item, giving semaphore
multiple-waiter fiber 1 got correct item, giving semaphore
multiple-waiter fiber 2 got correct item, giving semaphore
Task took multi-waiter reply semaphore 3 times, as expected.
Second pass
multiple-waiter fiber 0 receiving item...
multiple-waiter fiber 0 got correct item, giving semaphore
multiple-waiter fiber 1 receiving item...
multiple-waiter fiber 2 receiving item...
multiple-waiter fiber 1 got correct item, giving semaphore
multiple-waiter fiber 2 got correct item, giving semaphore
Task took multi-waiter reply semaphore 3 times, as expected.
===================================================================
PASS - main.
===================================================================
VXMICRO PROJECT EXECUTION SUCCESSFUL
