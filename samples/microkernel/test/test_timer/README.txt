Title: Low Resolution Timer

Description:

This test verifies the following low resolution microkernel timer APIs operate
as expected:
  task_timer_alloc (), task_timer_free()
  task_timer_start(), task_timer_restart(), task_timer_stop()
  sys_tick_delta(), sys_tick_get_32()

Also verifies the nanokernel timeouts can work alongside the microkernel timers.

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make qemu

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

Starting timer tests
===================================================================
Test the allocation of timers.
Test the one shot feature of a timer.
test nano_task_fifo_get with timeout > 0
nano_task_fifo_get timed out as expected
nano_task_fifo_get got fifo in time, as expected
testing timeouts of 5 fibers on same fifo
 got fiber (q order: 2, t/o: 10, fifo 00109134) as expected
 got fiber (q order: 3, t/o: 15, fifo 00109134) as expected
 got fiber (q order: 0, t/o: 20, fifo 00109134) as expected
 got fiber (q order: 4, t/o: 25, fifo 00109134) as expected
 got fiber (q order: 1, t/o: 30, fifo 00109134) as expected
testing timeouts of 9 fibers on different fifos
 got fiber (q order: 0, t/o: 10, fifo 00109140) as expected
 got fiber (q order: 5, t/o: 15, fifo 00109134) as expected
 got fiber (q order: 7, t/o: 20, fifo 00109134) as expected
 got fiber (q order: 1, t/o: 25, fifo 00109134) as expected
 got fiber (q order: 8, t/o: 30, fifo 00109140) as expected
 got fiber (q order: 2, t/o: 35, fifo 00109134) as expected
 got fiber (q order: 6, t/o: 40, fifo 00109134) as expected
 got fiber (q order: 4, t/o: 45, fifo 00109140) as expected
 got fiber (q order: 3, t/o: 50, fifo 00109140) as expected
testing 5 fibers timing out, but obtaining the data in time
(except the last one, which times out)
 got fiber (q order: 0, t/o: 20, fifo 00109134) as expected
 got fiber (q order: 1, t/o: 30, fifo 00109134) as expected
 got fiber (q order: 2, t/o: 10, fifo 00109134) as expected
 got fiber (q order: 3, t/o: 15, fifo 00109134) as expected
 got fiber (q order: 4, t/o: 25, fifo 00109134) as expected
===================================================================
PASS - test_fifo_timeout.
Test that a timer does not start.
Test the periodic feature of a timer
Test the stopping of a timer
Verifying the nanokernel timeouts worked
===================================================================
PROJECT EXECUTION SUCCESSFUL
