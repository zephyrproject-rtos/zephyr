Title: test_sema

Description:

This test verifies that the microkernel semaphore APIs operate as expected.

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make pristine
    make microkernel.qemu

--------------------------------------------------------------------------------

Sample Output:

Starting semaphore tests
===================================================================
Signal and test a semaphore without blocking
Signal and test a semaphore with blocking
Testing many tasks blocked on the same semaphore
Testing semaphore groups without blocking
Testing semaphore groups with blocking
Testing semaphore release by fiber
===================================================================
VXMICRO PROJECT EXECUTION SUCCESSFUL
