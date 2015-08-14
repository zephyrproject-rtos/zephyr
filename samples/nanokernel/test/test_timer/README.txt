Title: Timer APIs

Description:

This test verifies that the nanokernel timer APIs operate as expected.

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
PROJECT EXECUTION SUCCESSFUL
