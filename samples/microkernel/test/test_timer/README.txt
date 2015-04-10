Title: Microkernel low resolution timer test suite

Description:

This test verifies the following low resolution microkernel timer APIs operate
as expected:
  task_timer_alloc (), task_timer_free()
  task_timer_start(), task_timer_restart(), task_timer_stop()
  task_node_tick_delta(), task_node_tick_get_32()

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make pristine
    make microkernel.qemu

If executing on Simics, substitute 'simics' for 'qemu' in the command line.

--------------------------------------------------------------------------------

Sample Output:

Starting timer tests
===================================================================
Test the allocation of timers.
Test the one shot feature of a timer.
Test that a timer does not start.
Test the periodic feature of a timer
Test the stopping of a timer
===================================================================
VXMICRO PROJECT EXECUTION SUCCESSFUL
