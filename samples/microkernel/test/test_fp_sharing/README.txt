Title: test_fp_sharing

Description:

This test uses two tasks to independently compute pi, while two other tasks
load and store floating point registers and check for corruption. This tests
the ability of tasks to safely share floating point hardware resources, even
when switching occurs preemptively. (Note that both sets of tests run
concurrently even though they report their progress at different times.)

The demonstration utilizes microkernel resource APIs, timers, semaphores,
round robin scheduling, and floating point support.

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make microkernel.qemu

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

Sample Output:

Floating point sharing tests started
===================================================================
Pi calculation OK after 50 (high) + 1 (low) tests (computed 3.141594)
Load and store OK after 100 (high) + 29695 (low) tests
Pi calculation OK after 150 (high) + 2 (low) tests (computed 3.141594)
Load and store OK after 200 (high) + 47593 (low) tests
Pi calculation OK after 250 (high) + 4 (low) tests (computed 3.141594)
Load and store OK after 300 (high) + 66674 (low) tests
Pi calculation OK after 350 (high) + 5 (low) tests (computed 3.141594)
Load and store OK after 400 (high) + 83352 (low) tests
Pi calculation OK after 450 (high) + 7 (low) tests (computed 3.141594)
Load and store OK after 500 (high) + 92290 (low) tests
===================================================================
VXMICRO PROJECT EXECUTION SUCCESSFUL
