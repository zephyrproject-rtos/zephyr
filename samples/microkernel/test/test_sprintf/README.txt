Title: test_sprintf

Description:

This test verifies that sprintf() and its variants operate as expected.

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make pristine
    make microkernel.qemu

--------------------------------------------------------------------------------
Sample Output:

tc_start() - Test Microkernel sprintf APIs

===================================================================
Testing sprintf() with integers ....
Testing snprintf() ....
Testing vsprintf() ....
Testing vsnprintf() ....
Testing sprintf() with strings ....
Testing sprintf() with misc options ....
Testing sprintf() with doubles ....
===================================================================
PASS - RegressionTask.
===================================================================
VXMICRO PROJECT EXECUTION SUCCESSFUL
