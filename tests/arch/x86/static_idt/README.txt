Title: Static IDT Support

Description:

This test verifies that the static IDT feature operates as expected.

--------------------------------------------------------------------------------

Building and Running Project:

This project outputs to the console.  It can be built and executed
on QEMU as follows:

    make run


Sample Output:

tc_start() - Starting static IDT tests
Testing to see if IDT has address of test stubs()
Testing to see interrupt handler executes properly
Testing to see exception handler executes properly
Testing to see spurious handler executes properly
- Expect to see unhandled interrupt/exception message
***** Unhandled interrupt vector *****
Current thread ID = 0x001028e0
Faulting segment:address = 0x8:0x1001c9
eax: 0xa, ebx: 0x0, ecx: 0x1018e0, edx: 0xa
esi: 0x0, edi: 0x0, ebp: 01030b4, esp: 0x1030b4
eflags: 0x202
Fatal fault in thread 0x001028e0! Aborting.
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL
