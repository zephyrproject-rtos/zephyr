Title: kernel access to standard libraries

Description:

This test verifies kernel access to VxMicro's standard libraries.
It is intended to catch issues in which a library is completely absent
or non-functional, and is NOT intended to be a comprehensive test suite
of all functionality provided by the libraries.

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make pristine
    make microkernel.qemu

--------------------------------------------------------------------------------

Sample Output:

Starting standard libraries tests
===================================================================
Validating access to supported libraries
Testing ctype.h library ...
Testing inttypes.h library ...
Testing iso646.h library ...
Testing limits.h library ...
Testing stdbool.h library ...
Testing stddef.h library ...
Testing stdint.h library ...
Testing string.h library ...
Validation complete
===================================================================
VXMICRO PROJECT EXECUTION SUCCESSFUL
