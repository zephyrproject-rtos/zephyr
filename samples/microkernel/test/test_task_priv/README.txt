Title: Private Tasks

Description:

This test verifies that the microkernel task APIs operate as expected. This
also verifies the mechanism to define private task objects and their usage.

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

tc_start() - Test Microkernel Task API
===================================================================
Microkernel objects initialized
Testing isr_task_id_get() and isr_task_priority_get()
Testing task_id_get() and task_priority_get()
Testing task_priority_set()
Testing task_sleep()
Testing task_yield()
Testing task_suspend() and task_resume()
===================================================================
PASS - RegressionTask.
===================================================================
PROJECT EXECUTION SUCCESSFUL
