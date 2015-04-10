Title: test_secure_string_api

Desription:

This test verifies that the microkernel secure string APIs operate as expected.

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make pristine
    make microkernel.qemu

If executing on Simics, substitute 'simics' for 'qemu' in the command line.

--------------------------------------------------------------------------------

Sample Output:

**** Invalid string operation! ****
Current context ID = 0x0010583c
Faulting instruction address = 0xdeaddead
Fatal task error! Aborting task.
**** Invalid string operation! ****
Current context ID = 0x0010563c
Faulting instruction address = 0xdeaddead
Fatal task error! Aborting task.
As expected, test task 1 did not continue operating
after calling memcpy_s with incorrect parameters
As expected, test task 2 did not continue operating
after calling strcpy_s with incorrect parameters
===================================================================
PASS - MainTask.
===================================================================
VXMICRO PROJECT EXECUTION SUCCESSFUL
