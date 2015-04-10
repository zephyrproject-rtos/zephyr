Title: test_fifo

Description:

This test verifies that the nanokernel FIFO APIs operate as expected.

---------------------------------------------------------------------------

Building and Running Project:

This nanokernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make pristine
    make nanokernel.qemu

If executing on Simics, substitute 'simics' for 'qemu' in the command line.

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
PASS - main.
===================================================================
VXMICRO PROJECT EXECUTION SUCCESSFUL
