Title: Pipe APIs

Description:

This test verifies that the microkernel pipe APIs operate as expected.

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
Testing task_pipe_put(TICKS_NONE) ...
Testing task_pipe_put(TICKS_UNLIMITED) ...
Testing task_pipe_put(timeout) ...
Testing task_pipe_get(TICKS_NONE) ...
Testing task_pipe_get(TICKS_UNLIMITED) ...
Testing task_pipe_get(timeout) ...
===================================================================
PROJECT EXECUTION SUCCESSFUL
