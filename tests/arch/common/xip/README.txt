Title: Execute in Place (XIP) Support

Description:

This test verifies that Execute in Place (XIP) works as expected. The
fact that the test image boots and runs is itself a good indication that
XIP is functioning; the xip suite additionally checks that global variable
initialization is correct, confirming that initialized data is available
when running from a XIP image.

---------------------------------------------------------------------------

Building and Running:

Build and run with twister, for example on QEMU:

    twister -p qemu_x86/atom/xip -T tests/arch/common/xip

Or build and run a single platform directly with west:

    west build -b qemu_x86/atom/xip tests/arch/common/xip
    west build -t run

---------------------------------------------------------------------------

Sample Output:

Running TESTSUITE xip
===================================================================
START - test_globals
 PASS - test_globals
===================================================================
TESTSUITE xip succeeded
