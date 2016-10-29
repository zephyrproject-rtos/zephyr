Title: Microkernel early sleep functionality test

Description:

Test verifies that a task_sleep() function can be used during the system
initialization, then it tests that when the k_server() starts, task_sleep()
call makes another task run.

For fibers, test that fiber_sleep() called during the system
initialization puts a fiber to sleep for the provided amount of ticks,
then check that fiber_sleep() called from a fiber running on the
fully functioning microkernel puts that fiber to sleep for the proiveded
amount of ticks.

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

tc_start() - Test early and regular task and fiber sleep functionality

Test fiber_sleep() call during the system initialization
Test task_sleep() call during the system initialization
- At SECONDARY level
- At NANOKERNEL level
- At MICROKERNEL level
- At APPLICATION level
Test task_sleep() call on a running system
Test fiber_sleep() call on a running system
===================================================================
PASS - RegressionTask.
===================================================================
PROJECT EXECUTION SUCCESSFUL
