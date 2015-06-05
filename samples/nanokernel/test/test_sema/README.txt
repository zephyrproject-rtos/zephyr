Title: test_sema

Description:

This test verifies that the nanokernel semaphore APIs operate as expected.

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

tc_start() - Test Nanokernel Semaphores
Nano objects initialized
Giving and taking a semaphore in a task (non-blocking)
Giving and taking a semaphore in an ISR (non-blocking)
Giving and taking a semaphore in a fiber (non-blocking)
Semaphore from the task woke the fiber
Semaphore from the fiber woke the task
Semaphore from the ISR woke the task.
First pass
multiple-waiter fiber 0 trying to get semaphore...
multiple-waiter fiber 1 trying to get semaphore...
multiple-waiter fiber 2 trying to get semaphore...
multiple-waiter fiber 0 acquired semaphore, sending reply
multiple-waiter fiber 1 acquired semaphore, sending reply
multiple-waiter fiber 2 acquired semaphore, sending reply
Task took multi-waiter reply semaphore 3 times, as expected.
Second pass
multiple-waiter fiber 0 trying to get semaphore...
multiple-waiter fiber 1 trying to get semaphore...
multiple-waiter fiber 2 trying to get semaphore...
multiple-waiter fiber 0 acquired semaphore, sending reply
multiple-waiter fiber 1 acquired semaphore, sending reply
multiple-waiter fiber 2 acquired semaphore, sending reply
Task took multi-waiter reply semaphore 3 times, as expected.
===================================================================
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL
