Title: test_task

Description:

This test verifies that the microkernel task APIs operate as expected.

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make pristine
    make microkernel.qemu

If executing on Simics, substitute 'simics' for 'qemu' in the command line.

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
VXMICRO PROJECT EXECUTION SUCCESSFUL
