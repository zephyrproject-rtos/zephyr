/* thread.c - test nanokernel CPU and thread APIs */

/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
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

/*
DESCRIPTION
This module tests the following CPU and thread related routines:
  fiber_fiber_start(), task_fiber_start(), fiber_yield(),
  sys_thread_self_get(), sys_execution_context_type_get(), nano_cpu_idle(),
  irq_lock(), irq_unlock(),
  irq_connect(), nanoCpuExcConnect(),
  irq_enable(), irq_disable(),
 */

#include <tc_util.h>
#include <nano_private.h>
#include <arch/cpu.h>
#include <irq_offload.h>

#include <util_test_common.h>

/*
 * Include board.h from platform to get IRQ number.
 * NOTE: Cortex-M3/M4 does not need IRQ numbers
 */
#if !defined(CONFIG_CPU_CORTEX_M3_M4)
  #include <board.h>
#endif

#ifndef FIBER_STACKSIZE
#define FIBER_STACKSIZE    2000
#endif
#define FIBER_PRIORITY     4

#define THREAD_SELF_CMD    0
#define EXEC_CTX_TYPE_CMD  1

#define UNKNOWN_COMMAND    -1

/*
 * Get the timer type dependent IRQ number. If timer type
 * is not defined in platform, generate an error
 */
#if defined(CONFIG_HPET_TIMER)
  #define TICK_IRQ CONFIG_HPET_TIMER_IRQ
#elif defined(CONFIG_LOAPIC_TIMER)
  #define TICK_IRQ CONFIG_LOAPIC_TIMER_IRQ
#elif defined(CONFIG_CPU_CORTEX_M3_M4)
  /* Cortex-M3/M4 does not need a tick IRQ number. */
#else
  /* generate an error */
  #error Timer type is not defined for this platform
#endif

typedef struct {
	int     command;    /* command to process */
	int     error;      /* error value (if any) */
	union {
		void   *data;   /* pointer to data to use or return */
		int     value;  /* value to be passed or returned */
	};
} ISR_INFO;

typedef int  (* disable_interrupt_func)(int);
typedef void (* enable_interrupt_func)(int);

/* Cortex-M3/M4 does not implement connecting non-IRQ exception handlers */
#if !defined(CONFIG_CPU_CORTEX_M3_M4)
static volatile int    excHandlerExecuted;
#endif

static struct nano_sem        wakeFiber;
static struct nano_timer      timer;
static struct nano_sem        reply_timeout;
struct nano_fifo              timeout_order_fifo;
static void *timerData[1];

static int  fiberDetectedError = 0;
static char __stack fiberStack1[FIBER_STACKSIZE];
static char __stack fiberStack2[FIBER_STACKSIZE];
static int  fiberEvidence = 0;

static ISR_INFO  isrInfo;

/**
 *
 * @brief Handler to perform various actions from within an ISR context
 *
 * This routine is the ISR handler for _trigger_isrHandler().  It performs
 * the command requested in <isrInfo.command>.
 *
 * @return N/A
 */

void isr_handler(void *data)
{
	ARG_UNUSED(data);

	switch (isrInfo.command) {
	case THREAD_SELF_CMD:
		isrInfo.data = (void *) sys_thread_self_get();
		break;

	case EXEC_CTX_TYPE_CMD:
		isrInfo.value = sys_execution_context_type_get();
		break;

	default:
		isrInfo.error = UNKNOWN_COMMAND;
		break;
	}
}
static void _trigger_isrHandler(void)
{
	irq_offload(isr_handler, NULL);
}


/* Cortex-M3/M4 does not implement connecting non-IRQ exception handlers */
#if !defined(CONFIG_CPU_CORTEX_M3_M4)
/**
 *
 * @brief Divide by zero exception handler
 *
 * This handler is part of a test that is only interested in detecting the
 * error so that we know the exception connect code is working. It simply
 * adds 2 to the EIP to skip over the offending instruction:
 *         f7 f9         idiv %ecx
 * thereby preventing the infinite loop of divide-by-zero errors which would
 * arise if control simply returns to that instruction.
 *
 * @return N/A
 */

