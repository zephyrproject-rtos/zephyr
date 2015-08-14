Title: Stack APIs

Description:

This test verifies that the nanokernel stack APIs operate as expected.

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

tc_start() - Test Nanokernel STACK
Test Task STACK Push

TASK STACK Put Order:  100, 200, 300, 400,
===================================================================
Test Fiber STACK Pop

FIBER STACK Pop: count = 0, data is 400
FIBER STACK Pop: count = 1, data is 300
FIBER STACK Pop: count = 2, data is 200
FIBER STACK Pop: count = 3, data is 100
PASS - fiber1.
===================================================================
Test Fiber STACK Push

FIBER STACK Put Order:  400, 300, 200, 100,
===================================================================
Test Task STACK Pop
TASK STACK Pop: count = 0, data is 100
TASK STACK Pop: count = 1, data is 200
TASK STACK Pop: count = 2, data is 300
TASK STACK Pop: count = 3, data is 400
===================================================================
Test STACK Pop Wait Interfaces

TASK  STACK Push to queue2: 100
Test Fiber STACK Pop Wait Interfaces

FIBER STACK Pop from queue2: 100
FIBER STACK Push to queue1: 200
TASK STACK Pop from queue1: 200
TASK STACK Push to queue2: 300
FIBER STACK Pop from queue2: 300
FIBER STACK Push to queue1: 400
PASS - testFiberStackPopW.
===================================================================
Test ISR STACK (invoked from Fiber)

ISR STACK (running in fiber context) Pop from queue1: 400
ISR STACK (running in fiber context) Push to queue1:
  150,   250,   350,   450,
PASS - testIsrStackFromFiber.
PASS - fiber2.
PASS - testTaskStackPopW.
===================================================================
Test ISR STACK (invoked from Task)

  Pop from queue1: count = 0, data is 450
  Pop from queue1: count = 1, data is 350
  Pop from queue1: count = 2, data is 250
  Pop from queue1: count = 3, data is 150

Test ISR STACK (invoked from Task) - push 450 and pop back 450
PASS - testIsrStackFromTask.
===================================================================
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL
