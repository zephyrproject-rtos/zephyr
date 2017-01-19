/*
 * Copyright (c) 2015-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file C++ Synchronization demo.  Uses basic C++ functionality.
 */

#include <stdio.h>

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
	struct k_sem _sema_internal;
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
task_semaphore::task_semaphore():
	_sema_internal(K_SEM_INITIALIZER(_sema_internal, 0, UINT32_MAX))
{
	printf("Create semaphore %p\n", this);
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
		printf("%s: Hello World!\n", taskname);

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