void exc_divide_error_handler(NANO_ESF *pEsf)
{
	pEsf->eip += 2;
	excHandlerExecuted = 1;    /* provide evidence that the handler executed */
}
#endif

/**
 *
 * @brief Initialize nanokernel objects
 *
 * This routine initializes the nanokernel objects used in this module's tests.
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int initNanoObjects(void)
{
	nano_sem_init(&wakeFiber);
	nano_timer_init(&timer, timerData);
	nano_fifo_init(&timeout_order_fifo);

/* no nanoCpuExcConnect on Cortex-M3/M4 */
#if !defined(CONFIG_CPU_CORTEX_M3_M4)
	nanoCpuExcConnect(IV_DIVIDE_ERROR, exc_divide_error_handler);
#endif

	return TC_PASS;
}

/**
 *
 * @brief Test the nano_cpu_idle() routine
 *
 * This tests the nano_cpu_idle() routine.  The first thing it does is align to
 * a tick boundary.  The only source of interrupts while the test is running is
 * expected to be the tick clock timer which should wake the CPU.  Thus after
 * each call to nano_cpu_idle(), the tick count should be one higher.
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int nano_cpu_idleTest(void)
{
	int  tick;   /* current tick count */
	int  i;      /* loop variable */

	/* Align to a "tick boundary". */
	tick = sys_tick_get_32();
	while (tick == sys_tick_get_32()) {
	}
	tick = sys_tick_get_32();

	for (i = 0; i < 5; i++) {     /* Repeat the test five times */
		nano_cpu_idle();
		tick++;
		if (sys_tick_get_32() != tick) {
			return TC_FAIL;
		}
	}

	return TC_PASS;
}

/**
 *
 * @brief A wrapper for irq_lock()
 *
 * @return irq_lock() return value
 */

int irq_lockWrapper(int unused)
{
	ARG_UNUSED(unused);

	return irq_lock();
}

/**
 *
 * @brief A wrapper for irq_unlock()
 *
 * @return N/A
 */

void irq_unlockWrapper(int imask)
{
	irq_unlock(imask);
}

/**
 *
 * @brief A wrapper for irq_disable()
 *
 * @return <irq>
 */

int irq_disableWrapper(int irq)
{
	irq_disable(irq);
	return irq;
}

/**
 *
 * @brief A wrapper for irq_enable()
 *
 * @return N/A
 */

void irq_enableWrapper(int irq)
{
	irq_enable(irq);
}

/**
 *
 * @brief Test routines for disabling and enabling ints
 *
 * This routine tests the routines for disabling and enabling interrupts.  These
 * include irq_lock() and irq_unlock(), irq_disable() and irq_enable().
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int nanoCpuDisableInterruptsTest(disable_interrupt_func disableRtn,
								 enable_interrupt_func enableRtn, int irq)
{
	unsigned long long  count = 0;
	unsigned long long  i = 0;
	int  tick;
	int  tick2;
	int  imask;

	/* Align to a "tick boundary" */
	tick = sys_tick_get_32();
	while (sys_tick_get_32() == tick) {
	}
	tick++;

	while (sys_tick_get_32() == tick) {
		count++;
	}

	/*
	 * Inflate <count> so that when we loop later, many ticks should have
	 * elapsed during the loop.  This later loop will not exactly match the
	 * previous loop, but it should be close enough in structure that when
	 * combined with the inflated count, many ticks will have passed.
	 */

	count <<= 4;

	imask = disableRtn(irq);
	tick = sys_tick_get_32();
	for (i = 0; i < count; i++) {
		sys_tick_get_32();
	}

	tick2 = sys_tick_get_32();

	/*
	 * Re-enable interrupts before returning (for both success and failure
	 * cases).
	 */

	enableRtn(imask);

	if (tick2 != tick) {
		return TC_FAIL;
	}

	/* Now repeat with interrupts unlocked. */
	for (i = 0; i < count; i++) {
		sys_tick_get_32();
	}

	return (tick == sys_tick_get_32()) ? TC_FAIL : TC_PASS;
}

/**
 *
 * @brief Test the various nanoCtxXXX() routines from a task
 *
 * This routines tests the sys_thread_self_get() and
 * sys_execution_context_type_get() routines from both a task and an ISR (that
 * interrupted a task).  Checking those routines with fibers are done
 * elsewhere.
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int nanoCtxTaskTest(void)
{
	nano_thread_id_t  self_thread_id;

	TC_PRINT("Testing sys_thread_self_get() from an ISR and task\n");
	self_thread_id = sys_thread_self_get();
	isrInfo.command = THREAD_SELF_CMD;
	isrInfo.error = 0;
	_trigger_isrHandler();
	if ((isrInfo.error != 0) || (isrInfo.data != (void *) self_thread_id)) {
		/*
		 * Either the ISR detected an error, or the ISR context ID does not
		 * match the interrupted task's thread ID.
		 */
		return TC_FAIL;
	}

	TC_PRINT("Testing sys_execution_context_type_get() from an ISR\n");
	isrInfo.command = EXEC_CTX_TYPE_CMD;
	isrInfo.error = 0;
	_trigger_isrHandler();
	if ((isrInfo.error != 0) || (isrInfo.value != NANO_CTX_ISR)) {
		return TC_FAIL;
	}

	TC_PRINT("Testing sys_execution_context_type_get() from a task\n");
	if (sys_execution_context_type_get() != NANO_CTX_TASK) {
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * @brief Test the various context/thread routines from a fiber
 *
 * This routines tests the sys_thread_self_get() and
 * sys_execution_context_type_get() routines from both a fiber and an ISR (that
 * interrupted a fiber).  Checking those routines with tasks are done
 * elsewhere.
 *
 * This routine may set <fiberDetectedError> to the following values:
 *   1 - if fiber ID matches that of the task
 *   2 - if thread ID taken during ISR does not match that of the fiber
 *   3 - sys_execution_context_type_get() when called from an ISR is not
 *       NANO_TYPE_ISR
 *   4 - sys_execution_context_type_get() when called from a fiber is not
 *       NANO_TYPE_FIBER
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int nanoCtxFiberTest(nano_thread_id_t task_thread_id)
{
	nano_thread_id_t  self_thread_id;

	self_thread_id = sys_thread_self_get();
	if (self_thread_id == task_thread_id) {
		fiberDetectedError = 1;
		return TC_FAIL;
	}

	isrInfo.command = THREAD_SELF_CMD;
	isrInfo.error = 0;
	_trigger_isrHandler();
	if ((isrInfo.error != 0) || (isrInfo.data != (void *) self_thread_id)) {
		/*
		 * Either the ISR detected an error, or the ISR context ID does not
		 * match the interrupted fiber's thread ID.
		 */
		fiberDetectedError = 2;
		return TC_FAIL;
	}

	isrInfo.command = EXEC_CTX_TYPE_CMD;
	isrInfo.error = 0;
	_trigger_isrHandler();
	if ((isrInfo.error != 0) || (isrInfo.value != NANO_CTX_ISR)) {
		fiberDetectedError = 3;
		return TC_FAIL;
	}

	if (sys_execution_context_type_get() != NANO_CTX_FIBER) {
		fiberDetectedError = 4;
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * @brief Entry point to the fiber's helper
 *
 * This routine is the entry point to the fiber's helper fiber.  It is used to
 * help test the behaviour of the fiber_yield() routine.
 *
 * @param arg1    unused
 * @param arg2    unused
 *
 * @return N/A
 */

static void fiberHelper(int arg1, int arg2)
{
	nano_thread_id_t  self_thread_id;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	/*
	 * This fiber starts off at a higher priority than fiberEntry().  Thus, it
	 * should execute immediately.
	 */

	fiberEvidence++;

	/* Test that helper will yield to a fiber of equal priority */
	self_thread_id = sys_thread_self_get();
	self_thread_id->prio++;  /* Lower priority to that of fiberEntry() */
	fiber_yield();        /* Yield to fiber of equal priority */

	fiberEvidence++;
	/* <fiberEvidence> should now be 2 */

}

/**
 *
 * @brief Test the fiber_yield() routine
 *
 * This routine tests the fiber_yield() routine.  It starts another fiber
 * (thus also testing fiber_fiber_start()) and checks that behaviour of
 * fiber_yield() against the cases of there being a higher priority fiber,
 * a lower priority fiber, and another fiber of equal priority.
 *
 * On error, it may set <fiberDetectedError> to one of the following values:
 *   10 - helper fiber ran prematurely
 *   11 - fiber_yield() did not yield to a higher priority fiber
 *   12 - fiber_yield() did not yield to an equal prioirty fiber
 *   13 - fiber_yield() yielded to a lower priority fiber
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int fiber_yieldTest(void)
{
	nano_thread_id_t  self_thread_id;

	/*
	 * Start a fiber of higher priority.  Note that since the new fiber is
	 * being started from a fiber, it will not automatically switch to the
	 * fiber as it would if done from a task.
	 */

	self_thread_id = sys_thread_self_get();
	fiberEvidence = 0;
	fiber_fiber_start(fiberStack2, FIBER_STACKSIZE, fiberHelper,
		0, 0, FIBER_PRIORITY - 1, 0);

	if (fiberEvidence != 0) {
		/* ERROR! Helper spawned at higher */
		fiberDetectedError = 10;    /* priority ran prematurely. */
		return TC_FAIL;
	}

	/*
	 * Test that the fiber will yield to the higher priority helper.
	 * <fiberEvidence> is still 0.
	 */

	fiber_yield();

	if (fiberEvidence == 0) {
		/* ERROR! Did not yield to higher */
		fiberDetectedError = 11;    /* priority fiber. */
		return TC_FAIL;
	}

	if (fiberEvidence > 1) {
		/* ERROR! Helper did not yield to */
		fiberDetectedError = 12;    /* equal priority fiber. */
		return TC_FAIL;
	}

	/*
	 * Raise the priority of fiberEntry().  Calling fiber_yield() should
	 * not result in switching to the helper.
	 */

	self_thread_id->prio--;
	fiber_yield();

	if (fiberEvidence != 1) {
		/* ERROR! Context switched to a lower */
		fiberDetectedError = 13;    /* priority fiber! */
		return TC_FAIL;
	}

	/*
	 * Block on <wakeFiber>.  This will allow the helper fiber to complete.
	 * The main task will wake this fiber.
	 */

	nano_fiber_sem_take_wait(&wakeFiber);

	return TC_PASS;
}

/**
 *
 * @brief Entry point to fiber started by the task
 *
 * This routine is the entry point to the fiber started by the task.
 *
 * @param task_thread_id    thread ID of the spawning task
 * @param arg1         unused
 *
 * @return N/A
 */

static void fiberEntry(int task_thread_id, int arg1)
{
	int          rv;

	ARG_UNUSED(arg1);

	fiberEvidence++;    /* Prove to the task that the fiber has run */
	nano_fiber_sem_take_wait(&wakeFiber);

	rv = nanoCtxFiberTest((nano_thread_id_t) task_thread_id);
	if (rv != TC_PASS) {
		return;
	}

	/* Allow the task to print any messages before the next test runs */
	nano_fiber_sem_take_wait(&wakeFiber);

	rv = fiber_yieldTest();
	if (rv != TC_PASS) {
		return;
	}
}

/*
 * Timeout tests
 *
 * Test the fiber_sleep() API, as well as the fiber_delayed_start() ones.
 */

#include <tc_nano_timeout_common.h>

struct timeout_order_data {
	void *link_in_fifo;
	int32_t timeout;
	int timeout_order;
	int q_order;
};

struct timeout_order_data timeout_order_data[] = {
	{0, TIMEOUT(2), 2, 0},
	{0, TIMEOUT(4), 4, 1},
	{0, TIMEOUT(0), 0, 2},
	{0, TIMEOUT(1), 1, 3},
	{0, TIMEOUT(5), 5, 4},
	{0, TIMEOUT(6), 6, 5},
	{0, TIMEOUT(3), 3, 6},
};

#define NUM_TIMEOUT_FIBERS ARRAY_SIZE(timeout_order_data)
static char __stack timeout_stacks[NUM_TIMEOUT_FIBERS][FIBER_STACKSIZE];

#ifndef CONFIG_ARM
/* a fiber busy waits, then reports through a fifo */
static void test_fiber_busy_wait(int ticks, int unused)
{
	ARG_UNUSED(unused);

	uint32_t usecs = ticks * sys_clock_us_per_tick;

	TC_PRINT(" fiber busy waiting for %d usecs (%d ticks)\n",
			 usecs, ticks);
	sys_thread_busy_wait(usecs);
	TC_PRINT(" fiber busy waiting completed\n");

	/*
	 * Ideally the test should verify that the correct number of ticks
	 * have elapsed. However, when run under QEMU the tick interrupt
	 * may be processed on a very irregular basis, meaning that far
	 * fewer than the expected number of ticks may occur for a given
	 * number of clock cycles vs. what would ordinarily be expected.
	 *
	 * Consequently, the best we can do for now to test busy waiting is
	 * to invoke the API and verify that it returns. (If it takes way
	 * too long, or never returns, the main test task may be able to
	 * time out and report an error.)
	 */

	nano_fiber_sem_give(&reply_timeout);
}
#endif

/* a fiber sleeps and times out, then reports through a fifo */
static void test_fiber_sleep(int timeout, int arg2)
{
	int64_t orig_ticks = sys_tick_get();

	TC_PRINT(" fiber sleeping for %d ticks\n", timeout);
	fiber_sleep(timeout);
	TC_PRINT(" fiber back from sleep\n");
	if (!is_timeout_in_range(orig_ticks, timeout)) {
		return;
	}

	nano_fiber_sem_give(&reply_timeout);
}

/* a fiber is started with a delay, then it reports that it ran via a fifo */
void delayed_fiber(int num, int unused)
{
	struct timeout_order_data *data = &timeout_order_data[num];

	ARG_UNUSED(unused);

	TC_PRINT(" fiber (q order: %d, t/o: %d) is running\n",
				data->q_order, data->timeout);

	nano_fiber_fifo_put(&timeout_order_fifo, data);
}

static int test_timeout(void)
{
	int32_t timeout;
	int rv;
	int ii;
	struct timeout_order_data *data;

/*
 * sys_thread_busy_wait() is currently unsupported for ARM
 */

#ifndef CONFIG_ARM

	/* test sys_thread_busy_wait() */

	TC_PRINT("Testing sys_thread_busy_wait()\n");
	timeout = 2;
	task_fiber_start(timeout_stacks[0], FIBER_STACKSIZE,
						test_fiber_busy_wait, (int)timeout, 0,
						FIBER_PRIORITY, 0);

	rv = nano_task_sem_take_wait_timeout(&reply_timeout, timeout + 2);
	if (!rv) {
		rv = TC_FAIL;
		TC_ERROR(" *** task timed out waiting for sys_thread_busy_wait()\n");
		return TC_FAIL;
	}

#endif /* CONFIG_ARM */

	/* test fiber_sleep() */

	TC_PRINT("Testing fiber_sleep()\n");
	timeout = 5;
	task_fiber_start(timeout_stacks[0], FIBER_STACKSIZE,
						test_fiber_sleep, (int)timeout, 0,
						FIBER_PRIORITY, 0);

	rv = nano_task_sem_take_wait_timeout(&reply_timeout, timeout + 5);
	if (!rv) {
		rv = TC_FAIL;
		TC_ERROR(" *** task timed out waiting for fiber on fiber_sleep().\n");
		return TC_FAIL;
	}

	/* test fiber_delayed_start() without cancellation */

	TC_PRINT("Testing fiber_delayed_start() without cancellation\n");

	for (ii = 0; ii < NUM_TIMEOUT_FIBERS; ii++) {
		(void)task_fiber_delayed_start(timeout_stacks[ii], FIBER_STACKSIZE,
										delayed_fiber, ii, 0, 5, 0,
										timeout_order_data[ii].timeout);
	}
	for (ii = 0; ii < NUM_TIMEOUT_FIBERS; ii++) {

		data = nano_task_fifo_get_wait_timeout(&timeout_order_fifo,
												TIMEOUT_TWO_INTERVALS);

		if (!data) {
			TC_ERROR(" *** timeout while waiting for delayed fiber\n");
			return TC_FAIL;
		}

		if (data->timeout_order != ii) {
			TC_ERROR(" *** wrong delayed fiber ran (got %d, expected %d)\n",
						data->timeout_order, ii);
			return TC_FAIL;
		}

		TC_PRINT(" got fiber (q order: %d, t/o: %d) as expected\n",
					data->q_order, data->timeout);
	}

	/* ensure no more fibers fire */

	data = nano_task_fifo_get_wait_timeout(&timeout_order_fifo,
											TIMEOUT_TWO_INTERVALS);

	if (data) {
		TC_ERROR(" *** got something on the fifo, but shouldn't have...\n");
		return TC_FAIL;
	}

	/* test fiber_delayed_start() with cancellation */

	TC_PRINT("Testing fiber_delayed_start() with cancellations\n");

	int cancellations[] = {0, 3, 4, 6};
	int num_cancellations = ARRAY_SIZE(cancellations);
	int next_cancellation = 0;

	void *delayed_fibers[NUM_TIMEOUT_FIBERS];

	for (ii = 0; ii < NUM_TIMEOUT_FIBERS; ii++) {
		delayed_fibers[ii] =
			task_fiber_delayed_start(timeout_stacks[ii], FIBER_STACKSIZE,
										delayed_fiber, ii, 0, 5, 0,
										timeout_order_data[ii].timeout);
	}

	for (ii = 0; ii < NUM_TIMEOUT_FIBERS; ii++) {
		int jj;

		if (ii == cancellations[next_cancellation]) {
			TC_PRINT(" cancelling [q order: %d, t/o: %d, t/o order: %d]\n",
						timeout_order_data[ii].q_order,
						timeout_order_data[ii].timeout, ii);
			for (jj = 0; jj < NUM_TIMEOUT_FIBERS; jj++) {
				if (timeout_order_data[jj].timeout_order == ii) {
					break;
				}
			}
			task_fiber_delayed_start_cancel(delayed_fibers[jj]);
			++next_cancellation;
			continue;
		}

		data = nano_task_fifo_get_wait_timeout(&timeout_order_fifo,
												TIMEOUT_TEN_INTERVALS);

		if (!data) {
			TC_ERROR(" *** timeout while waiting for delayed fiber\n");
			return TC_FAIL;
		}

		if (data->timeout_order != ii) {
			TC_ERROR(" *** wrong delayed fiber ran (got %d, expected %d)\n",
						data->timeout_order, ii);
			return TC_FAIL;
		}

		TC_PRINT(" got (q order: %d, t/o: %d, t/o order %d) as expected\n",
					data->q_order, data->timeout);
	}

	if (num_cancellations != next_cancellation) {
		TC_ERROR(" *** wrong number of cancellations (expected %d, got %d\n",
					num_cancellations, next_cancellation);
		return TC_FAIL;
	}

	/* ensure no more fibers fire */

	data = nano_task_fifo_get_wait_timeout(&timeout_order_fifo,
											TIMEOUT_TWO_INTERVALS);

	if (data) {
		TC_ERROR(" *** got something on the fifo, but shouldn't have...\n");
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * @brief Entry point to timer tests
 *
 * This is the entry point to the CPU and thread tests.
 *
 * @return N/A
 */

void main(void)
{
	int           rv;       /* return value from tests */

	TC_START("Test Nanokernel CPU and thread routines");

	TC_PRINT("Initializing nanokernel objects\n");
	rv = initNanoObjects();
	if (rv != TC_PASS) {
		goto doneTests;
	}

	TC_PRINT("Testing nano_cpu_idle()\n");
	rv = nano_cpu_idleTest();
	if (rv != TC_PASS) {
		goto doneTests;
	}

	TC_PRINT("Testing interrupt locking and unlocking\n");
	rv = nanoCpuDisableInterruptsTest(irq_lockWrapper,
									  irq_unlockWrapper, -1);
	if (rv != TC_PASS) {
		goto doneTests;
	}


/*
 * The Cortex-M3/M4 use the SYSTICK exception for the system timer, which is
 * not considered an IRQ by the irq_enable/Disable APIs.
 */
#if !defined(CONFIG_CPU_CORTEX_M3_M4)
	/* Disable interrupts coming from the timer. */

	TC_PRINT("Testing irq_disable() and irq_enable()\n");
	rv = nanoCpuDisableInterruptsTest(irq_disableWrapper,
									  irq_enableWrapper, TICK_IRQ);
	if (rv != TC_PASS) {
		goto doneTests;
	}
#endif

	rv = nanoCtxTaskTest();
	if (rv != TC_PASS) {
		goto doneTests;
	}

	TC_PRINT("Spawning a fiber from a task\n");
	fiberEvidence = 0;
	task_fiber_start(fiberStack1, FIBER_STACKSIZE, fiberEntry,
					 (int) sys_thread_self_get(), 0, FIBER_PRIORITY, 0);

	if (fiberEvidence != 1) {
		rv = TC_FAIL;
		TC_ERROR("  - fiber did not execute as expected!\n");
		goto doneTests;
	}

	/*
	 * The fiber ran, now wake it so it can test sys_thread_self_get and
	 * sys_execution_context_type_get.
	 */
	TC_PRINT("Fiber to test sys_thread_self_get() and sys_execution_context_type_get\n");
	nano_task_sem_give(&wakeFiber);

	if (fiberDetectedError != 0) {
		rv = TC_FAIL;
		TC_ERROR("  - failure detected in fiber; fiberDetectedError = %d\n",
				 fiberDetectedError);
		goto doneTests;
	}

	TC_PRINT("Fiber to test fiber_yield()\n");
	nano_task_sem_give(&wakeFiber);

	if (fiberDetectedError != 0) {
		rv = TC_FAIL;
		TC_ERROR("  - failure detected in fiber; fiberDetectedError = %d\n",
				 fiberDetectedError);
		goto doneTests;
	}

	nano_task_sem_give(&wakeFiber);

	rv = test_timeout();
	if (rv != TC_PASS) {
		goto doneTests;
	}

/* Cortex-M3/M4 does not implement connecting non-IRQ exception handlers */
#if !defined(CONFIG_CPU_CORTEX_M3_M4)
	/*
	 * Test divide by zero exception handler.
	 *
	 * WARNING: This code has been very carefully crafted so that it does
	 * what it is supposed to. Both "error" and "excHandlerExecuted" must be
	 * volatile to prevent the compiler from issuing a "divide by zero"
	 * warning (since otherwise in knows "excHandlerExecuted" is zero),
	 * and to ensure the compiler issues the two byte "idiv" instruction
	 * that the exception handler is designed to deal with.
	 */

	volatile int error;    /* used to create a divide by zero error */
	TC_PRINT("Verifying exception handler installed\n");
	excHandlerExecuted = 0;
	error = error / excHandlerExecuted;
	TC_PRINT("excHandlerExecuted: %d\n", excHandlerExecuted);

	rv = (excHandlerExecuted == 1) ? TC_PASS : TC_FAIL;
#endif

doneTests:
	TC_END_RESULT(rv);
	TC_END_REPORT(rv);
}
