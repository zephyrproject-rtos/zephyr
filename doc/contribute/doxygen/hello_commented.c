/** @file
 @brief Hello World Demo

 A Hello World demo for the Nanokernel and the Microkernel.
 */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/** CONFIG_MICROKERNEL
 The microkernel hello world demo has two tasks that use semaphores
 and sleeps to take turns printing a greeting message at a
 controlled rate.*/

/** #else || CONFIG_NANOKERNEL
 * The nanokernel hello world demo has a task and a fiber that use
 * semaphores and timers to take turns printing a greeting message at
 * a controlled rate.
 */

/**
 * @def SLEEPTICKS (SLEEPTIME * sys_clock_ticks_per_sec / 1000)
 * @brief Compute equivalence in ticks.
 */
/**
 * @def SLEEPTIME
 * @brief Specify delay between greetings (in ms).
 */

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define PRINT           printf
#else
#include <misc/printk.h>
#define PRINT           printk
#endif

#ifdef CONFIG_MICROKERNEL

#include <zephyr.h>

#define SLEEPTIME  500
#define SLEEPTICKS (SLEEPTIME * sys_clock_ticks_per_sec / 1000)

/**
 * @brief A loop saying hello.
 *
 * @details
 * Actions:
 * 	 -# Ouputs "Hello World!".
 * 	 -# Waits, then lets another task run.

 @param taskname The task's identification string.
 @param mySem    The task's semaphore.
 @param otherSem The other task's semaphore.
 */
void helloLoop(const char *taskname, ksem_t mySem, ksem_t otherSem)
{
	while (1)
	{
		task_sem_take (mySem, TICKS_UNLIMITED);

		PRINT ("%s: Hello World!\n", taskname); /* Action 1 */

		task_sleep (SLEEPTICKS); /* Action 2 */
		task_sem_give (otherSem);
	}
}

/**
 * @brief Exchanges Hello messages with taskB.
 *
 * @details
 * Actions:
 *  -# taskA gives its own semaphore, thus it says hello right away.
 *  -# Calls function helloLoop, thus taskA exchanges hello messages with taskB.
 */
void taskA(void)
{
	task_sem_give (TASKASEM); /* Action 1 */

	helloLoop (__FUNCTION__, TASKASEM, TASKBSEM); /* Action 2 */
}

/**
 * @brief Exchanges Hello messages with taskA.
 *
 * Actions:
 *  -# Calls function helloLoop, thus taskB exchanges hello messages with taskA.
 */
void taskB(void)
{
	helloLoop (__FUNCTION__, TASKBSEM, TASKASEM); /* Action 1 */
}

#else

#include <nanokernel.h>
#include <nanokernel/cpu.h>

#define SLEEPTIME  500
#define SLEEPTICKS (SLEEPTIME * sys_clock_ticks_per_sec / 1000)

#define STACKSIZE 2000

/** Declares a stack for a fiber with a size of 2000.*/
char fiberStack[STACKSIZE];

/** Declares a nanokernel semaphore for a task. */
struct nano_sem nanoSemTask;

/** Declares a nanokernel semaphore for a fiber.*/
struct nano_sem nanoSemFiber;

/**
 * @brief Defines the turns taken by the tasks in the fiber.
 *
 * Actions:
 * -# Initializes semaphore.
 * -# Initializes timer.
 * -# Waits for task, then runs.
 * -# Outputs "Hello World!".
 * -# Waits, then yields to another task.
 */
void fiberEntry(void) {
	struct nano_timer timer;
	uint32_t data[2] = { 0, 0 };

	nano_sem_init(&nanoSemFiber); /* Action 1 */

	nano_timer_init(&timer, data); /* Action 2 */

	while (1) {

		nano_fiber_sem_take(&nanoSemFiber, TICKS_UNLIMITED); /* Action 3 */

		PRINT("%s: Hello World!\n", __FUNCTION__); /* Action 4 */

		nano_fiber_timer_start(&timer, SLEEPTICKS); /* Action 5 */
		nano_fiber_timer_test(&timer, TICKS_UNLIMITED);
		nano_fiber_sem_give(&nanoSemTask);
	}
}

/**
 * @brief Implements the Hello demo.
 *
 * Actions:
 * -# Outputs "hello".
 * -# Waits, then signals fiber's semaphore.
 * -# Waits on fiber to yield.
 */
void main(void) {
	struct nano_timer timer;
	uint32_t data[2] = { 0, 0 };

	task_fiber_start(&fiberStack[0], STACKSIZE, (nano_fiber_entry_t) fiberEntry,
			0, 0, 7, 0);

	nano_sem_init(&nanoSemTask);
	nano_timer_init(&timer, data);

	while (1) {

		PRINT("%s: Hello World!\n", __FUNCTION__); /* Action 1 */

		nano_task_timer_start(&timer, SLEEPTICKS); /* Action 2 */
		nano_task_timer_test(&timer, TICKS_UNLIMITED);
		nano_task_sem_give(&nanoSemFiber);

		nano_task_sem_take(&nanoSemTask, TICKS_UNLIMITED); /* Action 3 */
	}
}

#endif
