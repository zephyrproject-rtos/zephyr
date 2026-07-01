Title: Kernel Access to Dynamic Memory Allocation functions provided by
Standard Libraries

Description:

This test verifies kernel access to the dynamic memory allocation functions
provided by the standard C libraries supported in Zephyr (minimal libc,
newlib, newlib-nano and picolibc).  It is intended to catch issues in which a
library is completely absent or non-functional, and is NOT intended to be a
comprehensive test suite of all functionality provided by the libraries.

The c_lib_dynamic_memalloc suite exercises malloc(), calloc(), realloc(),
reallocarray() and free(), covering allocation, reuse, alignment,
data-preservation and overflow/failure handling.  The exact set of cases
depends on the selected libc and its heap configuration: when the common
libc malloc arena is present the allocating cases run, and when the arena
size is zero the "no memory" cases run instead.

See the Doxygen comments on the individual test functions for per-case
details.

---------------------------------------------------------------------------

Building and Running:

Build and run with twister, for example on QEMU:

    twister -p qemu_x86 -T tests/lib/mem_alloc

Or build and run a single platform directly with west:

    west build -b qemu_x86 tests/lib/mem_alloc
    west build -t run

---------------------------------------------------------------------------

Sample Output:

Running TESTSUITE c_lib_dynamic_memalloc
===================================================================
START - test_malloc
 PASS - test_malloc
...
===================================================================
TESTSUITE c_lib_dynamic_memalloc succeeded
