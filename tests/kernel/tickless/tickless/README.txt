Title: Tickless Idle Support

Description:

This test verifies the timing accuracy of the tickless idle feature.

The test first calibrates itself by repeatedly sleeping for 10 ticks with the
tickless idle feature disabled.  It then repeats this process with the tickless
idle feature enabled.  Lastly, it compares the average measured duration of
each approach and displays the result.  The tick timing is correct if the
'diff ticks' with tickless enabled matches the SLEEP_TICKS (10) setting in
the source.

The demonstration utilizes microkernel mutex APIs, timers and tickless
idle mode.

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

Sample Output:

Tickless Idle Test
Calibrating TSC...
Calibrated time stamp period = 0x00000000163adc3a
Do the real test with tickless enabled
Going idle for 10 ticks...
start ticks     : 343
end   ticks     : 353
diff  ticks     : 10
diff  time stamp: 0x0000000018a69898
Cal   time stamp: 0x00000000163adc3a
variance in time stamp diff: 10 percent
===================================================================
PROJECT EXECUTION SUCCESSFUL
