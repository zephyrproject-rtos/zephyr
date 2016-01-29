Title: Semaphore APIs

Description:

This test verifies that the nanokernel semaphore APIs operate as expected.

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

tc_start() - Test Nanokernel Semaphores
Nano objects initialized
Giving and taking a semaphore in a task (non-blocking)
Giving and taking a semaphore in an ISR (non-blocking)
Giving and taking a semaphore in a fiber (non-blocking)
Semaphore from the task woke the fiber
Semaphore from the fiber woke the task
Semaphore from the ISR woke the task.
First pass
multiple-waiter fiber 0 trying to get semaphore...
multiple-waiter fiber 1 trying to get semaphore...
multiple-waiter fiber 2 trying to get semaphore...
multiple-waiter fiber 0 acquired semaphore, sending reply
multiple-waiter fiber 1 acquired semaphore, sending reply
multiple-waiter fiber 2 acquired semaphore, sending reply
Task took multi-waiter reply semaphore 3 times, as expected.
Second pass
multiple-waiter fiber 0 trying to get semaphore...
multiple-waiter fiber 1 trying to get semaphore...
multiple-waiter fiber 2 trying to get semaphore...
multiple-waiter fiber 0 acquired semaphore, sending reply
multiple-waiter fiber 1 acquired semaphore, sending reply
multiple-waiter fiber 2 acquired semaphore, sending reply
Task took multi-waiter reply semaphore 3 times, as expected.
test nano_task_sem_take() with timeout > 0
nano_task_sem_take() timed out as expected
nano_task_sem_take() got sem in time, as expected
testing timeouts of 5 fibers on same sem
 got fiber (q order: 2, t/o: 10, sem: 200001c8) as expected
 got fiber (q order: 3, t/o: 15, sem: 200001c8) as expected
 got fiber (q order: 0, t/o: 20, sem: 200001c8) as expected
 got fiber (q order: 4, t/o: 25, sem: 200001c8) as expected
 got fiber (q order: 1, t/o: 30, sem: 200001c8) as expected
testing timeouts of 9 fibers on different sems
 got fiber (q order: 0, t/o: 10, sem: 200001d4) as expected
 got fiber (q order: 5, t/o: 15, sem: 200001c8) as expected
 got fiber (q order: 7, t/o: 20, sem: 200001c8) as expected
 got fiber (q order: 1, t/o: 25, sem: 200001c8) as expected
 got fiber (q order: 8, t/o: 30, sem: 200001d4) as expected
 got fiber (q order: 2, t/o: 35, sem: 200001c8) as expected
 got fiber (q order: 6, t/o: 40, sem: 200001c8) as expected
 got fiber (q order: 4, t/o: 45, sem: 200001d4) as expected
 got fiber (q order: 3, t/o: 50, sem: 200001d4) as expected
testing 5 fibers timing out, but obtaining the sem in time
(except the last one, which times out)
 got fiber (q order: 0, t/o: 20, sem: 200001c8) as expected
 got fiber (q order: 1, t/o: 30, sem: 200001c8) as expected
 got fiber (q order: 2, t/o: 10, sem: 200001c8) as expected
 got fiber (q order: 3, t/o: 15, sem: 200001c8) as expected
 got fiber (q order: 4, t/o: 25, sem: 200001c8) as expected
===================================================================
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL
