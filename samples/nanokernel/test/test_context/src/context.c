/* context.c - test nanokernel CPU and context APIs */

/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
This module tests the following CPU and context related routines:
  fiber_fiber_start(), task_fiber_start(), fiber_yield(),
  context_self_get(), context_type_get(), nano_cpu_idle(),
  irq_lock(), irq_unlock(),
  irq_lock_inline(), irq_unlock_inline(),
  irq_connect(), nanoCpuExcConnect(),
  irq_enable(), irq_disable(),
 */

#include <tc_util.h>
#include <nano_private.h>
#include <arch/cpu.h>

/* test uses 1 software IRQs */
#define NUM_SW_IRQS 1

#include <irq_test_common.h>
#include <util_test_common.h>

/*
 * Include board.h from BSP to get IRQ number.
 * NOTE: Cortex-M3/M4 does not need IRQ numbers
 */
#if !defined(CONFIG_CPU_CORTEX_M3_M4)
  #include <board.h>
#endif

#define FIBER_STACKSIZE    2000
#define FIBER_PRIORITY     4

#define CTX_SELF_CMD       0
#define CTX_TYPE_CMD       1

#define UNKNOWN_COMMAND    -1

/*
 * Get the timer type dependent IRQ number if timer type
 * is not defined in BSP, generate an error
 */
#if defined(CONFIG_HPET_TIMER)
  #define TICK_IRQ HPET_TIMER0_IRQ
#elif defined(CONFIG_LOAPIC_TIMER)
  #define TICK_IRQ LOAPIC_TIMER_IRQ
#elif defined(CONFIG_PIT)
  #define TICK_IRQ PIT_INT_LVL
#elif defined(CONFIG_CPU_CORTEX_M3_M4)
  /* no need for TICK_IRQ definition, see note where it is used */
#else
  /* generate an error */
  #error Timer type is not defined for this BSP
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
static NANO_CPU_EXC_STUB_DECL(nanoExcStub);
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

static void (*_trigger_isrHandler)(void) = (vvfn)sw_isr_trigger_0;

/**
 *
 * isr_handler - handler to perform various actions from within an ISR context
 *
 * This routine is the ISR handler for _trigger_isrHandler().  It performs
 * the command requested in <isrInfo.command>.
 *
 * RETURNS: N/A
 */

void isr_handler(void *data)
{
	ARG_UNUSED(data);

	switch (isrInfo.command) {
	case CTX_SELF_CMD:
		isrInfo.data = (void *) context_self_get();
		break;

	case CTX_TYPE_CMD:
		isrInfo.value = context_type_get();
		break;

	default:
		isrInfo.error = UNKNOWN_COMMAND;
		break;
	}
}

/* Cortex-M3/M4 does not implement connecting non-IRQ exception handlers */
#if !defined(CONFIG_CPU_CORTEX_M3_M4)
/**
 *
 * exc_divide_error_handler - divide by zero exception handler
 *
 * This handler is part of a test that is only interested in detecting the
 * error so that we know the exception connect code is working. It simply
 * adds 2 to the EIP to skip over the offending instruction:
 *         f7 f9         idiv %ecx
 * thereby preventing the infinite loop of divide-by-zero errors which would
 * arise if control simply returns to that instruction.
 *
 * RETURNS: N/A
 */

void exc_divide_error_handler(NANO_ESF *pEsf)
{
	pEsf->eip += 2;
	excHandlerExecuted = 1;    /* provide evidence that the handler executed */
}
#endif

/**
 *
 * initNanoObjects - initialize nanokernel objects
 *
 * This routine initializes the nanokernel objects used in this module's tests.
 *
 * RETURNS: TC_PASS on success, TC_FAIL on failure
 */

int initNanoObjects(void)
{
	nano_sem_init(&wakeFiber);
	nano_timer_init(&timer, timerData);
	nano_fifo_init(&timeout_order_fifo);

/* no nanoCpuExcConnect on Cortex-M3/M4 */
#if !defined(CONFIG_CPU_CORTEX_M3_M4)
	nanoCpuExcConnect(IV_DIVIDE_ERROR, exc_divide_error_handler, nanoExcStub);
#endif

	struct isrInitInfo i = {
	{isr_handler, NULL},
	{&isrInfo, NULL},
	};

	return initIRQ(&i) < 0 ? TC_FAIL : TC_PASS;
}

/**
 *
 * nano_cpu_idleTest - test the nano_cpu_idle() routine
 *
 * This tests the nano_cpu_idle() routine.  The first thing it does is align to
 * a tick boundary.  The only source of interrupts while the test is running is
 * expected to be the tick clock timer which should wake the CPU.  Thus after
 * each call to nano_cpu_idle(), the tick count should be one higher.
 *
 * RETURNS: TC_PASS on success, TC_FAIL on failure
 */

