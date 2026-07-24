Title: Test to verify the __TZ_WRAP_FUNC() macro.

Description:

__TZ_WRAP_FUNC() is part of the nonsecure TrustZone API, but is itself
independent of TrustZone functionality, so it is tested here outside the
context of secure/nonsecure firmware. This test runs only on ARM Cortex-M
targets.

The tz_wrap_func suite wraps a function with preface/postface callbacks via
__TZ_WRAP_FUNC() and verifies that the wrapper calls the preface, the
wrapped function and the postface in order, passes the arguments through
unchanged, returns the wrapped function's return value, and restores the
MSP and PSP stack pointers.

---------------------------------------------------------------------------

Building and Running:

Build and run with twister, for example on an ARM Cortex-M QEMU platform:

    twister -p mps2/an385 -T tests/arch/arm/arm_tz_wrap_func

Or build and run a single platform directly with west:

    west build -b mps2/an385 tests/arch/arm/arm_tz_wrap_func
    west build -t run

---------------------------------------------------------------------------

Sample Output:

Running TESTSUITE tz_wrap_func
===================================================================
START - test_tz_wrap_func
 PASS - test_tz_wrap_func
===================================================================
TESTSUITE tz_wrap_func succeeded
