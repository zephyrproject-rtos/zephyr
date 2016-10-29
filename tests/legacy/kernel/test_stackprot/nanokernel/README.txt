Title: Stack Protection Support

Description:

This test verifies that stack canaries operate as expected in the nanokernel.

---------------------------------------------------------------------------

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
PROJECT EXECUTION SUCCESSFUL
