Title: Kernel Access to Standard Libraries

Description:

This test verifies kernel access to the standard C libraries.  It is
intended to catch issues in which a library is completely absent or
non-functional, and is NOT intended to be a comprehensive test suite of
all functionality provided by the libraries.

The libc_common suite covers implementation-defined types and constants
(<limits.h>, <stdbool.h>, <stddef.h>, <stdint.h>, <ctype.h>, ssize_t), the
string and memory routines (the str* / mem* family and strtok_r()),
searching, sorting and conversion (bsearch(), qsort()/qsort_r(), abs(),
atoi(), strtol()/strtoul(), tolower()/toupper()), math (sqrt()/sqrtf()),
time (gmtime(), asctime(), localtime(), ctime() and their reentrant
variants), random numbers (rand()/srand() and reproducibility), and process
control and allocation (abort(), exit(), malloc()).

See the Doxygen comments on the individual test functions for per-case
details.

---------------------------------------------------------------------------

Building and Running:

Build and run with twister, for example on QEMU:

    twister -p qemu_x86 -T tests/lib/c_lib/common

Or build and run a single platform directly with west:

    west build -b qemu_x86 tests/lib/c_lib/common
    west build -t run

---------------------------------------------------------------------------

Sample Output:

Running TESTSUITE libc_common
===================================================================
START - test_limits
 PASS - test_limits
...
===================================================================
TESTSUITE libc_common succeeded
