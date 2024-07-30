/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @brief test context and thread APIs
 *
 * @defgroup kernel_context_tests Context Tests
 *
 * @ingroup all_tests
 *
 * This module tests the following CPU and thread related routines:
 * k_thread_create(), k_yield(), k_is_in_isr(),
 * k_current_get(), k_cpu_idle(), k_cpu_atomic_idle(),
 * irq_lock(), irq_unlock(),
 * irq_offload(), irq_enable(), irq_disable(),
 * @{
 * @}
 */

#include <stdlib.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/irq_offload.h>
#include <zephyr/sys_clock.h>

#if defined(CONFIG_SOC_POSIX)
/* TIMER_TICK_IRQ <soc.h> header for certain platforms */
#include <soc.h>
#endif

#define THREAD_STACKSIZE    (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define THREAD_STACKSIZE2   (384 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define THREAD_PRIORITY     4

#define THREAD_SELF_CMD    0
#define EXEC_CTX_TYPE_CMD  1

#define UNKNOWN_COMMAND    -1
#define INVALID_BEHAVIOUR  -2

/*
 * Get the timer type dependent IRQ number. If timer type
 * is not defined in platform, generate an error
 */

#if defined(CONFIG_APIC_TSC_DEADLINE_TIMER)
#define TICK_IRQ z_loapic_irq_base() /* first LVT interrupt */
#elif defined(CONFIG_CPU_CORTEX_M)
/*
 * The Cortex-M use the SYSTICK exception for the system timer, which is
 * not considered an IRQ by the irq_enable/Disable APIs.
 */
#elif defined(CONFIG_SPARC)
#elif defined(CONFIG_MIPS)
#elif defined(CONFIG_ARCH_POSIX)
#if  defined(CONFIG_BOARD_NATIVE_POSIX)
#define TICK_IRQ TIMER_TICK_IRQ
#else
/*
 * Other POSIX arch boards will skip the irq_disable() and irq_enable() test
 * unless TICK_IRQ is defined here for them
 */
#endif /* defined(CONFIG_ARCH_POSIX) */
#else

extern const int32_t z_sys_timer_irq_for_test;
#define TICK_IRQ (z_sys_timer_irq_for_test)

#endif

/* Cortex-M1 and Nios II do have a power saving instruction, so k_cpu_idle()
 * returns immediately
 */
#if !defined(CONFIG_CPU_CORTEX_M1) && !defined(CONFIG_NIOS2)
#define HAS_POWERSAVE_INSTRUCTION
#endif



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

static int thread_evidence;

static K_THREAD_STACK_DEFINE(thread_stack1, THREAD_STACKSIZE);
static K_THREAD_STACK_DEFINE(thread_stack2, THREAD_STACKSIZE);
static K_THREAD_STACK_DEFINE(thread_stack3, THREAD_STACKSIZE);
static struct k_thread thread_data1;
static struct k_thread thread_data2;
static struct k_thread thread_data3;

static ISR_INFO isr_info;

/**
 * @brief Handler to perform various actions from within an ISR context
 *
 * This routine is the ISR handler for isr_handler_trigger(). It performs
 * the command requested in <isr_info.command>.
 */
