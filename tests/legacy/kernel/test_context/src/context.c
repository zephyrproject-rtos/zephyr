/* thread.c - test nanokernel CPU and thread APIs */

/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DESCRIPTION
 * This module tests the following CPU and thread related routines:
 * fiber_fiber_start(), task_fiber_start(), fiber_yield(),
 * sys_thread_self_get(), sys_execution_context_type_get(), nano_cpu_idle(),
 * irq_lock(), irq_unlock(),
 * irq_offload(), nanoCpuExcConnect(),
 * irq_enable(), irq_disable(),
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
#if !defined(CONFIG_CPU_CORTEX_M) && !defined(CONFIG_XTENSA)
  #include <board.h>
#endif

#define FIBER_STACKSIZE    (384 + CONFIG_TEST_EXTRA_STACKSIZE)
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
 * do not have a power saving instruction, so nano_cpu_idle()
 * returns immediately
 */
#if !defined(CONFIG_NIOS2) && \
	(!defined(CONFIG_RISCV32) || defined(CONFIG_RISCV_HAS_CPU_IDLE))
#define HAS_POWERSAVE_INSTRUCTION
#endif

typedef struct {
	int command;	/* command to process   */
	int error;	/* error value (if any) */
	union {
		void *data;	/* pointer to data to use or return */
		int value;	/* value to be passed or returned   */
	};
} ISR_INFO;

typedef int  (*disable_int_func)(int);
typedef void (*enable_int_func)(int);

static struct nano_sem   sem_fiber;
static struct nano_timer timer;
static struct nano_sem   reply_timeout;
struct nano_fifo         timeout_order_fifo;

static int fiber_detected_error;
static int fiber_evidence;

static char __stack fiber_stack1[FIBER_STACKSIZE];
static char __stack fiber_stack2[FIBER_STACKSIZE];

static ISR_INFO  isr_info;

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
		isr_info.data = (void *) sys_thread_self_get();
		break;

	case EXEC_CTX_TYPE_CMD:
		isr_info.value = sys_execution_context_type_get();
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
 * @brief Initialize nanokernel objects
 *
 * This routine initializes the nanokernel objects used in this module's tests.
 *
 * @return TC_PASS
 */
static int nano_init_objects(void)
{
	nano_sem_init(&sem_fiber);
	nano_sem_init(&reply_timeout);
	nano_timer_init(&timer, NULL);
	nano_fifo_init(&timeout_order_fifo);

	return TC_PASS;
}

#ifdef HAS_POWERSAVE_INSTRUCTION
/**
 *
 * @brief Test the nano_cpu_idle() routine
 *
 * This tests the nano_cpu_idle() routine.  The first thing it does is align to
 * a tick boundary.  The only source of interrupts while the test is running is
 * expected to be the tick clock timer which should wake the CPU.  Thus after
 * each call to nano_cpu_idle(), the tick count should be one higher.
 *
 * @return TC_PASS on success
 * @return TC_FAIL on failure
 */
static int test_nano_cpu_idle(void)
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
static int test_nano_interrupts(disable_int_func disable_int,
				enable_int_func enable_int, int irq)
{
	unsigned long long  count = 0;
	unsigned long long  i = 0;
	int tick;
	int tick2;
	int imask;

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

	imask = disable_int(irq);
	tick = sys_tick_get_32();
	for (i = 0; i < count; i++) {
		sys_tick_get_32();
	}

	tick2 = sys_tick_get_32();

	/*
	 * Re-enable interrupts before returning (for both success and failure
	 * cases).
	 */

	enable_int(imask);

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
 * @brief Test some nano context routines from a task
 *
 * This routines tests the sys_thread_self_get() and
 * sys_execution_context_type_get() routines from both a task and an ISR (that
 * interrupted a task). Checking those routines with fibers are done
 * elsewhere.
 *
 * @return TC_PASS on success
 * @return TC_FAIL on failure
 */
static int test_nano_ctx_task(void)
{
	nano_thread_id_t self_thread_id;

	TC_PRINT("Testing sys_thread_self_get() from an ISR and task\n");

	self_thread_id = sys_thread_self_get();
	isr_info.command = THREAD_SELF_CMD;
	isr_info.error = 0;
	/* isr_info is modified by the isr_handler routine */
	isr_handler_trigger();
	if (isr_info.error || isr_info.data != (void *)self_thread_id) {
		/*
		 * Either the ISR detected an error, or the ISR context ID
		 * does not match the interrupted task's thread ID.
		 */
		return TC_FAIL;
	}

	TC_PRINT("Testing sys_execution_context_type_get() from an ISR\n");
	isr_info.command = EXEC_CTX_TYPE_CMD;
	isr_info.error = 0;
	isr_handler_trigger();
	if (isr_info.error || isr_info.value != NANO_CTX_ISR) {
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
 * This routines tests the sys_thread_self_get and
 * sys_execution_context_type_get() routines from both a fiber and an ISR (that
 * interrupted a fiber).  Checking those routines with tasks are done
 * elsewhere.
 *
 * This routine may set <fiber_detected_error> to the following values:
 *   1 - if fiber ID matches that of the task
 *   2 - if thread ID taken during ISR does not match that of the fiber
 *   3 - sys_execution_context_type_get() when called from an ISR is not
 *       NANO_TYPE_ISR
 *   4 - sys_execution_context_type_get() when called from a fiber is not
 *       NANO_TYPE_FIBER
 *
 * @return TC_PASS on success
 * @return TC_FAIL on failure
 */
static int test_nano_fiber(nano_thread_id_t task_thread_id)
{
	nano_thread_id_t  self_thread_id;

	self_thread_id = sys_thread_self_get();
	if (self_thread_id == task_thread_id) {
		fiber_detected_error = 1;
		return TC_FAIL;
	}

	isr_info.command = THREAD_SELF_CMD;
	isr_info.error = 0;
	isr_handler_trigger();
	if (isr_info.error || isr_info.data != (void *)self_thread_id) {
		/*
		 * Either the ISR detected an error, or the ISR context ID
		 * does not match the interrupted fiber's thread ID.
		 */
		fiber_detected_error = 2;
		return TC_FAIL;
	}

	isr_info.command = EXEC_CTX_TYPE_CMD;
	isr_info.error = 0;
	isr_handler_trigger();
	if (isr_info.error || (isr_info.value != NANO_CTX_ISR)) {
		fiber_detected_error = 3;
		return TC_FAIL;
	}

	if (sys_execution_context_type_get() != NANO_CTX_FIBER) {
		fiber_detected_error = 4;
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
#define fiber_priority_set(fiber, new_prio) task_priority_set(fiber, new_prio)

static void fiber_helper(int arg1, int arg2)
{
	nano_thread_id_t  self_thread_id;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	/*
	 * This fiber starts off at a higher priority than fiber_entry().
	 * Thus, it should execute immediately.
	 */

	fiber_evidence++;

	/* Test that helper will yield to a fiber of equal priority */
	self_thread_id = sys_thread_self_get();

	/* Lower priority to that of fiber_entry() */
	fiber_priority_set(self_thread_id, self_thread_id->base.prio + 1);

	fiber_yield();        /* Yield to fiber of equal priority */

	fiber_evidence++;
	/* <fiber_evidence> should now be 2 */

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
 * On error, it may set <fiber_detected_error> to one of the following values:
 *   10 - helper fiber ran prematurely
 *   11 - fiber_yield() did not yield to a higher priority fiber
 *   12 - fiber_yield() did not yield to an equal prioirty fiber
 *   13 - fiber_yield() yielded to a lower priority fiber
 *
 * @return TC_PASS on success
 * @return TC_FAIL on failure
 */
static int test_fiber_yield(void)
{
	nano_thread_id_t  self_thread_id;

	/*
	 * Start a fiber of higher priority.  Note that since the new fiber is
	 * being started from a fiber, it will not automatically switch to the
	 * fiber as it would if done from a task.
	 */

	self_thread_id = sys_thread_self_get();
	fiber_evidence = 0;
	fiber_fiber_start(fiber_stack2, FIBER_STACKSIZE, fiber_helper,
		0, 0, FIBER_PRIORITY - 1, 0);

	if (fiber_evidence != 0) {
		/* ERROR! Helper spawned at higher */
		fiber_detected_error = 10;    /* priority ran prematurely. */
		return TC_FAIL;
	}

	/*
	 * Test that the fiber will yield to the higher priority helper.
	 * <fiber_evidence> is still 0.
	 */

	fiber_yield();

	if (fiber_evidence == 0) {
		/* ERROR! Did not yield to higher */
		fiber_detected_error = 11;    /* priority fiber. */
		return TC_FAIL;
	}

	if (fiber_evidence > 1) {
		/* ERROR! Helper did not yield to */
		fiber_detected_error = 12;    /* equal priority fiber. */
		return TC_FAIL;
	}

	/*
	 * Raise the priority of fiber_entry().  Calling fiber_yield() should
	 * not result in switching to the helper.
	 */

	fiber_priority_set(self_thread_id, self_thread_id->base.prio - 1);
	fiber_yield();

	if (fiber_evidence != 1) {
		/* ERROR! Context switched to a lower */
		fiber_detected_error = 13;    /* priority fiber! */
		return TC_FAIL;
	}

	/*
	 * Block on <sem_fiber>.  This will allow the helper fiber to complete.
	 * The main task will wake this fiber.
	 */

	nano_fiber_sem_take(&sem_fiber, TICKS_UNLIMITED);

	return TC_PASS;
}

/**
 * @brief Entry point to fiber started by the task
 *
 * This routine is the entry point to the fiber started by the task.
 *
 * @param task_thread_id	thread ID of the spawning task
 * @param arg1			unused
 *
 * @return N/A
 */
static void fiber_entry(int task_thread_id, int arg1)
{
	int rv;

	ARG_UNUSED(arg1);

	fiber_evidence++;    /* Prove to the task that the fiber has run */
	nano_fiber_sem_take(&sem_fiber, TICKS_UNLIMITED);

	rv = test_nano_fiber((nano_thread_id_t)task_thread_id);
	if (rv != TC_PASS) {
		return;
	}

	/* Allow the task to print any messages before the next test runs */
	nano_fiber_sem_take(&sem_fiber, TICKS_UNLIMITED);

	rv = test_fiber_yield();
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

struct timeout_order {
	void *link_in_fifo;
	int32_t timeout;
	int timeout_order;
	int q_order;
};

struct timeout_order timeouts[] = {
	{0, TIMEOUT(2), 2, 0},
	{0, TIMEOUT(4), 4, 1},
	{0, TIMEOUT(0), 0, 2},
	{0, TIMEOUT(1), 1, 3},
	{0, TIMEOUT(5), 5, 4},
	{0, TIMEOUT(6), 6, 5},
	{0, TIMEOUT(3), 3, 6},
};

#define NUM_TIMEOUT_FIBERS ARRAY_SIZE(timeouts)
static char __stack timeout_stacks[NUM_TIMEOUT_FIBERS][FIBER_STACKSIZE];

/* a fiber busy waits, then reports through a fifo */
static void test_busy_wait(int ticks, int unused)
{
	uint32_t usecs;

	ARG_UNUSED(unused);

	usecs = ticks * sys_clock_us_per_tick;

	TC_PRINT("Fiber busy waiting for %d usecs (%d ticks)\n", usecs, ticks);
	sys_thread_busy_wait(usecs);
	TC_PRINT("Fiber busy waiting completed\n");

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

	nano_fiber_sem_give(&reply_timeout);
}

/* a fiber sleeps and times out, then reports through a fifo */
static void test_fiber_sleep(int timeout, int unused)
{
	int64_t orig_ticks = sys_tick_get();

	ARG_UNUSED(unused);

	TC_PRINT(" fiber sleeping for %d ticks\n", timeout);
	fiber_sleep(timeout);
	TC_PRINT(" fiber back from sleep\n");

	if (!is_timeout_in_range(orig_ticks, timeout)) {
		return;
	}

	nano_fiber_sem_give(&reply_timeout);
}

/* a fiber is started with a delay, then it reports that it ran via a fifo */
static void delayed_fiber(int num, int unused)
{
	struct timeout_order *timeout = &timeouts[num];

	ARG_UNUSED(unused);

	TC_PRINT(" fiber (q order: %d, t/o: %d) is running\n",
		 timeout->q_order, timeout->timeout);

	nano_fiber_fifo_put(&timeout_order_fifo, timeout);
}

static int test_timeout(void)
{
	struct timeout_order *data;
	int32_t timeout;
	int rv;
	int i;

	/* test sys_thread_busy_wait() */
	TC_PRINT("Testing sys_thread_busy_wait()\n");
	timeout = 2;
	task_fiber_start(timeout_stacks[0], FIBER_STACKSIZE, test_busy_wait,
			 (int)timeout, 0, FIBER_PRIORITY, 0);

	rv = nano_task_sem_take(&reply_timeout, timeout + 2);
	if (!rv) {
		TC_ERROR(" *** task timed out waiting for "
			 "sys_thread_busy_wait()\n");
		return TC_FAIL;
	}

	/* test fiber_sleep() */

	TC_PRINT("Testing fiber_sleep()\n");
	timeout = 5;
	task_fiber_start(timeout_stacks[0], FIBER_STACKSIZE, test_fiber_sleep,
			 (int)timeout, 0, FIBER_PRIORITY, 0);

	rv = nano_task_sem_take(&reply_timeout, timeout + 5);
	if (!rv) {
		TC_ERROR(" *** task timed out waiting for fiber on "
			 "fiber_sleep().\n");
		return TC_FAIL;
	}

	/* test fiber_delayed_start() without cancellation */
	TC_PRINT("Testing fiber_delayed_start() without cancellation\n");

	for (i = 0; i < NUM_TIMEOUT_FIBERS; i++) {
		task_fiber_delayed_start(timeout_stacks[i], FIBER_STACKSIZE,
					 delayed_fiber, i, 0, 5, 0,
					 timeouts[i].timeout);
	}
	for (i = 0; i < NUM_TIMEOUT_FIBERS; i++) {
		data = nano_task_fifo_get(&timeout_order_fifo,
					  TIMEOUT_TWO_INTERVALS);
		if (!data) {
			TC_ERROR(" *** timeout while waiting for delayed fiber\n");
			return TC_FAIL;
		}

		if (data->timeout_order != i) {
			TC_ERROR(" *** wrong delayed fiber ran (got %d, "
				 "expected %d)\n", data->timeout_order, i);
			return TC_FAIL;
		}

		TC_PRINT(" got fiber (q order: %d, t/o: %d) as expected\n",
			 data->q_order, data->timeout);
	}

	/* ensure no more fibers fire */
	data = nano_task_fifo_get(&timeout_order_fifo, TIMEOUT_TWO_INTERVALS);

	if (data) {
		TC_ERROR(" *** got something unexpected in the fifo\n");
		return TC_FAIL;
	}

	/* test fiber_delayed_start() with cancellation */
	TC_PRINT("Testing fiber_delayed_start() with cancellations\n");

	int cancellations[] = {0, 3, 4, 6};
	int num_cancellations = ARRAY_SIZE(cancellations);
	int next_cancellation = 0;

	nano_thread_id_t delayed_fibers[NUM_TIMEOUT_FIBERS];

	for (i = 0; i < NUM_TIMEOUT_FIBERS; i++) {
		nano_thread_id_t id;

		id = task_fiber_delayed_start(timeout_stacks[i],
					      FIBER_STACKSIZE, delayed_fiber, i,
					      0, 5, 0, timeouts[i].timeout);
		delayed_fibers[i] = id;
	}

	for (i = 0; i < NUM_TIMEOUT_FIBERS; i++) {
		int j;

		if (i == cancellations[next_cancellation]) {
			TC_PRINT(" cancelling "
				 "[q order: %d, t/o: %d, t/o order: %d]\n",
				 timeouts[i].q_order, timeouts[i].timeout, i);

			for (j = 0; j < NUM_TIMEOUT_FIBERS; j++) {
				if (timeouts[j].timeout_order == i) {
					break;
				}
			}

			if (j == NUM_TIMEOUT_FIBERS) {
				TC_ERROR(" *** array overrun: all timeout order values should have been between the boundaries\n");
				return TC_FAIL;
			}

			task_fiber_delayed_start_cancel(delayed_fibers[j]);
			++next_cancellation;
			continue;
		}

		data = nano_task_fifo_get(&timeout_order_fifo,
					  TIMEOUT_TEN_INTERVALS);

		if (!data) {
			TC_ERROR(" *** timeout while waiting for delayed fiber\n");
			return TC_FAIL;
		}

		if (data->timeout_order != i) {
			TC_ERROR(" *** wrong delayed fiber ran (got %d, "
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

	/* ensure no more fibers fire */
	data = nano_task_fifo_get(&timeout_order_fifo, TIMEOUT_TWO_INTERVALS);
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
	int rv;       /* return value from tests */

	fiber_detected_error = 0;
	fiber_evidence = 0;

	TC_START("Test Nanokernel CPU and thread routines");

	TC_PRINT("Initializing nanokernel objects\n");
	rv = nano_init_objects();
	if (rv != TC_PASS) {
		goto tests_done;
	}

#ifdef HAS_POWERSAVE_INSTRUCTION
	TC_PRINT("Testing nano_cpu_idle()\n");
	rv = test_nano_cpu_idle();
	if (rv != TC_PASS) {
		goto tests_done;
	}
#endif

	TC_PRINT("Testing interrupt locking and unlocking\n");
	rv = test_nano_interrupts(irq_lock_wrapper, irq_unlock_wrapper, -1);
	if (rv != TC_PASS) {
		goto tests_done;
	}

#ifdef TICK_IRQ
	/* Disable interrupts coming from the timer. */

	TC_PRINT("Testing irq_disable() and irq_enable()\n");
	rv = test_nano_interrupts(irq_disable_wrapper, irq_enable_wrapper,
				  TICK_IRQ);
	if (rv != TC_PASS) {
		goto tests_done;
	}
#endif

	TC_PRINT("Testing some nano context routines\n");
	rv = test_nano_ctx_task();
	if (rv != TC_PASS) {
		goto tests_done;
	}

	TC_PRINT("Spawning a fiber from a task\n");
	fiber_evidence = 0;
	task_fiber_start(fiber_stack1, FIBER_STACKSIZE, fiber_entry,
			 (int)sys_thread_self_get(), 0, FIBER_PRIORITY, 0);

	if (fiber_evidence != 1) {
		rv = TC_FAIL;
		TC_ERROR("  - fiber did not execute as expected!\n");
		goto tests_done;
	}

	/*
	 * The fiber ran, now wake it so it can test sys_thread_self_get and
	 * sys_execution_context_type_get.
	 */
	TC_PRINT("Fiber to test sys_thread_self_get() and "
		 "sys_execution_context_type_get\n");
	nano_task_sem_give(&sem_fiber);

	if (fiber_detected_error != 0) {
		rv = TC_FAIL;
		TC_ERROR("  - failure detected in fiber; "
			 "fiber_detected_error = %d\n", fiber_detected_error);
		goto tests_done;
	}

	TC_PRINT("Fiber to test fiber_yield()\n");
	nano_task_sem_give(&sem_fiber);

	if (fiber_detected_error != 0) {
		rv = TC_FAIL;
		TC_ERROR("  - failure detected in fiber; "
			 "fiber_detected_error = %d\n", fiber_detected_error);
		goto tests_done;
	}

	nano_task_sem_give(&sem_fiber);

	rv = test_timeout();
	if (rv != TC_PASS) {
		goto tests_done;
	}

tests_done:
	TC_END_RESULT(rv);
	TC_END_REPORT(rv);
}
