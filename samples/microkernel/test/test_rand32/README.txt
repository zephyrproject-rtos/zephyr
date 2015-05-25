Title: Microkernel random number generator test suite

Description:

This test verifies the following random number APIs operate
as expected:
sys_rand32_get()

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make pristine
    make microkernel.qemu

--------------------------------------------------------------------------------

Sample Output:

Starting random number tests
===================================================================
Generating random numbers
Generated 10 values with expected randomness
===================================================================
PASS - RegressionTaskEntry.
===================================================================
VXMICRO PROJECT EXECUTION SUCCESSFUL
