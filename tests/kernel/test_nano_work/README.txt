Title: Test nano_work

Description:

A simple application verifying the workqueue API

The first test submits a few work items from both fiber and task context and
checks that they are executed in order.

The second test checks that a work item can be resubmitted from its own handler.

--------------------------------------------------------------------------------

Building and Running Project:

This nanokernel project outputs to the console.  It can be built and executed
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

Starting sequence test
 - Initializing test items
 - Submitting test items
 - Submitting work 1 from task
 - Running test item 1
 - Submitting work 2 from fiber
 - Submitting work 3 from task
 - Submitting work 4 from fiber
 - Running test item 2
 - Submitting work 5 from task
 - Submitting work 6 from fiber
 - Waiting for work to finish
 - Running test item 3
 - Running test item 4
 - Running test item 5
 - Running test item 6
 - Checking results
Starting resubmit test
 - Submitting work
 - Waiting for work to finish
 - Resubmitting work
 - Resubmitting work
 - Resubmitting work
 - Resubmitting work
 - Resubmitting work
 - Checking results
===================================================================
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL
