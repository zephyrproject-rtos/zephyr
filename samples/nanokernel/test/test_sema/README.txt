Title: test_sema

Description:

This test verifies that the nanokernel semaphore APIs operate as expected.

---------------------------------------------------------------------------

Building and Running Project:

This nanokernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make pristine
    make nanokernel.qemu

If executing on Simics, substitute 'simics' for 'qemu' in the command line.

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
PASS - main.
===================================================================
VXMICRO PROJECT EXECUTION SUCCESSFUL
