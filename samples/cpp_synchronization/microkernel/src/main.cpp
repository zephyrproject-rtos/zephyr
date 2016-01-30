/*
 * Copyright (c) 2015-2016 Wind River Systems, Inc.
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

/**
 * @file C++ Synchronization demo.  Uses basic C++ functionality.
 */

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define PRINT           printf
#else
#include <misc/printk.h>
#define PRINT           printk
#endif

/**
 * @class semaphore the basic pure virtual semaphore class
 */
class semaphore {
public:
	virtual int wait(void) = 0;
	virtual int wait(int timeout) = 0;
	virtual void give(void) = 0;
};

/* specify delay between greetings (in ms); compute equivalent in ticks */

#define SLEEPTIME  500
#define SLEEPTICKS (SLEEPTIME * sys_clock_ticks_per_sec / 1000)

#ifdef CONFIG_MICROKERNEL

/*
 * Microkernel version of C++ synchronization demo has two tasks that utilize
 * semaphores and sleeps to take turns printing a greeting message at
 * a controlled rate.
 */

#include <zephyr.h>

/*
 * @class task_semaphore
 * @brief miscrokernel task semaphore
 *
 * Class derives from the pure virtual semaphore class and
 * implements it's methods for the microkernel semaphore
 */
class task_semaphore: public semaphore {
protected:
	struct _k_sem_struct _sema_internal;
	ksem_t sema;
public:
	task_semaphore();
	virtual ~task_semaphore() {}
	virtual int wait(void);
	virtual int wait(int timeout);
	virtual void give(void);
};

/*
 * @brief task_semaphore basic constructor
 */
task_semaphore::task_semaphore(): _sema_internal(__K_SEMAPHORE_DEFAULT)
{
	PRINT("Create semaphore %p\n", this);
	sema = (ksem_t)&_sema_internal;
}

/*
 * @brief wait for a semaphore
 *
 * Test a semaphore to see if it has been signaled.  If the signal
 * count is greater than zero, it is decremented.
 *
 * @return RC_OK on success, RC_FAIL on error
 */
int task_semaphore::wait(void)
{
	return task_sem_take(sema, TICKS_UNLIMITED);
}

/*
 * @brief wait for a semaphore within a specified timeout
 * Test a semaphore to see if it has been signaled.  If the signal
 * count is greater than zero, it is decremented. The function
 * waits for the specified timeout
 *
 * @param timeout the specified timeout in ticks
 *
 * @return RC_OK on success, RC_FAIL on error or RC_TIME on timeout
 */
int task_semaphore::wait(int timeout)
{
	return task_sem_take(sema, timeout);
}

/**
 *
 * @brief Signal a semaphore
 *
 * This routine signals the specified semaphore.
 *
 * @return N/A
 */
void task_semaphore::give(void)
{
	task_sem_give(sema);
}

/*
 *
 * @param taskname    task identification string
 * @param mySem       task's own semaphore
 * @param otherSem    other task's semaphore
 *
 */
void hello_loop(const char *taskname,
	       task_semaphore& my_sem,
	       task_semaphore& other_sem)
{
	while (1) {
		my_sem.wait();

		/* say "hello" */
		PRINT("%s: Hello World!\n", taskname);

		/* wait a while, then let other task have a turn */
		task_sleep(SLEEPTICKS);
		other_sem.give();
	}
}

/* two tasks synchronization semaphores */
task_semaphore sem_a;
task_semaphore sem_b;

extern "C" void task_a(void)
{
	/* taskA gives its own semaphore, allowing it to say hello right away */
	sem_a.give();

	/* invoke routine that allows task to ping-pong hello messages with taskB */
	hello_loop(__FUNCTION__, sem_a, sem_b);
}

extern "C" void task_b(void)
{
	/* invoke routine that allows task to ping-pong hello messages with taskA */
	hello_loop(__FUNCTION__, sem_b, sem_a);
}

#else /*  CONFIG_NANOKERNEL */

/*
 * Nanokernel version of C++ synchronization demo has a task and a fiber that
 * utilize semaphores and timers to take turns printing a greeting message at
 * a controlled rate.
 */

#include <nanokernel.h>
#include <arch/cpu.h>

#define STACKSIZE 2000

char __stack fiber_stack[STACKSIZE];

/*
 * @class nano_semaphore
 * @brief nano semaphore
 *
 * Class derives from the pure virtual semaphore class and
 * implements it's methods for the nanokernel semaphore
 */
class nano_semaphore: public semaphore {
protected:
	struct nano_sem _sema_internal;
public:
	nano_semaphore();
	virtual ~nano_semaphore() {}
	virtual int wait(void);
	virtual int wait(int timeout);
	virtual void give(void);
};

/*
 * @brief nano_semaphore basic constructor
 */
nano_semaphore::nano_semaphore()
{
	PRINT("Create semaphore %p\n", this);
	nano_sem_init(&_sema_internal);
}

/*
 * @brief wait for a semaphore
 *
 * Test a semaphore to see if it has been signaled.  If the signal
 * count is greater than zero, it is decremented.
 *
 * @return 1 when semaphore is available
 */
int nano_semaphore::wait(void)
{
	nano_sem_take(&_sema_internal, TICKS_UNLIMITED);
	return 1;
}

/*
 * @brief wait for a semaphore within a specified timeout
 *
 * Test a semaphore to see if it has been signaled.  If the signal
 * count is greater than zero, it is decremented. The function
 * waits for timeout specified
 *
 * @param timeout the specified timeout in ticks
 *
 * @return 1 if semaphore is available, 0 if timed out
 */
int nano_semaphore::wait(int timeout)
{
	return nano_sem_take(&_sema_internal, timeout);
}

/**
 *
 * @brief Signal a semaphore
 *
 * This routine signals the specified semaphore.
 *
 * @return N/A
 */
void nano_semaphore::give(void)
{
	nano_sem_give(&_sema_internal);
}

/* task and fiber synchronization semaphores */
nano_semaphore nano_sem_fiber;
nano_semaphore nano_sem_task;

void fiber_entry(void)
{
	struct nano_timer timer;
	uint32_t data[2] = {0, 0};

	nano_timer_init(&timer, data);

	while (1) {
		/* wait for task to let us have a turn */
		nano_sem_fiber.wait();

		/* say "hello" */
		PRINT("%s: Hello World!\n", __FUNCTION__);

		/* wait a while, then let task have a turn */
		nano_fiber_timer_start(&timer, SLEEPTICKS);
		nano_fiber_timer_test(&timer, TICKS_UNLIMITED);
		nano_sem_task.give();
	}
}

void main(void)
{
	struct nano_timer timer;
	uint32_t data[2] = {0, 0};

	task_fiber_start(&fiber_stack[0], STACKSIZE,
			(nano_fiber_entry_t) fiber_entry, 0, 0, 7, 0);

	nano_timer_init(&timer, data);

	while (1) {
		/* say "hello" */
		PRINT("%s: Hello World!\n", __FUNCTION__);

		/* wait a while, then let fiber have a turn */
		nano_task_timer_start(&timer, SLEEPTICKS);
		nano_task_timer_test(&timer, TICKS_UNLIMITED);
		nano_sem_fiber.give();

		/* now wait for fiber to let us have a turn */
		nano_sem_task.wait();
	}
}

#endif /* CONFIG_MICROKERNEL ||  CONFIG_NANOKERNEL */
