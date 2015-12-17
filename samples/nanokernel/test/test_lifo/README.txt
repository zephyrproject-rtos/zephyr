Title: LIFO APIs

Description:

This test verifies that the nanokernel LIFO APIs operate as expected.

---------------------------------------------------------------------------

Building and Running Project:

This nanokernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make qemu

---------------------------------------------------------------------------

Troubleshooting:

Problems caused by out-dated project information can be addressed by
issuing one of the following commands then rebuilding the project:

    make clean          # discard results of previous builds
                        # but keep existing configuration info
or
    make pristine       # discard results of previous builds
                        # and restore pre-defined configuration info

---------------------------------------------------------------------------

Sample Output:

tc_start() - Test Nanokernel LIFO
Nano objects initialized
Fiber waiting on an empty LIFO
Task waiting on an empty LIFO
Fiber to get LIFO items without waiting
Task to get LIFO items without waiting
ISR to get LIFO items without waiting
First pass
multiple-waiter fiber 0 receiving item...
multiple-waiter fiber 1 receiving item...
multiple-waiter fiber 2 receiving item...
multiple-waiter fiber 0 got correct item, giving semaphore
multiple-waiter fiber 1 got correct item, giving semaphore
multiple-waiter fiber 2 got correct item, giving semaphore
Task took multi-waiter reply semaphore 3 times, as expected.
Second pass
multiple-waiter fiber 0 receiving item...
multiple-waiter fiber 0 got correct item, giving semaphore
multiple-waiter fiber 1 receiving item...
multiple-waiter fiber 2 receiving item...
multiple-waiter fiber 1 got correct item, giving semaphore
multiple-waiter fiber 2 got correct item, giving semaphore
Task took multi-waiter reply semaphore 3 times, as expected.
test nano_task_lifo_get() with timeout > 0
nano_task_lifo_get() timed out as expected
nano_task_lifo_get() got lifo in time, as expected
testing timeouts of 5 fibers on same lifo
 got fiber (q order: 2, t/o: 10, lifo 20005ff0) as expected
 got fiber (q order: 3, t/o: 15, lifo 20005ff0) as expected
 got fiber (q order: 0, t/o: 20, lifo 20005ff0) as expected
 got fiber (q order: 4, t/o: 25, lifo 20005ff0) as expected
 got fiber (q order: 1, t/o: 30, lifo 20005ff0) as expected
testing timeouts of 9 fibers on different lifos
 got fiber (q order: 0, t/o: 10, lifo 20005ffc) as expected
 got fiber (q order: 5, t/o: 15, lifo 20005ff0) as expected
 got fiber (q order: 7, t/o: 20, lifo 20005ff0) as expected
 got fiber (q order: 1, t/o: 25, lifo 20005ff0) as expected
 got fiber (q order: 8, t/o: 30, lifo 20005ffc) as expected
 got fiber (q order: 2, t/o: 35, lifo 20005ff0) as expected
 got fiber (q order: 6, t/o: 40, lifo 20005ff0) as expected
 got fiber (q order: 4, t/o: 45, lifo 20005ffc) as expected
 got fiber (q order: 3, t/o: 50, lifo 20005ffc) as expected
testing 5 fibers timing out, but obtaining the data in time
(except the last one, which times out)
 got fiber (q order: 0, t/o: 20, lifo 20005ff0) as expected
 got fiber (q order: 1, t/o: 30, lifo 20005ff0) as expected
 got fiber (q order: 2, t/o: 10, lifo 20005ff0) as expected
 got fiber (q order: 3, t/o: 15, lifo 20005ff0) as expected
 got fiber (q order: 4, t/o: 25, lifo 20005ff0) as expected
===================================================================
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL
