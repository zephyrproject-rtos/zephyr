/* context.c - test context and thread APIs */

/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DESCRIPTION
 * This module tests the following CPU and thread related routines:
 * k_thread_create, k_yield(), k_is_in_isr(),
 * k_current_get(), k_cpu_idle(), k_cpu_atomic_idle(),
 * irq_lock(), irq_unlock(),
 * irq_offload(), irq_enable(), irq_disable(),
 */

#include <tc_util.h>
#include <kernel_structs.h>
#include <arch/cpu.h>
#include <irq_offload.h>

#include <util_test_common.h>

/*
 * Include board.h from platform to get IRQ number.
 * NOTE: Cortex-M does not need IRQ numbers
 */
#if !defined(CONFIG_CPU_CORTEX_M)
#include <board.h>
#endif

#define THREAD_STACKSIZE    (384 + CONFIG_TEST_EXTRA_STACKSIZE)
#define THREAD_PRIORITY     4

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
#if defined(CONFIG_LOAPIC)
#define TICK_IRQ CONFIG_LOAPIC_TIMER_IRQ
#else
/* MVIC case */
#define TICK_IRQ CONFIG_MVIC_TIMER_IRQ
#endif
#elif defined(CONFIG_XTENSA)
#include <xtensa_timer.h>
#define TICK_IRQ XT_TIMER_INTNUM
#elif defined(CONFIG_ALTERA_AVALON_TIMER)
#define TICK_IRQ TIMER_0_IRQ
#elif defined(CONFIG_ARCV2_TIMER)
#define TICK_IRQ IRQ_TIMER0
#elif defined(CONFIG_PULPINO_TIMER)
#define TICK_IRQ PULP_TIMER_A_CMP_IRQ
#elif defined(CONFIG_RISCV_MACHINE_TIMER)
#define TICK_IRQ RISCV_MACHINE_TIMER_IRQ
#elif defined(CONFIG_CPU_CORTEX_M)
/*
 * The Cortex-M use the SYSTICK exception for the system timer, which is
 * not considered an IRQ by the irq_enable/Disable APIs.
 */
#else
/* generate an error */
#error Timer type is not defined for this platform
#endif

/* Nios II and RISCV32 without CONFIG_RISCV_HAS_CPU_IDLE
 * do have a power saving instruction, so k_cpu_idle() returns immediately
 */
#if !defined(CONFIG_NIOS2) && \
	(!defined(CONFIG_RISCV32) || defined(CONFIG_RISCV_HAS_CPU_IDLE))
#define HAS_POWERSAVE_INSTRUCTION
#endif



extern u32_t _tick_get_32(void);
extern s64_t _tick_get(void);

typedef struct {
	int command;            /* command to process   */
	int error;              /* error value (if any) */
	union {
		void *data;     /* pointer to data to use or return */
		int value;      /* value to be passed or returned   */
	};
} ISR_INFO;

typedef int (*disable_int_func) (int);
typedef void (*enable_int_func) (int);

static struct k_sem sem_thread;
static struct k_timer timer;
static struct k_sem reply_timeout;
struct k_fifo timeout_order_fifo;

static int thread_detected_error;
static int thread_evidence;

static K_THREAD_STACK_DEFINE(thread_stack1, THREAD_STACKSIZE);
static K_THREAD_STACK_DEFINE(thread_stack2, THREAD_STACKSIZE);
static struct k_thread thread_data1;
static struct k_thread thread_data2;

static ISR_INFO isr_info;

/**
 *
 * @brief Handler to perform various actions from within an ISR context
 *
 * This routine is the ISR handler for isr_handler_trigger().  It performs
 * the command requested in <isr_info.command>.
 *
 * @return N/A
 */
static void isr_handler(void *data)
{
	ARG_UNUSED(data);

	switch (isr_info.command) {
	case THREAD_SELF_CMD:
		isr_info.data = (void *)k_current_get();
		break;

	case EXEC_CTX_TYPE_CMD:
		if (k_is_in_isr()) {
			isr_info.value = K_ISR;
			break;
		}

		if (_current->base.prio < 0) {
			isr_info.value = K_COOP_THREAD;
			break;
		}

		isr_info.value = K_PREEMPT_THREAD;

		break;

	default:
		isr_info.error = UNKNOWN_COMMAND;
		break;
	}
}

static void isr_handler_trigger(void)
{
	irq_offload(isr_handler, NULL);
}

/**
 *
 * @brief Initialize kernel objects
 *
 * This routine initializes the kernel objects used in this module's tests.
 *
 * @return TC_PASS
 */
static int kernel_init_objects(void)
{
	k_sem_init(&sem_thread, 0, UINT_MAX);
	k_sem_init(&reply_timeout, 0, UINT_MAX);
	k_timer_init(&timer, NULL, NULL);
	k_fifo_init(&timeout_order_fifo);

	return TC_PASS;
}

#ifdef HAS_POWERSAVE_INSTRUCTION
/**
 *
 * @brief Test the k_cpu_idle() routine
 *
 * This tests the k_cpu_idle() routine.  The first thing it does is align to
 * a tick boundary.  The only source of interrupts while the test is running is
 * expected to be the tick clock timer which should wake the CPU.  Thus after
 * each call to k_cpu_idle(), the tick count should be one higher.
 *
 * @return TC_PASS on success
 * @return TC_FAIL on failure
 */
static int test_kernel_cpu_idle(int atomic)
{
	int tms, tms2;;         /* current time in millisecond */
	int i;                  /* loop variable */

	/* Align to a "ms boundary". */
	tms = k_uptime_get_32();
	while (tms == k_uptime_get_32()) {
	}

	tms = k_uptime_get_32();
	for (i = 0; i < 5; i++) {       /* Repeat the test five times */
		if (atomic) {
			unsigned int key = irq_lock();

			k_cpu_atomic_idle(key);
		} else {
			k_cpu_idle();
		}
		/* calculating milliseconds per tick*/
		tms += sys_clock_us_per_tick / USEC_PER_MSEC;
		tms2 = k_uptime_get_32();
		if (tms2 < tms) {
			TC_ERROR("Bad ms per tick value computed, got %d which is less than %d\n",
				 tms2, tms);
			return TC_FAIL;
		}
	}
	return TC_PASS;
}
#endif

/**
 *
 * @brief A wrapper for irq_lock()
 *
 * @return irq_lock() return value
 */
int irq_lock_wrapper(int unused)
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
void irq_unlock_wrapper(int imask)
{
	irq_unlock(imask);
}

/**
 *
 * @brief A wrapper for irq_disable()
 *
 * @return <irq>
 */
int irq_disable_wrapper(int irq)
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
void irq_enable_wrapper(int irq)
{
	irq_enable(irq);
}

/**
 *
 * @brief Test routines for disabling and enabling ints
 *
 * This routine tests the routines for disabling and enabling interrupts.
 * These include irq_lock() and irq_unlock(), irq_disable() and irq_enable().
 *
 * @return TC_PASS on success
 * @return TC_FAIL on failure
 */
static int test_kernel_interrupts(disable_int_func disable_int,
				  enable_int_func enable_int, int irq)
{
	unsigned long long count = 0;
	unsigned long long i = 0;
	int tick;
	int tick2;
	int imask;

	/* Align to a "tick boundary" */
	tick = _tick_get_32();
	while (_tick_get_32() == tick) {
	}

	tick++;
	while (_tick_get_32() == tick) {
		count++;
	}

	/*
	 * Inflate <count> so that when we loop later, many ticks should have
	 * elapsed during the loop.  This later loop will not exactly match the
	 * previous loop, but it should be close enough in structure that when
	 * combined with the inflated count, many ticks will have passed.
	 */

	count <<= 4;

	imask = disable_int(irq);
	tick = _tick_get_32();
	for (i = 0; i < count; i++) {
		_tick_get_32();
	}

	tick2 = _tick_get_32();

	/*
	 * Re-enable interrupts before returning (for both success and failure
	 * cases).
	 */

	enable_int(imask);

	if (tick2 != tick) {
		TC_ERROR("tick advanced with interrupts locked\n");
		return TC_FAIL;
	}

	/* Now repeat with interrupts unlocked. */
	for (i = 0; i < count; i++) {
		_tick_get_32();
	}

	tick2 = _tick_get_32();
	if (tick == tick2) {
		TC_ERROR("tick didn't advance as expected\n");
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * @brief Test some context routines from a preemptible thread
 *
 * This routines tests the k_current_get() and
 * k_is_in_isr() routines from both a preemtible thread  and an ISR (that
 * interrupted a preemtible thread). Checking those routines with cooperative
 * threads are done elsewhere.
 *
 * @return TC_PASS on success
 * @return TC_FAIL on failure
 */
static int test_kernel_ctx_task(void)
{
	k_tid_t self_thread_id;

	TC_PRINT("Testing k_current_get() from an ISR and task\n");

	self_thread_id = k_current_get();
	isr_info.command = THREAD_SELF_CMD;
	isr_info.error = 0;
	/* isr_info is modified by the isr_handler routine */
	isr_handler_trigger();

	if (isr_info.error) {
		TC_ERROR("ISR detected an error\n");
		return TC_FAIL;
	}

	if (isr_info.data != (void *)self_thread_id) {
		TC_ERROR("ISR context ID mismatch\n");
		return TC_FAIL;
	}

	TC_PRINT("Testing k_is_in_isr() from an ISR\n");
	isr_info.command = EXEC_CTX_TYPE_CMD;
	isr_info.error = 0;
	isr_handler_trigger();

	if (isr_info.error) {
		TC_ERROR("ISR detected an error\n");
		return TC_FAIL;
	}

	if (isr_info.value != K_ISR) {
		TC_ERROR("isr_info.value was not K_ISR\n");
		return TC_FAIL;
	}

	TC_PRINT("Testing k_is_in_isr() from a preemptible thread\n");
	if (k_is_in_isr()) {
		TC_ERROR("Should not be in ISR context\n");
		return TC_FAIL;
	}
	if (_current->base.prio < 0) {
		TC_ERROR("Current thread should have preemptible priority\n");
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * @brief Test the various context/thread routines from a cooperative thread
 *
 * This routines tests the k_current_get and
 * k_is_in_isr() routines from both a thread and an ISR (that interrupted a
 * cooperative thread).  Checking those routines with preemptible threads are
 * done elsewhere.
 *
 * This routine may set <thread_detected_error> to the following values:
 *   1 - if thread ID matches that of the task
 *   2 - if thread ID taken during ISR does not match that of the thread
 *   3 - k_is_in_isr() when called from an ISR is false
 *   4 - k_is_in_isr() when called from a thread is true
 *   5 - if thread is not a cooperative thread
 *
 * @return TC_PASS on success
 * @return TC_FAIL on failure
 */
static int test_kernel_thread(k_tid_t task_thread_id)
{
	k_tid_t self_thread_id;

	self_thread_id = k_current_get();
	if (self_thread_id == task_thread_id) {
		thread_detected_error = 1;
		return TC_FAIL;
	}

	isr_info.command = THREAD_SELF_CMD;
	isr_info.error = 0;
	isr_handler_trigger();
	if (isr_info.error || isr_info.data != (void *)self_thread_id) {
		/*
		 * Either the ISR detected an error, or the ISR context ID
		 * does not match the interrupted thread's thread ID.
		 */
		thread_detected_error = 2;
		return TC_FAIL;
	}

	isr_info.command = EXEC_CTX_TYPE_CMD;
	isr_info.error = 0;
	isr_handler_trigger();
	if (isr_info.error || (isr_info.value != K_ISR)) {
		thread_detected_error = 3;
		return TC_FAIL;
	}

	if (k_is_in_isr()) {
		thread_detected_error = 4;
		return TC_FAIL;
	}

	if (_current->base.prio >= 0) {
		thread_detected_error = 5;
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * @brief Entry point to the thread's helper
 *
 * This routine is the entry point to the thread's helper thread.  It is used to
 * help test the behaviour of the k_yield() routine.
 *
 * @param arg1    unused
 * @param arg2    unused
 *
 * @return N/A
 */

static void thread_helper(void *arg1, void *arg2, void *arg3)
{
	k_tid_t self_thread_id;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	/*
	 * This thread starts off at a higher priority than thread_entry().
	 * Thus, it should execute immediately.
	 */

	thread_evidence++;

	/* Test that helper will yield to a thread of equal priority */
	self_thread_id = k_current_get();

	/* Lower priority to that of thread_entry() */
	k_thread_priority_set(self_thread_id, self_thread_id->base.prio + 1);

	k_yield();              /* Yield to thread of equal priority */

	thread_evidence++;
	/* <thread_evidence> should now be 2 */

}

/**
 *
 * @brief Test the k_yield() routine
 *
 * This routine tests the k_yield() routine.  It starts another thread
 * (thus also testing k_thread_create() and checks that behaviour of
 * k_yield() against the cases of there being a higher priority thread,
 * a lower priority thread, and another thread of equal priority.
 *
 * On error, it may set <thread_detected_error> to one of the following values:
 *   10 - helper thread ran prematurely
 *   11 - k_yield() did not yield to a higher priority thread
 *   12 - k_yield() did not yield to an equal prioirty thread
 *   13 - k_yield() yielded to a lower priority thread
 *
 * @return TC_PASS on success
 * @return TC_FAIL on failure
 */
static int test_k_yield(void)
{
	k_tid_t self_thread_id;

	/*
	 * Start a thread of higher priority.  Note that since the new thread is
	 * being started from a thread, it will not automatically switch to the
	 * thread as it would if done from a task.
	 */

	self_thread_id = k_current_get();
	thread_evidence = 0;

	k_thread_create(&thread_data2, thread_stack2, THREAD_STACKSIZE,
			thread_helper, NULL, NULL, NULL,
			K_PRIO_COOP(THREAD_PRIORITY - 1), 0, 0);

	if (thread_evidence != 0) {
		/* ERROR! Helper created at higher */
		thread_detected_error = 10;     /* priority ran prematurely. */
		return TC_FAIL;
	}

	/*
	 * Test that the thread will yield to the higher priority helper.
	 * <thread_evidence> is still 0.
	 */

	k_yield();

	if (thread_evidence == 0) {
		/* ERROR! Did not yield to higher */
		thread_detected_error = 11;     /* priority thread. */
		return TC_FAIL;
	}

	if (thread_evidence > 1) {
		/* ERROR! Helper did not yield to */
		thread_detected_error = 12;     /* equal priority thread. */
		return TC_FAIL;
	}

	/*
	 * Raise the priority of thread_entry().  Calling k_yield() should
	 * not result in switching to the helper.
	 */

	k_thread_priority_set(self_thread_id, self_thread_id->base.prio - 1);
	k_yield();

	if (thread_evidence != 1) {
		/* ERROR! Context switched to a lower */
		thread_detected_error = 13;     /* priority thread! */
		return TC_FAIL;
	}

	/*
	 * Block on <sem_thread>.  This will allow the helper thread to
	 * complete. The main task will wake this thread.
	 */

	k_sem_take(&sem_thread, K_FOREVER);

	return TC_PASS;
}

/**
 * @brief Entry point to thread started by the task
 *
 * This routine is the entry point to the thread started by the task.
 *
 * @param task_thread_id	thread ID of the spawning task
 * @param arg1			unused
 * @param arg2			unused
 *
 * @return N/A
 */
static void thread_entry(void *task_thread_id, void *arg1, void *arg2)
{
	int rv;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	thread_evidence++;      /* Prove to the task that the thread has run */
	k_sem_take(&sem_thread, K_FOREVER);

	rv = test_kernel_thread((k_tid_t) task_thread_id);
	if (rv != TC_PASS) {
		return;
	}

	/* Allow the task to print any messages before the next test runs */
	k_sem_take(&sem_thread, K_FOREVER);

	rv = test_k_yield();
	if (rv != TC_PASS) {
		return;
	}
}

/*
 * Timeout tests
 *
 * Test the k_sleep() API, as well as the k_thread_create() ones.
 */

struct timeout_order {
	void *link_in_fifo;
	s32_t timeout;
	int timeout_order;
	int q_order;
};

struct timeout_order timeouts[] = {
	{ 0, 1000, 2, 0 },
	{ 0, 1500, 4, 1 },
	{ 0, 500, 0, 2 },
	{ 0, 750, 1, 3 },
	{ 0, 1750, 5, 4 },
	{ 0, 2000, 6, 5 },
	{ 0, 1250, 3, 6 },
};

#define NUM_TIMEOUT_THREADS ARRAY_SIZE(timeouts)
static K_THREAD_STACK_ARRAY_DEFINE(timeout_stacks, NUM_TIMEOUT_THREADS,
				   THREAD_STACKSIZE);
static struct k_thread timeout_threads[NUM_TIMEOUT_THREADS];

/* a thread busy waits, then reports through a fifo */
static void test_busy_wait(void *mseconds, void *arg2, void *arg3)
{
	u32_t usecs;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	usecs = (int)mseconds * 1000;

	TC_PRINT("Thread busy waiting for %d usecs\n", usecs);
	k_busy_wait(usecs);
	TC_PRINT("Thread busy waiting completed\n");

	/*
	 * Ideally the test should verify that the correct number of ticks
	 * have elapsed. However, when running under QEMU, the tick interrupt
	 * may be processed on a very irregular basis, meaning that far
	 * fewer than the expected number of ticks may occur for a given
	 * number of clock cycles vs. what would ordinarily be expected.
	 *
	 * Consequently, the best we can do for now to test busy waiting is
	 * to invoke the API and verify that it returns. (If it takes way
	 * too long, or never returns, the main test task may be able to
	 * time out and report an error.)
	 */

	k_sem_give(&reply_timeout);
}

/* a thread sleeps and times out, then reports through a fifo */
static void test_thread_sleep(void *delta, void *arg2, void *arg3)
{
	s64_t timestamp;
	int timeout = (int)delta;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	TC_PRINT(" thread sleeping for %d milliseconds\n", timeout);
	timestamp = k_uptime_get();
	k_sleep(timeout);
	timestamp = k_uptime_get() - timestamp;
	TC_PRINT(" thread back from sleep\n");

	if (timestamp < timeout || timestamp > timeout + __ticks_to_ms(2)) {
		TC_ERROR("timestamp out of range, got %d\n", (int)timestamp);
		return;
	}

	k_sem_give(&reply_timeout);
}

/* a thread is started with a delay, then it reports that it ran via a fifo */
static void delayed_thread(void *num, void *arg2, void *arg3)
{
	struct timeout_order *timeout = &timeouts[(int)num];

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	TC_PRINT(" thread (q order: %d, t/o: %d) is running\n",
		 timeout->q_order, timeout->timeout);

	k_fifo_put(&timeout_order_fifo, timeout);
}

static int test_timeout(void)
{
	struct timeout_order *data;
	s32_t timeout;
	int rv;
	int i;

	/* test k_busy_wait() */
	TC_PRINT("Testing k_busy_wait()\n");
	timeout = 20;           /* in ms */

	k_thread_create(&timeout_threads[0], timeout_stacks[0],
			THREAD_STACKSIZE, test_busy_wait,
			(void *)(intptr_t) timeout, NULL,
			NULL, K_PRIO_COOP(THREAD_PRIORITY), 0, 0);

	rv = k_sem_take(&reply_timeout, timeout * 2);

	if (rv) {
		TC_ERROR(" *** task timed out waiting for " "k_busy_wait()\n");
		return TC_FAIL;
	}

	/* test k_sleep() */

	TC_PRINT("Testing k_sleep()\n");
	timeout = 50;

	k_thread_create(&timeout_threads[0], timeout_stacks[0],
			THREAD_STACKSIZE, test_thread_sleep,
			(void *)(intptr_t) timeout, NULL,
			NULL, K_PRIO_COOP(THREAD_PRIORITY), 0, 0);

	rv = k_sem_take(&reply_timeout, timeout * 2);
	if (rv) {
		TC_ERROR(" *** task timed out waiting for thread on "
			 "k_sleep().\n");
		return TC_FAIL;
	}

	/* test k_thread_create() without cancellation */
	TC_PRINT("Testing k_thread_create() without cancellation\n");

	for (i = 0; i < NUM_TIMEOUT_THREADS; i++) {
		k_thread_create(&timeout_threads[i], timeout_stacks[i],
				THREAD_STACKSIZE,
				delayed_thread,
				(void *)i,
				NULL, NULL,
				K_PRIO_COOP(5), 0, timeouts[i].timeout);
	}
	for (i = 0; i < NUM_TIMEOUT_THREADS; i++) {
		data = k_fifo_get(&timeout_order_fifo, 750);
		if (!data) {
			TC_ERROR
				(" *** timeout while waiting for delayed thread\n");
			return TC_FAIL;
		}

		if (data->timeout_order != i) {
			TC_ERROR(" *** wrong delayed thread ran (got %d, "
				 "expected %d)\n", data->timeout_order, i);
			return TC_FAIL;
		}

		TC_PRINT(" got thread (q order: %d, t/o: %d) as expected\n",
			 data->q_order, data->timeout);
	}

	/* ensure no more thread fire */
	data = k_fifo_get(&timeout_order_fifo, 750);

	if (data) {
		TC_ERROR(" *** got something unexpected in the fifo\n");
		return TC_FAIL;
	}

	/* test k_thread_create() with cancellation */
	TC_PRINT("Testing k_thread_create() with cancellations\n");

	int cancellations[] = { 0, 3, 4, 6 };
	int num_cancellations = ARRAY_SIZE(cancellations);
	int next_cancellation = 0;

	k_tid_t delayed_threads[NUM_TIMEOUT_THREADS];

	for (i = 0; i < NUM_TIMEOUT_THREADS; i++) {
		k_tid_t id;

		id = k_thread_create(&timeout_threads[i], timeout_stacks[i],
				     THREAD_STACKSIZE, delayed_thread,
				     (void *)i, NULL, NULL,
				     K_PRIO_COOP(5), 0, timeouts[i].timeout);

		delayed_threads[i] = id;
	}

	for (i = 0; i < NUM_TIMEOUT_THREADS; i++) {
		int j;

		if (i == cancellations[next_cancellation]) {
			TC_PRINT(" cancelling "
				 "[q order: %d, t/o: %d, t/o order: %d]\n",
				 timeouts[i].q_order, timeouts[i].timeout, i);

			for (j = 0; j < NUM_TIMEOUT_THREADS; j++) {
				if (timeouts[j].timeout_order == i) {
					break;
				}
			}

			if (j < NUM_TIMEOUT_THREADS) {
				k_thread_cancel(delayed_threads[j]);
				++next_cancellation;
				continue;
			}
		}

		data = k_fifo_get(&timeout_order_fifo, 2750);

		if (!data) {
			TC_ERROR
				(" *** timeout while waiting for delayed thread\n");
			return TC_FAIL;
		}

		if (data->timeout_order != i) {
			TC_ERROR(" *** wrong delayed thread ran (got %d, "
				 "expected %d)\n", data->timeout_order, i);
			return TC_FAIL;
		}

		TC_PRINT(" got (q order: %d, t/o: %d, t/o order %d) "
			 "as expected\n", data->q_order, data->timeout,
			 data->timeout_order);
	}

	if (num_cancellations != next_cancellation) {
		TC_ERROR(" *** wrong number of cancellations (expected %d, "
			 "got %d\n", num_cancellations, next_cancellation);
		return TC_FAIL;
	}

	/* ensure no more thread fire */
	data = k_fifo_get(&timeout_order_fifo, 750);
	if (data) {
		TC_ERROR(" *** got something unexpected in the fifo\n");
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 * @brief Entry point to timer tests
 *
 * This is the entry point to the CPU and thread tests.
 *
 * @return N/A
 */
void main(void)
{
	int rv;                 /* return value from tests */

	thread_detected_error = 0;
	thread_evidence = 0;

	TC_START("Test kernel CPU and thread routines");

	TC_PRINT("Initializing kernel objects\n");
	rv = kernel_init_objects();
	if (rv != TC_PASS) {
		goto tests_done;
	}

	TC_PRINT("Testing interrupt locking and unlocking\n");
	rv = test_kernel_interrupts(irq_lock_wrapper, irq_unlock_wrapper, -1);
	if (rv != TC_PASS) {
		goto tests_done;
	}
#ifdef TICK_IRQ
	/* Disable interrupts coming from the timer. */

	TC_PRINT("Testing irq_disable() and irq_enable()\n");
	rv = test_kernel_interrupts(irq_disable_wrapper, irq_enable_wrapper,
				    TICK_IRQ);
	if (rv != TC_PASS) {
		goto tests_done;
	}
#endif

	TC_PRINT("Testing some kernel context routines\n");
	rv = test_kernel_ctx_task();
	if (rv != TC_PASS) {
		goto tests_done;
	}

	TC_PRINT("Spawning a thread from a task\n");
	thread_evidence = 0;

	k_thread_create(&thread_data1, thread_stack1, THREAD_STACKSIZE,
			thread_entry, k_current_get(), NULL,
			NULL, K_PRIO_COOP(THREAD_PRIORITY), 0, 0);

	if (thread_evidence != 1) {
		rv = TC_FAIL;
		TC_ERROR("  - thread did not execute as expected!\n");
		goto tests_done;
	}

	/*
	 * The thread ran, now wake it so it can test k_current_get and
	 * k_is_in_isr.
	 */
	TC_PRINT("Thread to test k_current_get() and " "k_is_in_isr()\n");
	k_sem_give(&sem_thread);

	if (thread_detected_error != 0) {
		rv = TC_FAIL;
		TC_ERROR("  - failure detected in thread; "
			 "thread_detected_error = %d\n", thread_detected_error);
		goto tests_done;
	}

	TC_PRINT("Thread to test k_yield()\n");
	k_sem_give(&sem_thread);

	if (thread_detected_error != 0) {
		rv = TC_FAIL;
		TC_ERROR("  - failure detected in thread; "
			 "thread_detected_error = %d\n", thread_detected_error);
		goto tests_done;
	}

	k_sem_give(&sem_thread);

	rv = test_timeout();
	if (rv != TC_PASS) {
		goto tests_done;
	}

#ifdef HAS_POWERSAVE_INSTRUCTION
	TC_PRINT("Testing k_cpu_idle()\n");
	rv = test_kernel_cpu_idle(0);
	if (rv != TC_PASS) {
		goto tests_done;
	}
#ifndef CONFIG_ARM
	TC_PRINT("Testing k_cpu_atomic_idle()\n");
	rv = test_kernel_cpu_idle(1);
	if (rv != TC_PASS) {
		goto tests_done;
	}
#endif
#endif

tests_done:
	TC_END_RESULT(rv);
	TC_END_REPORT(rv);
}
