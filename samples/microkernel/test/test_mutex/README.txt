Title: test_res

Description:

This test verifies that the microkernel mutex APIs operate as expected.

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make pristine
    make microkernel.qemu

If executing on Simics, substitute 'simics' for 'qemu' in the command line.

--------------------------------------------------------------------------------

Sample Output:

tc_start() - Test Microkernel Mutex API
===================================================================
Done LOCKING!  Current priority = 10
===================================================================
PASS - RegressionTask.
===================================================================
VXMICRO PROJECT EXECUTION SUCCESSFUL