static void isr_handler(const void *data)
{
	ARG_UNUSED(data);

	if (k_can_yield()) {
		isr_info.error = INVALID_BEHAVIOUR;
	}

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
 */
static void kernel_init_objects(void)
{
	k_sem_init(&reply_timeout, 0, UINT_MAX);
	k_timer_init(&timer, NULL, NULL);
	k_fifo_init(&timeout_order_fifo);
}

/**
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
 * @brief A wrapper for irq_unlock()
 */
void irq_unlock_wrapper(int imask)
{
	irq_unlock(imask);
}

/**
 * @brief A wrapper for irq_disable()
 *
 * @return @a irq
 */
int irq_disable_wrapper(int irq)
{
	irq_disable(irq);
	return irq;
}

/**
 * @brief A wrapper for irq_enable()
 */
void irq_enable_wrapper(int irq)
{
	irq_enable(irq);
}

#if defined(HAS_POWERSAVE_INSTRUCTION)
#if defined(CONFIG_TICKLESS_KERNEL)
static struct k_timer idle_timer;

static volatile bool idle_timer_done;

static void idle_timer_expiry_function(struct k_timer *timer_id)
{
	k_timer_stop(&idle_timer);
	idle_timer_done = true;
}

static void _test_kernel_cpu_idle(int atomic)
{
	uint64_t t0, dt;
	unsigned int i, key;
	uint32_t dur = k_ms_to_ticks_ceil32(10);
	uint32_t slop = 1 + k_ms_to_ticks_ceil32(1);
	int idle_loops;

	/* Set up a time to trigger events to exit idle mode */
	k_timer_init(&idle_timer, idle_timer_expiry_function, NULL);

	for (i = 0; i < 5; i++) {
		k_usleep(1);
		t0 = k_uptime_ticks();
		idle_loops = 0;
		idle_timer_done = false;
		k_timer_start(&idle_timer, K_TICKS(dur), K_NO_WAIT);
		key = irq_lock();
		do {
			if (atomic) {
				k_cpu_atomic_idle(key);
			} else {
				k_cpu_idle();
			}
		} while ((idle_loops++ < CONFIG_MAX_IDLE_WAKES) && (idle_timer_done == false));
		zassert_true(idle_timer_done,
			     "The CPU was waken spuriously too many times (%d > %d)",
			     idle_loops, CONFIG_MAX_IDLE_WAKES);
		dt = k_uptime_ticks() - t0;
		zassert_true(abs((int32_t) (dt - dur)) <= slop,
			     "Inaccurate wakeup, idled for %d ticks, expected %d",
			     dt, dur);
	}
}

#else /* CONFIG_TICKLESS_KERNEL */
static void _test_kernel_cpu_idle(int atomic)
{
	int tms, tms2;
	int i;

	/* Align to a "ms boundary". */
	tms = k_uptime_get_32();
	while (tms == k_uptime_get_32()) {
		Z_SPIN_DELAY(50);
	}

	tms = k_uptime_get_32();
	for (i = 0; i < 5; i++) { /* Repeat the test five times */
		if (atomic) {
			unsigned int key = irq_lock();

			k_cpu_atomic_idle(key);
		} else {
			k_cpu_idle();
		}
		/* calculating milliseconds per tick*/
		tms += k_ticks_to_ms_floor64(1);
		tms2 = k_uptime_get_32();
		zassert_false(tms2 < tms, "Bad ms per tick value computed,"
			      "got %d which is less than %d\n",
			      tms2, tms);
	}
}
#endif /* CONFIG_TICKLESS_KERNEL */

/**
 * @brief Test cpu idle function
 *
 * @details
 * Test Objective:
 * - The kernel architecture provide an idle function to be run when the system
 *   has no work for the current CPU
 * - This routine tests the k_cpu_atomic_idle() routine
 *
 * Testing techniques
 * - Functional and black box testing
 * - Interface testing
 *
 * Prerequisite Condition:
 * - HAS_POWERSAVE_INSTRUCTION is set
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Record system time before cpu enters idle state
 * -# Enter cpu idle state by k_cpu_atomic_idle()
 * -# Record system time after cpu idle state is interrupted
 * -# Compare the two system time values.
 *
 * Expected Test Result:
 * - cpu enters idle state for a given time
 *
 * Pass/Fail criteria:
 * - Success if the cpu enters idle state, failure otherwise.
 *
 * Assumptions and Constraints
 * - N/A
 *
 * @see k_cpu_atomic_idle()
 * @ingroup kernel_context_tests
 */
ZTEST(context_cpu_idle, test_cpu_idle_atomic)
{
#if defined(CONFIG_ARM) || defined(CONFIG_ARM64)
	ztest_test_skip();
#else
	_test_kernel_cpu_idle(1);
#endif
}

/**
 * @brief Test cpu idle function
 *
 * @details
 * Test Objective:
 * - The kernel architecture provide an idle function to be run when the system
 *   has no work for the current CPU
 * - This routine tests the k_cpu_idle() routine
 *
 * Testing techniques
 * - Functional and black box testing
 * - Interface testing
 *
 * Prerequisite Condition:
 * - HAS_POWERSAVE_INSTRUCTION is set
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Record system time before cpu enters idle state
 * -# Enter cpu idle state by k_cpu_idle()
 * -# Record system time after cpu idle state is interrupted
 * -# Compare the two system time values.
 *
 * Expected Test Result:
 * - cpu enters idle state for a given time
 *
 * Pass/Fail criteria:
 * - Success if the cpu enters idle state, failure otherwise.
 *
 * Assumptions and Constraints
 * - N/A
 *
 * @see k_cpu_idle()
 * @ingroup kernel_context_tests
 */
ZTEST(context_cpu_idle, test_cpu_idle)
{
	_test_kernel_cpu_idle(0);
}

#else /* HAS_POWERSAVE_INSTRUCTION */
ZTEST(context_cpu_idle, test_cpu_idle)
{
	ztest_test_skip();
}
ZTEST(context_cpu_idle, test_cpu_idle_atomic)
{
	ztest_test_skip();
}
#endif

static void _test_kernel_interrupts(disable_int_func disable_int,
				    enable_int_func enable_int, int irq)
{
	unsigned long long count = 1ull;
	unsigned long long i = 0;
	int tick;
	int tick2;
	int imask;

	/* Align to a "tick boundary" */
	tick = sys_clock_tick_get_32();
	while (sys_clock_tick_get_32() == tick) {
		Z_SPIN_DELAY(1000);
	}

	tick++;
	while (sys_clock_tick_get_32() == tick) {
		Z_SPIN_DELAY(1000);
		count++;
	}

	/*
	 * Inflate <count> so that when we loop later, many ticks should have
	 * elapsed during the loop. This later loop will not exactly match the
	 * previous loop, but it should be close enough in structure that when
	 * combined with the inflated count, many ticks will have passed.
	 */

	count <<= 4;

	imask = disable_int(irq);
	tick = sys_clock_tick_get_32();
	for (i = 0; i < count; i++) {
		sys_clock_tick_get_32();
		Z_SPIN_DELAY(1000);
	}

	tick2 = sys_clock_tick_get_32();

	/*
	 * Re-enable interrupts before returning (for both success and failure
	 * cases).
	 */
	enable_int(imask);

	/* In TICKLESS, current time is retrieved from a hardware
	 * counter and ticks DO advance with interrupts locked!
	 */
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		zassert_equal(tick2, tick,
			      "tick advanced with interrupts locked");
	}

	/* Now repeat with interrupts unlocked. */
	for (i = 0; i < count; i++) {
		sys_clock_tick_get_32();
		Z_SPIN_DELAY(1000);
	}

	tick2 = sys_clock_tick_get_32();
	zassert_not_equal(tick, tick2,
			  "tick didn't advance as expected");
}

/**
 * @brief Test routines for disabling and enabling interrupts
 *
 * @ingroup kernel_context_tests
 *
 * @details
 * Test Objective:
 * - To verify kernel architecture layer shall provide a mechanism to
 *   selectively disable and enable specific numeric interrupts.
 * - This routine tests the routines for disabling and enabling interrupts.
 *   These include irq_lock() and irq_unlock().
 *
 * Testing techniques:
 * - Interface testing, function and black box testing,
 *   dynamic analysis and testing
 *
 * Prerequisite Conditions:
 * - CONFIG_TICKLESS_KERNEL is not set.
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Do action to align to a tick boundary.
 * -# Left shift 4 bits for the value of counts.
 * -# Call irq_lock() and restore its return value to imask.
 * -# Call sys_clock_tick_get_32() and store its return value to tick.
 * -# Repeat counts of calling sys_clock_tick_get_32().
 * -# Call sys_clock_tick_get_32() and store its return value to tick2.
 * -# Call irq_unlock() with parameter imask.
 * -# Check if tick is equal to tick2.
 * -# Repeat counts of calling sys_clock_tick_get_32().
 * -# Call sys_clock_tick_get_32() and store its return value to tick2.
 * -# Check if tick is NOT equal to tick2.
 *
 * Expected Test Result:
 * - The ticks shall not increase while interrupt locked.
 *
 * Pass/Fail Criteria:
 * - Successful if check points in test procedure are all passed, otherwise
 *   failure.
 *
 * Assumptions and Constraints:
 * - N/A
 *
 * @see irq_lock(), irq_unlock()
 */
ZTEST(context, test_interrupts)
{
	/* IRQ locks don't prevent ticks from advancing in tickless mode */
	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		ztest_test_skip();
	}

	_test_kernel_interrupts(irq_lock_wrapper, irq_unlock_wrapper, -1);
}

