Title: Test to verify the __TZ_WRAP_FUNC() macro.

Description:
__TZ_WRAP_FUNC() is part of the nonsecure TrustZone API, but is itself
independent of TrustZone functionality, so it is tested here outside the context
of secure/nonsecure firmware.

The test verifies that:
 - The wrapper functions are correctly called.
 - The arguments are passed to the wrapped function.
 - The return value from the wrapped function is correctly returned from the
   wrapper function.

---------------------------------------------------------------------------

Building and Running Project:

This project outputs to the console.  It can be built and executed on QEMU as
follows:

    ninja/make run

---------------------------------------------------------------------------

Troubleshooting:

Problems caused by out-dated project information can be addressed by
issuing one of the following commands then rebuilding the project:

    ninja/make clean    # discard results of previous builds
                        # but keep existing configuration info
or
    ninja/make pristine # discard results of previous builds
                        # and restore pre-defined configuration info

---------------------------------------------------------------------------

Sample Output:

*** Booting Zephyr OS build zephyr-v2.3.0-2427-g6a7e2dc314b2  ***
Running test suite tz_wrap_func
===================================================================
START - test_tz_wrap_func
 PASS - test_tz_wrap_func
===================================================================
Test suite tz_wrap_func succeeded
===================================================================
PROJECT EXECUTION SUCCESSFUL
