Title: Kernel Access to Standard Libraries

Description:

This test verifies kernel access to the standard C libraries.
It is intended to catch issues in which a library is completely absent
or non-functional, and is NOT intended to be a comprehensive test suite
of all functionality provided by the libraries.

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make qemu

--------------------------------------------------------------------------------

Troubleshooting:

Problems caused by out-dated project information can be addressed by
issuing one of the following commands then rebuilding the project:

    make clean          # discard results of previous builds
                        # but keep existing configuration info
or
    make pristine       # discard results of previous builds
                        # and restore pre-defined configuration info

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
PROJECT EXECUTION SUCCESSFUL
