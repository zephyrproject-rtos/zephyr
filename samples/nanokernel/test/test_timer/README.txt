Title: test_timer

Description:

This test verifies that the nanokernel timer APIs operate as expected.

---------------------------------------------------------------------------

Building and Running Project:

This nanokernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make pristine
    make nanokernel.qemu

If executing on Simics, substitute 'simics' for 'qemu' in the command line.

---------------------------------------------------------------------------

Sample Output:

tc_start() - Test Nanokernel Timer
Task testing basic timer functionality
  - test expected to take four seconds
Task testing timers expire in the correct order
  - test expected to take five or six seconds
Task testing the stopping of timers
  - test expected to take six seconds
Fiber testing basic timer functionality
  - test expected to take four seconds
Fiber testing timers expire in the correct order
  - test expected to take five or six seconds
Task testing the stopping of timers
  - test expected to take six seconds
Fiber to stop a timer that has a waiting fiber
Task to stop a timer that has a waiting fiber
PASS - main.
===================================================================
VXMICRO PROJECT EXECUTION SUCCESSFUL
