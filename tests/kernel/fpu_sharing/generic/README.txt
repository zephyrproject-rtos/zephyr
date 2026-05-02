Title: Shared Floating Point Support

Description:

The Shared Floating Point Support test uses two tasks to:

  1) load and store floating point registers and check for corruption
  2) independently compute pi and check for any errors

This tests the ability of tasks to safely share floating point hardware
resources, even when switching occurs preemptively (note that both sets of
tests run concurrently even though they report their progress at different
times).

The demonstration utilizes semaphores, round robin scheduling, and floating
point support.

--------------------------------------------------------------------------------

Building and Running Project:

This project outputs to the console.  It can be built and executed
on QEMU as follows:

    make run

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

Advanced:

Depending upon the board's speed, the frequency of test output may range from
every few seconds to every few minutes. The speed of the test can be controlled
through the variable PI_NUM_ITERATIONS (default 700000). Lowering this value
will increase the test's speed, but at the expense of the calculation's
precision.

    make run PI_NUM_ITERATIONS=100000

--------------------------------------------------------------------------------

Sample Output:

*** Booting Zephyr OS build zephyr-v2.2.0-845-g8b769de30317  ***
Running test suite fpu_sharing
===================================================================
starting test - test_load_store
Load and store OK after 0 (high) + 63 (low) tests
Load and store OK after 100 (high) + 6540 (low) tests
Load and store OK after 200 (high) + 12965 (low) tests
Load and store OK after 300 (high) + 19366 (low) tests
Load and store OK after 400 (high) + 25756 (low) tests
Load and store OK after 500 (high) + 32128 (low) tests
PASS - test_load_store
===================================================================
starting test - test_pi
Pi calculation OK after 50 (high) + 10 (low) tests (computed 3.141598)
Pi calculation OK after 150 (high) + 31 (low) tests (computed 3.141598)
Pi calculation OK after 250 (high) + 51 (low) tests (computed 3.141598)
Pi calculation OK after 350 (high) + 72 (low) tests (computed 3.141598)
Pi calculation OK after 450 (high) + 92 (low) tests (computed 3.141598)
PASS - test_pi
===================================================================
Test suite fpu_sharing succeeded
===================================================================
PROJECT EXECUTION SUCCESSFUL
