Title: test_stackprot

Description:

This test verifies that stack canaries operate as expected in the nanokernel.

---------------------------------------------------------------------------

This nanokernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make pristine
    make nanokernel.qemu

If executing on Simics, substitute 'simics' for 'qemu' in the command line.

---------------------------------------------------------------------------

Sample Output:

tc_start() - Test Stack Protection Canary

Starts main
Starts fiber1
fiber1: Input string is too long and stack overflowed!

***** Stack Check Fail! *****
Current context ID = 0x00102628
Faulting instruction address = 0xdeaddead
Fatal fiber error! Aborting fiber.
main: Stack ok
main: Stack ok
main: Stack ok
main: Stack ok
main: Stack ok
main: Stack ok
===================================================================
PASS - main.
===================================================================
VXMICRO PROJECT EXECUTION SUCCESSFUL
