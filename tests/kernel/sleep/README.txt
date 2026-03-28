Title: cooperative thread  Sleep and Wakeup APIs

Description:

This test verifies that cooperative  sleep and wakeup APIs operate as
expected.

---------------------------------------------------------------------------

Building and Running Project:

This project outputs to the console.  It can be built and executed
on QEMU as follows:

    make run

---------------------------------------------------------------------------

Troubleshooting:

Problems caused by out-dated project information can be addressed by
issuing one of the following commands then rebuilding the project:

    make clean          # discard results of previous builds
                        # but keep existing configuration info
or
    make pristine       # discard results of previous builds
                        # and restore pre-defined configuration info

---------------------------------------------------------------------------

Sample Output:

Running test suite sleep
===================================================================
starting test - test_sleep
Kernel objects initialized
Test thread started: id = 0x00400040
Helper thread started: id = 0x00400000
Testing normal expiration of k_sleep()
Testing: test thread sleep + helper thread wakeup test
Testing: test thread sleep + isr offload wakeup test
Testing: test thread sleep + main wakeup test thread
Testing kernel k_sleep()
PASS - test_sleep
===================================================================
starting test - test_usleep
elapsed_ms = 1000
PASS - test_usleep
===================================================================
Test suite sleep succeeded
===================================================================
PROJECT EXECUTION SUCCESSFUL