int nano_cpu_idleTest(void)
{
	int  tick;   /* current tick count */
	int  i;      /* loop variable */

	/* Align to a "tick boundary". */
	tick = nano_tick_get_32();
	while (tick == nano_tick_get_32()) {
	}
	tick = nano_tick_get_32();

	for (i = 0; i < 5; i++) {     /* Repeat the test five times */
		nano_cpu_idle();
		tick++;
		if (nano_tick_get_32() != tick) {
			return TC_FAIL;
		}
	}

	return TC_PASS;
}

/**
 *
 * irq_lockWrapper - a wrapper for irq_lock()
 *
 * RETURNS: irq_lock() return value
 */

int irq_lockWrapper(int unused)
{
	ARG_UNUSED(unused);

	return irq_lock();
}

/**
 *
 * irq_unlockWrapper - a wrapper for irq_unlock()
 *
 * RETURNS: N/A
 */

void irq_unlockWrapper(int imask)
{
	irq_unlock(imask);
}

/**
 *
 * irq_lock_inlineWrapper - a wrapper for irq_lock_inline()
 *
 * RETURNS: irq_lock_inline() return value
 */

int irq_lock_inlineWrapper(int unused)
{
	ARG_UNUSED(unused);

	return irq_lock_inline();
}

/**
 *
 * irq_unlock_inlineWrapper - a wrapper for irq_unlock_inline()
 *
 * RETURNS: N/A
 */

void irq_unlock_inlineWrapper(int imask)
{
	irq_unlock_inline(imask);
}

/**
 *
 * irq_disableWrapper - a wrapper for irq_disable()
 *
 * RETURNS: <irq>
 */

int irq_disableWrapper(int irq)
{
	irq_disable(irq);
	return irq;
}

/**
 *
 * irq_enableWrapper - a wrapper for irq_enable()
 *
 * RETURNS: N/A
 */

void irq_enableWrapper(int irq)
{
	irq_enable(irq);
}

/**
 *
 * nanoCpuDisableInterruptsTest - test routines for disabling and enabling ints
 *
 * This routine tests the routines for disabling and enabling interrupts.  These
 * include irq_lock() and irq_unlock(), irq_lock_inline() and
 * irq_unlock_inline(), irq_disable() and irq_enable().
 *
 * RETURNS: TC_PASS on success, TC_FAIL on failure
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
	tick = nano_tick_get_32();
	while (nano_tick_get_32() == tick) {
	}
	tick++;

	while (nano_tick_get_32() == tick) {
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
	tick = nano_tick_get_32();
	for (i = 0; i < count; i++) {
		nano_tick_get_32();
	}

	tick2 = nano_tick_get_32();

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
		nano_tick_get_32();
	}

	return (tick == nano_tick_get_32()) ? TC_FAIL : TC_PASS;
}

/**
 *
 * nanoCtxTaskTest - test the various nanoCtxXXX() routines from a task
 *
 * This routines tests the context_self_get() and context_type_get() routines from both
 * a task and an ISR (that interrupted a task).  Checking those routines with
 * fibers are done elsewhere.
 *
 * RETURNS: TC_PASS on success, TC_FAIL on failure
 */

int nanoCtxTaskTest(void)
{
	nano_context_id_t  ctxId;

	TC_PRINT("Testing context_self_get() from an ISR and task\n");
	ctxId = context_self_get();
	isrInfo.command = CTX_SELF_CMD;
	isrInfo.error = 0;
	_trigger_isrHandler();
	if ((isrInfo.error != 0) || (isrInfo.data != (void *) ctxId)) {
		/*
		 * Either the ISR detected an error, or the ISR context ID does not
		 * match the interrupted task's context ID.
		 */
		return TC_FAIL;
	}

	TC_PRINT("Testing context_type_get() from an ISR\n");
	isrInfo.command = CTX_TYPE_CMD;
	isrInfo.error = 0;
	_trigger_isrHandler();
	if ((isrInfo.error != 0) || (isrInfo.value != NANO_CTX_ISR)) {
		return TC_FAIL;
	}

	TC_PRINT("Testing context_type_get() from a task\n");
	if (context_type_get() != NANO_CTX_TASK) {
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * nanoCtxFiberTest - test the various nanoCtxXXX() routines from a fiber
 *
 * This routines tests the context_self_get() and context_type_get() routines from both
 * a fiber and an ISR (that interrupted a fiber).  Checking those routines with
 * tasks are done elsewhere.
 *
 * This routine may set <fiberDetectedError> to the following values:
 *   1 - if fiber context ID matches that of the task
 *   2 - if context ID taken during ISR does not match that of the fiber
 *   3 - context_type_get() when called from an ISR is not NANO_TYPE_ISR
 *   3 - context_type_get() when called from a fiber is not NANO_TYPE_FIBER
 *
 * RETURNS: TC_PASS on success, TC_FAIL on failure
 */

int nanoCtxFiberTest(nano_context_id_t taskCtxId)
{
	nano_context_id_t  ctxId;

	ctxId = context_self_get();
	if (ctxId == taskCtxId) {
		fiberDetectedError = 1;
		return TC_FAIL;
	}

	isrInfo.command = CTX_SELF_CMD;
	isrInfo.error = 0;
	_trigger_isrHandler();
	if ((isrInfo.error != 0) || (isrInfo.data != (void *) ctxId)) {
		/*
		 * Either the ISR detected an error, or the ISR context ID does not
		 * match the interrupted fiber's context ID.
		 */
		fiberDetectedError = 2;
		return TC_FAIL;
	}

	isrInfo.command = CTX_TYPE_CMD;
	isrInfo.error = 0;
	_trigger_isrHandler();
	if ((isrInfo.error != 0) || (isrInfo.value != NANO_CTX_ISR)) {
		fiberDetectedError = 3;
		return TC_FAIL;
	}

	if (context_type_get() != NANO_CTX_FIBER) {
		fiberDetectedError = 4;
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * fiberHelper - entry point to the fiber's helper
 *
 * This routine is the entry point to the fiber's helper fiber.  It is used to
 * help test the behaviour of the fiber_yield() routine.
 *
 * @param arg1    unused
 * @param arg2    unused
 *
 * RETURNS: N/A
 */

static void fiberHelper(int arg1, int arg2)
{
	nano_context_id_t  ctxId;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	/*
	 * This fiber starts off at a higher priority than fiberEntry().  Thus, it
	 * should execute immediately.
	 */

	fiberEvidence++;

	/* Test that helper will yield to a fiber of equal priority */
	ctxId = context_self_get();
	ctxId->prio++;            /* Lower priority to that of fiberEntry() */
	fiber_yield();        /* Yield to fiber of equal priority */

	fiberEvidence++;
	/* <fiberEvidence> should now be 2 */

}

/**
 *
 * fiber_yieldTest - test the fiber_yield() routine
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
 * RETURNS: TC_PASS on success, TC_FAIL on failure
 */

int fiber_yieldTest(void)
{
	nano_context_id_t  ctxId;

	/*
	 * Start a fiber of higher priority.  Note that since the new fiber is
	 * being started from a fiber, it will not automatically switch to the
	 * fiber as it would if done from a task.
	 */

	ctxId = context_self_get();
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

	ctxId->prio--;
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
 * fiberEntry - entry point to fiber started by the task
 *
 * This routine is the entry point to the fiber started by the task.
 *
 * @param taskCtxId    context ID of the spawning task
 * @param arg1         unused
 *
 * RETURNS: N/A
 */

static void fiberEntry(int taskCtxId, int arg1)
{
	int          rv;

	ARG_UNUSED(arg1);

	fiberEvidence++;    /* Prove to the task that the fiber has run */
	nano_fiber_sem_take_wait(&wakeFiber);

	rv = nanoCtxFiberTest((nano_context_id_t) taskCtxId);
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

/* a fiber sleeps and times out, then reports through a fifo */
static void test_fiber_sleep(int timeout, int arg2)
{
	int64_t orig_ticks = nano_tick_get();

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
 * main - entry point to timer tests
 *
 * This is the entry point to the CPU and context tests.
 *
 * RETURNS: N/A
 */

void main(void)
{
	int           rv;       /* return value from tests */

	TC_START("Test Nanokernel CPU and context routines");

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


	TC_PRINT("Testing inline interrupt locking and unlocking\n");
	rv = nanoCpuDisableInterruptsTest(irq_lock_inlineWrapper,
									  irq_unlock_inlineWrapper, -1);
	if (rv != TC_PASS) {
		goto doneTests;
	}

/*
 * The Cortex-M3/M4 use the SYSTICK exception for the system timer, which is
 * not considered an IRQ by the irq_enable/Disable APIs.
 */
#if !defined(CONFIG_CPU_CORTEX_M3_M4)
	/*
	 * !!! TAKE NOTE !!!
	 * Disable interrupts coming from the timer.  In the pcPentium case, this
	 * is IRQ0 (see board.h for definition of PIT_INT_LVL).  Other BSPs may
	 * not be using the i8253 timer on IRQ0 and so a different IRQ value may
	 * be necessary when porting to another BSP.
	 */

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
					 (int) context_self_get(), 0, FIBER_PRIORITY, 0);

	if (fiberEvidence != 1) {
		rv = TC_FAIL;
		TC_ERROR("  - fiber did not execute as expected!\n");
		goto doneTests;
	}

	/* The fiber ran, now wake it so it can test context_self_get and context_type_get */
	TC_PRINT("Fiber to test context_self_get() and context_type_get\n");
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
