Title: Nanokernel Object Performance

Description:

The SysKernel test measures the performance of the nanokernel's semaphore,
lifo, fifo and stack objects.

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project outputs to the console. It can be built and executed
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

MODULE: Nanokernel API test
KERNEL VERSION: <varies>

Each test below are repeated 5000 times and the average
time for one iteration is displayed.

TEST CASE: Semaphore #1
TEST COVERAGE:
	nano_sem_init
	nano_fiber_sem_take(TICKS_UNLIMITED)
	nano_fiber_sem_give
Starting test. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNN nSec
END TEST CASE

TEST CASE: Semaphore #2
TEST COVERAGE:
	nano_sem_init
	nano_fiber_sem_take(TICKS_NONE)
	fiber_yield
	nano_fiber_sem_give
Starting test. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNN nSec
END TEST CASE

TEST CASE: Semaphore #3
TEST COVERAGE:
	nano_sem_init
	nano_fiber_sem_take(TICKS_UNLIMITED)
	nano_fiber_sem_give
	nano_task_sem_give
	nano_task_sem_take(TICKS_UNLIMITED)
Starting test. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNN nSec
END TEST CASE

TEST CASE: LIFO #1
TEST COVERAGE:
	nano_lifo_init
	nano_fiber_lifo_get(TICKS_UNLIMITED)
	nano_fiber_lifo_put
Starting test. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNN nSec
END TEST CASE

TEST CASE: LIFO #2
TEST COVERAGE:
	nano_lifo_init
	nano_fiber_lifo_get(TICKS_UNLIMITED)
	nano_fiber_lifo_get(TICKS_NONE)
	nano_fiber_lifo_put
	fiber_yield
Starting test. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNN nSec
END TEST CASE

TEST CASE: LIFO #3
TEST COVERAGE:
	nano_lifo_init
	nano_fiber_lifo_get(TICKS_UNLIMITED)
	nano_fiber_lifo_put
	nano_task_lifo_get(TICKS_UNLIMITED)
	nano_task_lifo_put
Starting test. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNN nSec
END TEST CASE

TEST CASE: FIFO #1
TEST COVERAGE:
	nano_fifo_init
	nano_fiber_fifo_get
	nano_fiber_fifo_put
Starting test. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNN nSec
END TEST CASE

TEST CASE: FIFO #2
TEST COVERAGE:
	nano_fifo_init
	nano_fiber_fifo_get(TICKS_UNLIMITED)
	nano_fiber_fifo_get
	nano_fiber_fifo_put
	fiber_yield
Starting test. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNN nSec
END TEST CASE

TEST CASE: FIFO #3
TEST COVERAGE:
	nano_fifo_init
	nano_fiber_fifo_get
	nano_fiber_fifo_put
	nano_task_fifo_get
	nano_task_fifo_put
Starting test. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNN nSec
END TEST CASE

TEST CASE: Stack #1
TEST COVERAGE:
	nano_stack_init
	nano_fiber_stack_pop(TICKS_UNLIMITED)
	nano_fiber_stack_push
Starting test. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNN nSec
END TEST CASE

TEST CASE: Stack #2
TEST COVERAGE:
	nano_stack_init
	nano_fiber_stack_pop(TICKS_UNLIMITED)
	nano_fiber_stack_pop
	nano_fiber_stack_push
	fiber_yield
Starting test. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNN nSec
END TEST CASE

TEST CASE: Stack #3
TEST COVERAGE:
	nano_stack_init
	nano_fiber_stack_pop(TICKS_UNLIMITED)
	nano_fiber_stack_push
	nano_task_stack_pop(TICKS_UNLIMITED)
	nano_task_stack_push
Starting test. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNN nSec
END TEST CASE

PROJECT EXECUTION SUCCESSFUL
