Title: test_pipe

Description:

This test verifies that the microkernel pipe APIs operate as expected.

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make pristine
    make microkernel.qemu

--------------------------------------------------------------------------------

Sample Output:

Starting pipe tests
===================================================================
Testing task_pipe_put() ...
Testing task_pipe_put_wait() ...
Testing task_pipe_put_wait_timeout() ...
Testing task_pipe_get() ...
Testing task_pipe_get_wait() ...
Testing task_pipe_get_wait_timeout() ...
===================================================================
VXMICRO PROJECT EXECUTION SUCCESSFUL
