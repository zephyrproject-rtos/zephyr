Title: kernel Object Performance

Description:

The SysKernel test measures the performance of semaphore,
lifo, fifo and stack objects.

--------------------------------------------------------------------------------

Building and Running Project:

This project outputs to the console. It can be built and executed
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

Sample Output:

MODULE: kernel API test
KERNEL VERSION: 0x1066300

Each test below is repeated 5000 times;
average time for one iteration is displayed.

TEST CASE: Semaphore #1
TEST COVERAGE:
        k_sem_init
        k_sem_take(K_FOREVER)
        k_sem_give
Starting test. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNN nSec
END TEST CASE

TEST CASE: Semaphore #2
TEST COVERAGE:
        k_sem_init
        k_sem_take(TICKS_NONE)
        k_yield
        k_sem_give
Starting test. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNN nSec
END TEST CASE

TEST CASE: Semaphore #3
TEST COVERAGE:
        k_sem_init
        k_sem_take(K_FOREVER)
        k_sem_give
        k_sem_give
        k_sem_take(K_FOREVER)
Starting test. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNN nSec
END TEST CASE

TEST CASE: LIFO #1
TEST COVERAGE:
        k_lifo_init
        k_lifo_get(K_FOREVER)
        k_lifo_put
Starting test. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNN nSec
END TEST CASE

TEST CASE: LIFO #2
TEST COVERAGE:
        k_lifo_init
        k_lifo_get(K_FOREVER)
        k_lifo_get(TICKS_NONE)
        k_lifo_put
        k_yield
Starting test. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNN nSec
END TEST CASE

TEST CASE: LIFO #3
TEST COVERAGE:
        k_lifo_init
        k_lifo_get(K_FOREVER)
        k_lifo_put
        k_lifo_get(K_FOREVER)
        k_lifo_put
Starting test. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNN nSec
END TEST CASE

TEST CASE: FIFO #1
TEST COVERAGE:
        k_fifo_init
        k_fifo_get(K_FOREVER)
        k_fifo_put
Starting test. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNN nSec
END TEST CASE

TEST CASE: FIFO #2
TEST COVERAGE:
        k_fifo_init
        k_fifo_get(K_FOREVER)
        k_fifo_get(TICKS_NONE)
        k_fifo_put
        k_yield
Starting test. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNN nSec
END TEST CASE

TEST CASE: FIFO #3
TEST COVERAGE:
        k_fifo_init
        k_fifo_get(K_FOREVER)
        k_fifo_put
        k_fifo_get(K_FOREVER)
        k_fifo_put
Starting test. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNN nSec
END TEST CASE

TEST CASE: Stack #1
TEST COVERAGE:
        k_stack_init
        k_stack_pop(K_FOREVER)
        k_stack_push
Starting test. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNN nSec
END TEST CASE

TEST CASE: Stack #2
TEST COVERAGE:
        k_stack_init
        k_stack_pop(K_FOREVER)
        k_stack_pop
        k_stack_push
        k_yield
Starting test. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNN nSec
END TEST CASE

TEST CASE: Stack #3
TEST COVERAGE:
        k_stack_init
        k_stack_pop(K_FOREVER)
        k_stack_push
        k_stack_pop(K_FOREVER)
        k_stack_push
Starting test. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNN nSec
END TEST CASE

PROJECT EXECUTION SUCCESSFUL
QEMU: Terminated