/**
 * @brief Test routines for disabling and enabling interrupts (disable timer)
 *
 * @ingroup kernel_context_tests
 *
 * @details
 * Test Objective:
 * - To verify the kernel architecture layer shall provide a mechanism to
 *   simultaneously mask all local CPU interrupts and return the previous mask
 *   state for restoration.
 * - This routine tests the routines for disabling and enabling interrupts.
 *   These include irq_disable() and irq_enable().
 *
 * Testing techniques:
 * - Interface testing, function and black box testing,
 *   dynamic analysis and testing
 *
 * Prerequisite Conditions:
 * - TICK_IRQ is defined.
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Do action to align to a tick boundary.
 * -# Left shift 4 bit for the value of counts.
 * -# Call irq_disable() and restore its return value to imask.
 * -# Call sys_clock_tick_get_32() and store its return value to tick.
 * -# Repeat counts of calling sys_clock_tick_get_32().
 * -# Call sys_clock_tick_get_32() and store its return value to tick2.
 * -# Call irq_enable() with parameter imask.
 * -# Check if tick is equal to tick2.
 * -# Repeat counts of calling sys_clock_tick_get_32().
 * -# Call sys_clock_tick_get_32() and store its return value to tick2.
 * -# Check if tick is NOT equal to tick2.
 *
 * Expected Test Result:
 * - The ticks shall not increase while interrupt locked.
 *
 * Pass/Fail Criteria:
 * - Successful if check points in test procedure are all passed, otherwise
 *   failure.
 *
 * Assumptions and Constraints:
 * - Note that this test works by disabling the timer interrupt
 *   directly, without any interaction with the timer driver or
 *   timeout subsystem.  NOT ALL ARCHITECTURES will latch and deliver
 *   a timer interrupt that arrives while the interrupt is disabled,
 *   which means that the timeout list will become corrupted (because
 *   it contains items that should have expired in the past).  Any use
 *   of kernel timeouts after completion of this test is disallowed.
 *   RUN THIS TEST LAST IN THE SUITE.
 *
 * @see irq_disable(), irq_enable()
 */
ZTEST(context_one_cpu, test_timer_interrupts)
{
#if (defined(TICK_IRQ) && defined(CONFIG_TICKLESS_KERNEL))
	/* Disable interrupts coming from the timer. */
	_test_kernel_interrupts(irq_disable_wrapper, irq_enable_wrapper, TICK_IRQ);
#else
	ztest_test_skip();
#endif
}

/**
 * @brief Test some context routines
 *
 * @details
 * Test Objective:
 * - Thread context handles derived from context switches must be able to be
 *   restored upon interrupt exit
 *
 * Testing techniques
 * - Functional and black box testing
 * - Interface testing
 *
 * Prerequisite Condition:
 * - N/A
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Set priority of current thread to 0 as a preemptible thread
 * -# Trap to interrupt context, get thread id of the interrupted thread and
 *  pass back to that thread.
 * -# Return to thread context and make sure this context is interrupted by
 *  comparing its thread ID and the thread ID passed by isr.
 * -# Pass command to isr to check whether the isr is executed in interrupt
 *  context
 * -# When return to thread context, check the return value of command.
 *
 * Expected Test Result:
 * - Thread context restored upon interrupt exit
 *
 * Pass/Fail criteria:
 * - Success if context of thread restored correctly, failure otherwise.
 *
 * Assumptions and Constraints
 * - N/A
 *
 * @ingroup kernel_context_tests
 * @see k_current_get(), k_is_in_isr()
 */
ZTEST(context, test_ctx_thread)
{
	k_tid_t self_thread_id;

	k_thread_priority_set(k_current_get(), 0);

	TC_PRINT("Testing k_current_get() from an ISR and thread\n");

	self_thread_id = k_current_get();
	isr_info.command = THREAD_SELF_CMD;
	isr_info.error = 0;
	/* isr_info is modified by the isr_handler routine */
	isr_handler_trigger();

	zassert_false(isr_info.error, "ISR detected an error");

	zassert_equal(isr_info.data, (void *)self_thread_id,
		      "ISR context ID mismatch");

	TC_PRINT("Testing k_is_in_isr() from an ISR\n");
	isr_info.command = EXEC_CTX_TYPE_CMD;
	isr_info.error = 0;
	isr_handler_trigger();

	zassert_false(isr_info.error, "ISR detected an error");

	zassert_equal(isr_info.value, K_ISR,
		      "isr_info.value was not K_ISR");

	TC_PRINT("Testing k_is_in_isr() from a preemptible thread\n");
	zassert_false(k_is_in_isr(), "Should not be in ISR context");

	zassert_false(_current->base.prio < 0,
		      "Current thread should have preemptible priority: %d",
		      _current->base.prio);

}

