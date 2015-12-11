Title: FIFO APIs

Description:

This test verifies that the nanokernel FIFO APIs operate as expected.

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

tc_start() - Test Nanokernel FIFO
Test Task FIFO Put

TASK FIFO Put Order:  001056dc, 00104ed4, 001046c0, 00103e80,
===================================================================
Test Fiber FIFO Get

FIBER FIFO Get: count = 0, ptr is 001056dc
FIBER FIFO Get: count = 1, ptr is 00104ed4
FIBER FIFO Get: count = 2, ptr is 001046c0
FIBER FIFO Get: count = 3, ptr is 00103e80
PASS - fiber1.
===================================================================
Test Fiber FIFO Put

FIBER FIFO Put Order:  00103e80, 001046c0, 00104ed4, 001056dc,
===================================================================
Test Task FIFO Get
TASK FIFO Get: count = 0, ptr is 00103e80
TASK FIFO Get: count = 1, ptr is 001046c0
TASK FIFO Get: count = 2, ptr is 00104ed4
TASK FIFO Get: count = 3, ptr is 001056dc
===================================================================
Test Task FIFO Get Wait Interfaces

TASK FIFO Put to queue2: 001056dc
Test Fiber FIFO Get Wait Interfaces

FIBER FIFO Get from queue2: 001056dc
FIBER FIFO Put to queue1: 00104ed4
TASK FIFO Get from queue1: 00104ed4
TASK FIFO Put to queue2: 001046c0
FIBER FIFO Get from queue2: 001046c0
FIBER FIFO Put to queue1: 00103e80
PASS - testFiberFifoGetW.
===================================================================
Test ISR FIFO (invoked from Fiber)

ISR FIFO Get from queue1: 00103e80

ISR FIFO (running in fiber context) Put Order:
 001056dc, 00104ed4, 001046c0, 00103e80,
PASS - testIsrFifoFromFiber.
PASS - fiber2.
PASS - testTaskFifoGetW.
===================================================================
Test ISR FIFO (invoked from Task)

Get from queue1: count = 0, ptr is 001056dc
Get from queue1: count = 1, ptr is 00104ed4
Get from queue1: count = 2, ptr is 001046c0
Get from queue1: count = 3, ptr is 00103e80

Test ISR FIFO (invoked from Task) - put 001056dc and get back 001056dc
PASS - testIsrFifoFromTask.
===================================================================
test nano_task_fifo_get with timeout > 0
nano_task_fifo_get timed out as expected
nano_task_fifo_get got fifo in time, as expected
testing timeouts of 5 fibers on same fifo
 got fiber (q order: 2, t/o: 10, fifo 200049c0) as expected
 got fiber (q order: 3, t/o: 15, fifo 200049c0) as expected
 got fiber (q order: 0, t/o: 20, fifo 200049c0) as expected
 got fiber (q order: 4, t/o: 25, fifo 200049c0) as expected
 got fiber (q order: 1, t/o: 30, fifo 200049c0) as expected
testing timeouts of 9 fibers on different fifos
 got fiber (q order: 0, t/o: 10, fifo 200049cc) as expected
 got fiber (q order: 5, t/o: 15, fifo 200049c0) as expected
 got fiber (q order: 7, t/o: 20, fifo 200049c0) as expected
 got fiber (q order: 1, t/o: 25, fifo 200049c0) as expected
 got fiber (q order: 8, t/o: 30, fifo 200049cc) as expected
 got fiber (q order: 2, t/o: 35, fifo 200049c0) as expected
 got fiber (q order: 6, t/o: 40, fifo 200049c0) as expected
 got fiber (q order: 4, t/o: 45, fifo 200049cc) as expected
 got fiber (q order: 3, t/o: 50, fifo 200049cc) as expected
testing 5 fibers timing out, but obtaining the data in time
(except the last one, which times out)
 got fiber (q order: 0, t/o: 20, fifo 200049c0) as expected
 got fiber (q order: 1, t/o: 30, fifo 200049c0) as expected
 got fiber (q order: 2, t/o: 10, fifo 200049c0) as expected
 got fiber (q order: 3, t/o: 15, fifo 200049c0) as expected
 got fiber (q order: 4, t/o: 25, fifo 200049c0) as expected
===================================================================
PASS - test_timeout.
===================================================================
===================================================================
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL
