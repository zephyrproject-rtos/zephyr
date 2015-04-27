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

    make pristine
    make microkernel.qemu

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
Pi calculation OK after 550 (high) + 9 (low) tests (computed 3.141594)
Load and store OK after 600 (high) + 123288 (low) tests
Pi calculation OK after 650 (high) + 11 (low) tests (computed 3.141594)
Load and store OK after 700 (high) + 153272 (low) tests
Pi calculation OK after 750 (high) + 14 (low) tests (computed 3.141594)
Load and store OK after 800 (high) + 175104 (low) tests
Pi calculation OK after 850 (high) + 16 (low) tests (computed 3.141594)
Load and store OK after 900 (high) + 193801 (low) tests
Pi calculation OK after 950 (high) + 17 (low) tests (computed 3.141594)
Load and store OK after 1000 (high) + 215252 (low) tests
===================================================================
VXMICRO PROJECT EXECUTION SUCCESSFUL
