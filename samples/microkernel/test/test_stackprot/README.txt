Title: test_stackprot

Description:

This test verifies that stack canaries operate as expected in the microkernel.

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make pristine
    make microkernel.qemu

If executing on Simics, substitute 'simics' for 'qemu' in the command line.

--------------------------------------------------------------------------------

Sample Output:

tc_start() - Test Stack Protection Canary

Starts RegressionTask
Starts AlternateTask
AlternateTask: Input string is too long and stack overflowed!

***** Stack Check Fail! *****
Current context ID = 0x00102804
Faulting instruction address = 0xdeaddead
Fatal task error! Aborting task.
RegressionTask: Stack ok
RegressionTask: Stack ok
RegressionTask: Stack ok
RegressionTask: Stack ok
RegressionTask: Stack ok
RegressionTask: Stack ok
===================================================================
PASS - RegressionTask.
===================================================================
VXMICRO PROJECT EXECUTION SUCCESSFUL
