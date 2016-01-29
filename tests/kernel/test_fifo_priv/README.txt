Title: Private FIFOs

Description:

This test verifies that the microkernel FIFO APIs operate as expected.
This also verifies the mechanism to define private FIFO and its usage.

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

tc_start() - Test Microkernel FIFO
myData[0] = 1,
myData[1] = 101,
myData[2] = 201,
myData[3] = 301,
myData[4] = 401,
===================================================================
PASS - fillFIFO.
verifyQueueData: i=0, successfully get data 1
verifyQueueData: i=1, successfully get data 101
verifyQueueData: i=2, FIFOQ is empty. No data.
===================================================================
PASS - verifyQueueData.
===================================================================
PASS - fillFIFO.
RegressionTask: About to putWT with data 401
RegressionTask: FIFO Put time out as expected for data 401
verifyQueueData: i=0, successfully get data 1
verifyQueueData: i=1, successfully get data 101
===================================================================
PASS - verifyQueueData.
===================================================================
PASS - fillFIFO.
RegressionTask: 2 element in queue
RegressionTask: Successfully purged queue
RegressionTask: confirm 0 element in queue
===================================================================
RegressionTask: About to GetW data
Starts MicroTestFifoTask
MicroTestFifoTask: Puts element 999
RegressionTask: GetW get back 999
MicroTestFifoTask: FIRegressionTask: GetWT timeout expected
===================================================================
PASS - fillFIFO.
RegressionTask: about to putW data 999
FOPut OK for 999
MicroTestFifoTask: About to purge queue
RegressionTask: PutW ok when queue is purged while waiting
===================================================================
PASS - fillFIFO.
RegressionTask: about to putW data 401
MicroTestFifoTask: Successfully purged queue
MicroTestFifoTask: About to dequeue 1 element
RegressionTask: PutW success for data 401
===================================================================
RegressionTask: Get back data 101
RegressionTask: Get back data 401
RegressionTask: queue is empty.  Test Done!
MicroTestFifoTask: task_fifo_get got back correct data 1
===================================================================
PASS - MicroTestFifoTask.
===================================================================
PASS - RegressionTask.
===================================================================
PROJECT EXECUTION SUCCESSFUL
