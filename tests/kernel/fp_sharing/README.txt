Title: Shared Floating Point Support

Description:

This test uses two tasks to independently compute pi, while two other tasks
load and store floating point registers and check for corruption. This tests
the ability of tasks to safely share floating point hardware resources, even
when switching occurs preemptively. (Note that both sets of tests run
concurrently even though they report their progress at different times.)

The demonstration utilizes mutex APIs, timers, semaphores,
round robin scheduling, and floating point support.

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

***** BOOTING ZEPHYR OS vxxx - BUILD: Jan xxxx *****
Floating point sharing tests started
===================================================================
Load and store OK after 100 (high) + 47119 (low) tests
Pi calculation OK after 50 (high) + 2 (low) tests (computed 3.141594)
Load and store OK after 200 (high) + 94186 (low) tests
Load and store OK after 300 (high) + 142416 (low) tests
Pi calculation OK after 150 (high) + 7 (low) tests (computed 3.141594)
Load and store OK after 400 (high) + 190736 (low) tests
Load and store OK after 500 (high) + 238618 (low) tests
===================================================================
PASS - load_store_high.
===================================================================
PROJECT EXECUTION SUCCESSFUL

