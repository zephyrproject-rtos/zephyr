Title: test_pipe_priv

Description:

This test verifies that the microkernel pipe APIs operate as expected.
This also verifies the mechanism to define private pipe object and its usage.

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

Starting pipe tests
===================================================================
Testing task_pipe_put() ...
Testing task_pipe_put_wait() ...
Testing task_pipe_put_wait_timeout() ...
Testing task_pipe_get() ...
Testing task_pipe_get_wait() ...
Testing task_pipe_get_wait_timeout() ...
===================================================================
PROJECT EXECUTION SUCCESSFUL
