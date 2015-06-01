Title: SysKernel

Description:

The SysKernel test measures the performance of the sema, lifo, fifo and stack
objects provided by the VxMicro nanokernel.

--------------------------------------------------------------------------------

Building and Running Project:

This nanokernel project outputs to the console. It can be built and executed
on QEMU as follows:

    make nanokernel.qemu

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

TEST CASE: Semaphore channel - 'nano_fiber_sem_take_wait'
DESCRIPTION: testing 'nano_sem_init','nano_fiber_sem_take_wait', 'nano_fiber_sem_give' functions;
Starting test 'nano_fiber_sem_take_wait'. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNNN nSec
END TEST CASE

TEST CASE: Semaphore channel - 'nano_fiber_sem_take'
DESCRIPTION: testing 'nano_sem_init','nano_fiber_sem_take', 'fiber_yield',
	'nano_fiber_sem_give' functions;
Starting test 'nano_fiber_sem_take'. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNNN nSec
END TEST CASE

TEST CASE: Semaphore channel - 'nano_task_sem_take_wait'
DESCRIPTION: testing 'nano_sem_init','nano_fiber_sem_take_wait', 'nano_fiber_sem_give',
	'nano_task_sem_give', 'nano_task_sem_take_wait' functions;
Starting test 'nano_task_sem_take_wait'. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNNN nSec
END TEST CASE

TEST CASE: LIFO channel - 'nano_fiber_lifo_get_wait'
DESCRIPTION: testing 'nano_lifo_init','nano_fiber_lifo_get_wait', 'nano_fiber_lifo_put' functions;
Starting test 'nano_fiber_lifo_get_wait'. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNNN nSec
END TEST CASE

TEST CASE: LIFO channel - 'nano_fiber_lifo_get'
DESCRIPTION: testing 'nano_lifo_init','nano_fiber_lifo_get_wait', 'nano_fiber_lifo_get',
	'nano_fiber_lifo_put', 'fiber_yield' functions;
Starting test 'nano_fiber_lifo_get'. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNNN nSec
END TEST CASE

TEST CASE: LIFO channel - 'nano_task_lifo_get_wait'
DESCRIPTION: testing 'nano_lifo_init','nano_fiber_lifo_get_wait', 'nano_fiber_lifo_put',
	'nano_task_lifo_get_wait', 'nano_task_lifo_put' functions;
Starting test 'nano_task_lifo_get_wait'. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNNN nSec
END TEST CASE

TEST CASE: FIFO channel - 'nano_fiber_fifo_get_wait'
DESCRIPTION: testing 'nano_fifo_init','nano_fiber_fifo_get_wait', 'nano_fiber_fifo_put' functions;
Starting test 'nano_fiber_fifo_get_wait'. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNNN nSec
END TEST CASE

TEST CASE: FIFO channel - 'nano_fiber_fifo_get'
DESCRIPTION: testing 'nano_fifo_init','nano_fiber_fifo_get_wait', 'nano_fiber_fifo_get',
	'nano_fiber_fifo_put', 'fiber_yield' functions;
Starting test 'nano_fiber_fifo_get'. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNNN nSec
END TEST CASE

TEST CASE: FIFO channel - 'nano_task_fifo_get_wait'
DESCRIPTION: testing 'nano_fifo_init','nano_fiber_fifo_get_wait', 'nano_fiber_fifo_put',
	'nano_task_fifo_get_wait', 'nano_task_fifo_put' functions;
Starting test 'nano_task_fifo_get_wait'. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNNN nSec
END TEST CASE

TEST CASE: Stack channel - 'nano_fiber_stack_pop_wait'
DESCRIPTION: testing 'nano_stack_init','nano_fiber_stack_pop_wait', 'nano_fiber_stack_push' functions;
Starting test 'nano_fiber_stack_pop_wait'. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNNN nSec
END TEST CASE

TEST CASE: Stack channel - 'nano_fiber_stack_pop'
DESCRIPTION: testing 'nano_stack_init','nano_fiber_stack_pop_wait', 'nano_fiber_stack_pop',
	'nano_fiber_stack_push', 'fiber_yield' functions;
Starting test 'nano_fiber_stack_pop'. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNNN nSec
END TEST CASE

TEST CASE: Stack channel - 'nano_task_stack_pop_wait'
DESCRIPTION: testing 'nano_stack_init','nano_fiber_stack_pop_wait', 'nano_fiber_stack_push',
	'nano_task_stack_pop_wait', 'nano_task_stack_push' functions;
Starting test 'nano_task_stack_pop_wait'. Please wait...
TEST RESULT: SUCCESSFUL
DETAILS: Average time for 1 iteration: NNNNN nSec
END TEST CASE

VXMICRO PROJECT EXECUTION SUCCESSFUL