/**
 * @brief Test the various context/thread routines from a cooperative thread
 *
 * This routines tests the k_current_get() and k_is_in_isr() routines from both
 * a thread and an ISR (that interrupted a cooperative thread). Checking those
 * routines with preemptible threads are done elsewhere.
 *
 * @see k_current_get(), k_is_in_isr()
 */
static void _test_kernel_thread(k_tid_t _thread_id)
{
	k_tid_t self_thread_id;

	self_thread_id = k_current_get();
	zassert_true((self_thread_id != _thread_id), "thread id matches parent thread");

	isr_info.command = THREAD_SELF_CMD;
	isr_info.error = 0;
	isr_handler_trigger();
	/*
	 * Either the ISR detected an error, or the ISR context ID
	 * does not match the interrupted thread's ID.
	 */
	zassert_false((isr_info.error || (isr_info.data != (void *)self_thread_id)),
		      "Thread ID taken during ISR != calling thread");

	isr_info.command = EXEC_CTX_TYPE_CMD;
	isr_info.error = 0;
	isr_handler_trigger();
	zassert_false((isr_info.error || (isr_info.value != K_ISR)),
		      "k_is_in_isr() when called from an ISR is false");

	zassert_false(k_is_in_isr(), "k_is_in_isr() when called from a thread is true");

	zassert_false((_current->base.prio >= 0),
		      "thread is not a cooperative thread");
}

/**
 *
 * @brief Entry point to the thread's helper
 *
 * This routine is the entry point to the thread's helper thread. It is used to
 * help test the behavior of the k_yield() routine.
 *
 * @param arg1    unused
 * @param arg2    unused
 * @param arg3    unused
 *
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

	k_yield();      /* Yield to thread of equal priority */

	thread_evidence++;
	/* thread_evidence should now be 2 */

}

/**
 * @brief Entry point to thread started by another thread
 *
 * This routine is the entry point to the thread started by the thread.
 */
static void k_yield_entry(void *arg0, void *arg1, void *arg2)
{
	k_tid_t self_thread_id;

	ARG_UNUSED(arg0);
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	thread_evidence++;      /* Prove that the thread has run */
	k_sem_take(&sem_thread, K_FOREVER);

	/*
	 * Start a thread of higher priority. Note that since the new thread is
	 * being started from a thread, it will not automatically switch to the
	 * thread as it would if done from another thread.
	 */
	self_thread_id = k_current_get();
	thread_evidence = 0;

	k_thread_create(&thread_data2, thread_stack2, THREAD_STACKSIZE,
			thread_helper, NULL, NULL, NULL,
			K_PRIO_COOP(THREAD_PRIORITY - 1), 0, K_NO_WAIT);

	zassert_equal(thread_evidence, 0,
		      "Helper created at higher priority ran prematurely.");

	/*
	 * Validate the thread is allowed to yield
	 */
	zassert_true(k_can_yield(), "Thread incorrectly detected it could not yield");

	/*
	 * Test that the thread will yield to the higher priority helper.
	 * thread_evidence is still 0.
	 */
	k_yield();

	zassert_not_equal(thread_evidence, 0,
			  "k_yield() did not yield to a higher priority thread: %d",
			  thread_evidence);

	zassert_false((thread_evidence > 1),
		      "k_yield() did not yield to an equal priority thread: %d",
		      thread_evidence);

	/*
	 * Raise the priority of thread_entry(). Calling k_yield() should
	 * not result in switching to the helper.
	 */
	k_thread_priority_set(self_thread_id, self_thread_id->base.prio - 1);
	k_yield();

	zassert_equal(thread_evidence, 1,
		      "k_yield() yielded to a lower priority thread");

	/*
	 * Block on sem_thread. This will allow the helper thread to
	 * complete. The main thread will wake this thread.
	 */
	k_sem_take(&sem_thread, K_FOREVER);
}

static void kernel_thread_entry(void *_thread_id, void *arg1, void *arg2)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	thread_evidence++;      /* Prove that the thread has run */
	k_sem_take(&sem_thread, K_FOREVER);

	_test_kernel_thread((k_tid_t) _thread_id);

}

/*
 * @brief Timeout tests
 *
 * Test the k_sleep() API, as well as the k_thread_create() ones.
 */
struct timeout_order {
	void *link_in_fifo;
	int32_t timeout;
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
				   THREAD_STACKSIZE2);
static struct k_thread timeout_threads[NUM_TIMEOUT_THREADS];

/* a thread busy waits */
static void busy_wait_thread(void *mseconds, void *arg2, void *arg3)
{
	uint32_t usecs;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	usecs = POINTER_TO_INT(mseconds) * 1000;

	k_busy_wait(usecs);

	/* FIXME: Broken on Nios II, see #22956 */
#ifndef CONFIG_NIOS2
	int key = arch_irq_lock();

	k_busy_wait(usecs);
	arch_irq_unlock(key);
#endif

	/*
	 * Ideally the test should verify that the correct number of ticks
	 * have elapsed. However, when running under QEMU, the tick interrupt
	 * may be processed on a very irregular basis, meaning that far
	 * fewer than the expected number of ticks may occur for a given
	 * number of clock cycles vs. what would ordinarily be expected.
	 *
	 * Consequently, the best we can do for now to test busy waiting is
	 * to invoke the API and verify that it returns. (If it takes way
	 * too long, or never returns, the main test thread may be able to
	 * time out and report an error.)
	 */

	k_sem_give(&reply_timeout);
}

/* a thread sleeps and times out, then reports through a fifo */
static void thread_sleep(void *delta, void *arg2, void *arg3)
{
	int64_t timestamp;
	int timeout = POINTER_TO_INT(delta);

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	timestamp = k_uptime_get();
	k_msleep(timeout);
	timestamp = k_uptime_get() - timestamp;

	int slop = MAX(k_ticks_to_ms_floor64(2), 1);

	if (timestamp < timeout || timestamp > timeout + slop) {
		TC_ERROR("timestamp out of range, got %d\n", (int)timestamp);
		return;
	}

	k_sem_give(&reply_timeout);
}

/* a thread is started with a delay, then it reports that it ran via a fifo */
static void delayed_thread(void *num, void *arg2, void *arg3)
{
	struct timeout_order *timeout = &timeouts[POINTER_TO_INT(num)];

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	TC_PRINT(" thread (q order: %d, t/o: %d) is running\n",
		 timeout->q_order, timeout->timeout);

	k_fifo_put(&timeout_order_fifo, timeout);
}

/**
 * @brief Test timeouts
 *
 * @ingroup kernel_context_tests
 *
 * @see k_busy_wait(), k_sleep()
 */
ZTEST(context_one_cpu, test_busy_wait)
{
	int32_t timeout;
	int rv;

	timeout = 20;           /* in ms */

	k_thread_create(&timeout_threads[0], timeout_stacks[0],
			THREAD_STACKSIZE2, busy_wait_thread,
			INT_TO_POINTER(timeout), NULL,
			NULL, K_PRIO_COOP(THREAD_PRIORITY), 0, K_NO_WAIT);

	rv = k_sem_take(&reply_timeout, K_MSEC(timeout * 2 * 2));

	zassert_false(rv, " *** thread timed out waiting for " "k_busy_wait()");
}

/**
 * @brief Test timeouts
 *
 * @ingroup kernel_context_tests
 *
 * @see k_sleep()
 */
ZTEST(context_one_cpu, test_k_sleep)
{
	struct timeout_order *data;
	int32_t timeout;
	int rv;
	int i;


	timeout = 50;

	k_thread_create(&timeout_threads[0], timeout_stacks[0],
			THREAD_STACKSIZE2, thread_sleep,
			INT_TO_POINTER(timeout), NULL,
			NULL, K_PRIO_COOP(THREAD_PRIORITY), 0, K_NO_WAIT);

	rv = k_sem_take(&reply_timeout, K_MSEC(timeout * 2));
	zassert_equal(rv, 0, " *** thread timed out waiting for thread on "
		      "k_sleep().");

	/* test k_thread_create() without cancellation */
	TC_PRINT("Testing k_thread_create() without cancellation\n");

	for (i = 0; i < NUM_TIMEOUT_THREADS; i++) {
		k_thread_create(&timeout_threads[i], timeout_stacks[i],
				THREAD_STACKSIZE2,
				delayed_thread,
				INT_TO_POINTER(i), NULL, NULL,
				K_PRIO_COOP(5), 0,
				K_MSEC(timeouts[i].timeout));
	}
	for (i = 0; i < NUM_TIMEOUT_THREADS; i++) {
		data = k_fifo_get(&timeout_order_fifo, K_MSEC(750));
		zassert_not_null(data, " *** timeout while waiting for"
				 " delayed thread");

		zassert_equal(data->timeout_order, i,
			      " *** wrong delayed thread ran (got %d, "
			      "expected %d)\n", data->timeout_order, i);

		TC_PRINT(" got thread (q order: %d, t/o: %d) as expected\n",
			 data->q_order, data->timeout);
	}

	/* ensure no more thread fire */
	data = k_fifo_get(&timeout_order_fifo, K_MSEC(750));

	zassert_false(data, " *** got something unexpected in the fifo");

	/* test k_thread_create() with cancellation */
	TC_PRINT("Testing k_thread_create() with cancellations\n");

	int cancellations[] = { 0, 3, 4, 6 };
	int num_cancellations = ARRAY_SIZE(cancellations);
	int next_cancellation = 0;

	k_tid_t delayed_threads[NUM_TIMEOUT_THREADS];

	for (i = 0; i < NUM_TIMEOUT_THREADS; i++) {
		k_tid_t id;

		id = k_thread_create(&timeout_threads[i], timeout_stacks[i],
				     THREAD_STACKSIZE2, delayed_thread,
				     INT_TO_POINTER(i), NULL, NULL,
				     K_PRIO_COOP(5), 0,
				     K_MSEC(timeouts[i].timeout));

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
				k_thread_abort(delayed_threads[j]);
				++next_cancellation;
				continue;
			}
		}

		data = k_fifo_get(&timeout_order_fifo, K_MSEC(2750));

		zassert_not_null(data, " *** timeout while waiting for"
				 " delayed thread");

		zassert_equal(data->timeout_order, i,
			      " *** wrong delayed thread ran (got %d, "
			      "expected %d)\n", data->timeout_order, i);

		TC_PRINT(" got (q order: %d, t/o: %d, t/o order %d) "
			 "as expected\n", data->q_order, data->timeout,
			 data->timeout_order);
	}

	zassert_equal(num_cancellations, next_cancellation,
		      " *** wrong number of cancellations (expected %d, "
		      "got %d\n", num_cancellations, next_cancellation);

	/* ensure no more thread fire */
	data = k_fifo_get(&timeout_order_fifo, K_MSEC(750));
	zassert_false(data, " *** got something unexpected in the fifo");

}

/**
 *
 * @brief Test the k_yield() routine
 *
 * @ingroup kernel_context_tests
 *
 * Tests the k_yield() routine. It starts another thread
 * (thus also testing k_thread_create()) and checks that behavior of
 * k_yield() against the a higher priority thread,
 * a lower priority thread, and another thread of equal priority.
 *
 * @see k_yield()
 */
ZTEST(context_one_cpu, test_k_yield)
{
	thread_evidence = 0;
	k_thread_priority_set(k_current_get(), 0);

	k_sem_init(&sem_thread, 0, UINT_MAX);

	k_thread_create(&thread_data1, thread_stack1, THREAD_STACKSIZE,
			k_yield_entry, NULL, NULL,
			NULL, K_PRIO_COOP(THREAD_PRIORITY), 0, K_NO_WAIT);

	zassert_equal(thread_evidence, 1,
		      "Thread did not execute as expected!: %d", thread_evidence);

	k_sem_give(&sem_thread);
	k_sem_give(&sem_thread);
	k_sem_give(&sem_thread);
}

/**
 * @brief Test kernel thread creation
 *
 * @ingroup kernel_context_tests
 *
 * @see k_thread_create
 */

ZTEST(context_one_cpu, test_thread)
{

	k_thread_create(&thread_data3, thread_stack3, THREAD_STACKSIZE,
			kernel_thread_entry, NULL, NULL,
			NULL, K_PRIO_COOP(THREAD_PRIORITY), 0, K_NO_WAIT);

}

static void *context_setup(void)
{
	kernel_init_objects();

	return NULL;
}

ZTEST_SUITE(context_cpu_idle, NULL, context_setup, NULL, NULL, NULL);

ZTEST_SUITE(context, NULL, context_setup, NULL, NULL, NULL);

ZTEST_SUITE(context_one_cpu, NULL, context_setup,
		ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
