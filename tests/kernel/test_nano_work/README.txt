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
Starting delayed test
 - Initializing delayed test items
 - Submitting delayed test items
 - Submitting delayed work 1 from task
 - Submitting delayed work 3 from task
 - Submitting delayed work 5 from task
 - Waiting for delayed work to finish
 - Submitting delayed work 2 from fiber
 - Submitting delayed work 4 from fiber
 - Submitting delayed work 6 from fiber
 - Running delayed test item 1
 - Running delayed test item 2
 - Running delayed test item 3
 - Running delayed test item 4
 - Running delayed test item 5
 - Running delayed test item 6
 - Checking results
Starting delayed resubmit test
 - Submitting delayed work
 - Waiting for work to finish
 - Resubmitting delayed work
 - Resubmitting delayed work
 - Resubmitting delayed work
 - Resubmitting delayed work
 - Resubmitting delayed work
 - Checking results
Starting delayed resubmit from fiber test
 - Resubmitting delayed work with 1 tick
 - Resubmitting delayed work with 1 tick
 - Resubmitting delayed work with 1 tick
 - Resubmitting delayed work with 1 tick
 - Resubmitting delayed work with 1 tick
 - Resubmitting delayed work with 1 tick
 - Running delayed test item 1
 - Waiting for work to finish
 - Checking results
Starting delayed cancel test
 - Cancel delayed work from task
 - Cancel delayed work from fiber
 - Waiting for work to finish
 - Checking results
===================================================================
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL
