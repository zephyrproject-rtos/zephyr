Title: test_static_idt

Description:

This test verifies that the static IDT feature operates as expected.

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make pristine
    make microkernel.qemu

--------------------------------------------------------------------------------

Sample Output:

tc_start() - Test Nanokernel static IDT tests
Testing to see if IDT has address of test stubs()
Testing to see interrupt handler executes properly
Testing to see exception handler executes properly
Testing to see spurious handler executes properly
- Expect to see unhandled interrupt/exception message
***** Unhandled exception/interrupt occurred! *****
Current context ID = 0x00102c44
Faulting instruction address = 0x0010342c
Fatal task error! Aborting task.
PASS - idtTestTask.
===================================================================
VXMICRO PROJECT EXECUTION SUCCESSFUL
