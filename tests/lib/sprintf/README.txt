Title: sprintf() APIs

Description:

This test verifies that sprintf() and its variants operate as expected.
The sprintf suite exercises the integer, string, floating-point and
miscellaneous conversion specifiers, the snprintf()/vsnprintf()/vsprintf()
truncation and return-length behaviour, and the return values of the
printf()/fprintf()/putc()/fwrite() family.  Some cases are conditional on
the selected libc's floating-point support and on the console
configuration.

See the Doxygen comments on the individual test functions for per-case
details.

---------------------------------------------------------------------------

Building and Running:

Build and run with twister, for example on QEMU:

    twister -p qemu_x86 -T tests/lib/sprintf

Or build and run a single platform directly with west:

    west build -b qemu_x86 tests/lib/sprintf
    west build -t run

---------------------------------------------------------------------------

Sample Output:

Running TESTSUITE sprintf
===================================================================
START - test_sprintf_integer
 PASS - test_sprintf_integer
...
===================================================================
TESTSUITE sprintf succeeded
