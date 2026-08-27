Title: Shared Floating Point Support

Description:

The Shared Floating Point Support test uses two threads to:

  1) load and store floating point registers and check for corruption
  2) independently compute pi and check for any errors

This tests the ability of threads to safely share floating point hardware
resources, even when switching occurs preemptively (both sets of tests run
concurrently even though they report their progress at different times).
The test utilizes semaphores, round robin scheduling, and floating point
support.

The fpu_sharing_generic suite runs two cases:

1. test_load_store
   - Repeatedly load and store the floating point registers and verify no
     corruption occurs across context switches.

2. test_pi
   - Independently compute pi in a separate thread and verify the result
     stays correct across context switches.

---------------------------------------------------------------------------

Building and Running:

Build and run with twister, for example on QEMU:

    twister -p qemu_x86 -T tests/kernel/fpu_sharing/generic

Or build and run a single platform directly with west (the x86 build uses
the dedicated prj_x86.conf):

    west build -b qemu_x86 tests/kernel/fpu_sharing/generic -- -DCONF_FILE=prj_x86.conf
    west build -t run

---------------------------------------------------------------------------

Advanced:

Depending upon the board's speed, the frequency of test output may range
from every few seconds to every few minutes. The speed of the test can be
controlled through PI_NUM_ITERATIONS (see the per-arch defaults in
testcase.yaml). Lowering this value increases the test's speed, at the
expense of the calculation's precision:

    west build -b qemu_x86 tests/kernel/fpu_sharing/generic \
        -- -DCONF_FILE=prj_x86.conf -DPI_NUM_ITERATIONS=100000

---------------------------------------------------------------------------

Sample Output:

Running TESTSUITE fpu_sharing_generic
===================================================================
START - test_load_store
Load and store OK after 0 (high) + 63 (low) tests
Load and store OK after 100 (high) + 6540 (low) tests
 PASS - test_load_store
===================================================================
START - test_pi
Pi calculation OK after 50 (high) + 10 (low) tests (computed 3.141598)
Pi calculation OK after 150 (high) + 31 (low) tests (computed 3.141598)
 PASS - test_pi
===================================================================
TESTSUITE fpu_sharing_generic succeeded
