Title: test_events

Description:

This test verifies that the microkernel event APIs operate as expected.

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make pristine
    make microkernel.qemu

If executing on Simics, substitute 'simics' for 'qemu' in the command line.

--------------------------------------------------------------------------------

Sample Output:

tc_start() - Test Microkernel Events

Microkernel objects initialized
Testing task_event_recv() and task_event_send() ...
Testing task_event_recv_wait() and task_event_send() ...
Testing task_event_recv_wait_timeout() and task_event_send() ...
Testing isr_event_send() ...
Testing fiber_event_send() ...
Testing task_event_set_handler() ...
===================================================================
PASS - RegressionTask.
===================================================================
VXMICRO PROJECT EXECUTION SUCCESSFUL
