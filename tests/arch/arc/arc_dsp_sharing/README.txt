Title: Shared DSP Support

Description:

This test is only for ARC targets and uses two tasks to:

  1) Test load and store dsp registers (including arch specific registers)
  2) compute complex vector product and check for any errors

This tests the ability of tasks to safely share dsp hardware
resources, even when switching occurs preemptively (note that both sets of
tests run concurrently even though they report their progress at different
times).

The demonstration utilizes semaphores, round robin scheduling, DSP and XY
memory support.

--------------------------------------------------------------------------------

Sample Output:

Running TESTSUITE dsp_sharing
===================================================================
START - test_load_store
Load and store OK after 0 (high) + 84 (low) tests
Load and store OK after 100 (high) + 11926 (low) tests
Load and store OK after 200 (high) + 23767 (low) tests
Load and store OK after 300 (high) + 35607 (low) tests
Load and store OK after 400 (high) + 47448 (low) tests
Load and store OK after 500 (high) + 59287 (low) tests
 PASS - test_load_store in 10.18 seconds
===================================================================
START - test_calculation
complex product calculation OK after 50 (high) + 63297 (low) tests (computed -160)
complex product calculation OK after 150 (high) + 188138 (low) tests (computed -160)
complex product calculation OK after 250 (high) + 312972 (low) tests (computed -160)
complex product calculation OK after 350 (high) + 437806 (low) tests (computed -160)
complex product calculation OK after 450 (high) + 562639 (low) tests (computed -160)
 PASS - test_calculation in 10.16 seconds
===================================================================
TESTSUITE dsp_sharing succeeded
===================================================================
PROJECT EXECUTION SUCCESSFUL
