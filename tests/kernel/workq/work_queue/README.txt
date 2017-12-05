Title: Test workqeue APIs

Description:

A simple application verifying the workqueue API

--------------------------------------------------------------------------------

Building and Running Project:

This kernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make run

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


***** BOOTING ZEPHYR OS vxxxx - BUILD: xxxxx *****
Starting sequence test
 - Initializing test items
 - Submitting test items
 - Submitting work 1 from preempt thread
 - Running test item 1
 - Submitting work 2 from coop thread
 - Submitting work 3 from preempt thread
 - Submitting work 4 from coop thread
 - Running test item 2
 - Submitting work 5 from preempt thread
 - Submitting work 6 from coop thread
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
 - Submitting delayed work 1 from preempt thread
 - Submitting delayed work 3 from preempt thread
 - Submitting delayed work 5 from preempt thread
 - Waiting for delayed work to finish
 - Submitting delayed work 2 from coop thread
 - Submitting delayed work 4 from coop thread
 - Submitting delayed work 6 from coop thread
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
Starting delayed resubmit from coop thread test
 - Resubmitting delayed work with 1 ms
 - Resubmitting delayed work with 1 ms
 - Resubmitting delayed work with 1 ms
 - Resubmitting delayed work with 1 ms
 - Resubmitting delayed work with 1 ms
 - Resubmitting delayed work with 1 ms
 - Waiting for work to finish
 - Running delayed test item 1
 - Checking results
Starting delayed cancel test
 - Cancel delayed work from preempt thread
 - Cancel delayed work from coop thread
 - Cancel pending delayed work from coop thread
 - Waiting for work to finish
 - Checking results
===================================================================
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL
