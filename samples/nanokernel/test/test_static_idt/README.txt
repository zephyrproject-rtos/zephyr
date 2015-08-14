Title: Static IDT Support

Description:

This test verifies that the static IDT feature operates as expected in a
nanokernel environment.

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

tc_start() - Test Nanokernel static IDT tests
Testing to see if IDT has address of test stubs()
Testing to see interrupt handler executes properly
Testing to see exception handler executes properly
Testing to see spurious handler executes properly
- Expect to see unhandled interrupt/exception message
***** Unhandled exception/interrupt occurred! *****
Current context ID = 0x00102a68
Faulting instruction address = 0x00102c50
Fatal fiber error! Aborting fiber.
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL
