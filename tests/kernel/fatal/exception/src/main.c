/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/tc_util.h>
#include <zephyr/test_toolchain.h>
#include <zephyr/irq_offload.h>
#include <kswap.h>
#include <assert.h>

#if defined(CONFIG_USERSPACE)
#include <zephyr/kernel/mm.h>
#include <zephyr/internal/syscall_handler.h>
#include "test_syscalls.h"
#endif

#if defined(CONFIG_DEMAND_PAGING)
#include <zephyr/kernel/mm/demand_paging.h>
#endif

#if defined(CONFIG_X86) && defined(CONFIG_X86_MMU)
#define STACKSIZE (8192)
#else
#define  STACKSIZE (2048 + CONFIG_TEST_EXTRA_STACK_SIZE)
#endif
#define MAIN_PRIORITY 7
#define PRIORITY 5

static K_THREAD_STACK_DEFINE(alt_stack, STACKSIZE);

#if defined(CONFIG_STACK_SENTINEL) && !defined(CONFIG_ARCH_POSIX)
#define OVERFLOW_STACKSIZE (STACKSIZE / 2)
static k_thread_stack_t *overflow_stack =
		alt_stack + (STACKSIZE - OVERFLOW_STACKSIZE);
#else
#if defined(CONFIG_USERSPACE) && defined(CONFIG_ARC)
/* for ARC, privilege stack is merged into defined stack */
#define OVERFLOW_STACKSIZE (STACKSIZE + CONFIG_PRIVILEGED_STACK_SIZE)
#else
#define OVERFLOW_STACKSIZE STACKSIZE
#endif
#endif

static struct k_thread alt_thread;
volatile int rv;

static ZTEST_DMEM volatile int expected_reason = -1;

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *pEsf)
{
	TC_PRINT("Caught system error -- reason %d\n", reason);

	if (expected_reason == -1) {
		printk("Was not expecting a crash\n");
		TC_END_REPORT(TC_FAIL);
		k_fatal_halt(reason);
	}

	if (k_current_get() != &alt_thread) {
		printk("Wrong thread crashed\n");
		TC_END_REPORT(TC_FAIL);
		k_fatal_halt(reason);
	}

	if (reason != expected_reason) {
		printk("Wrong crash type got %d expected %d\n", reason,
		       expected_reason);
		TC_END_REPORT(TC_FAIL);
		k_fatal_halt(reason);
	}

	expected_reason = -1;
}

void entry_cpu_exception(void *p1, void *p2, void *p3)
{
	expected_reason = K_ERR_CPU_EXCEPTION;

#if defined(CONFIG_X86)
	__asm__ volatile ("ud2");
#elif defined(CONFIG_ARC)
	__asm__ volatile ("swi");
#elif defined(CONFIG_RISCV)
	/* Illegal instruction on RISCV. */
	__asm__ volatile (".word 0x77777777");
#else
	/* Triggers usage fault on ARM, illegal instruction on
	 * xtensa, TLB exception (instruction fetch) on MIPS.
	 */
	{
		volatile long illegal = 0;
		((void(*)(void))&illegal)();
	}
#endif
	rv = TC_FAIL;
}

void entry_cpu_exception_extend(void *p1, void *p2, void *p3)
{
	expected_reason = K_ERR_CPU_EXCEPTION;

#if defined(CONFIG_ARM64)
	__asm__ volatile ("svc 0");
#elif defined(CONFIG_CPU_AARCH32_CORTEX_R) || defined(CONFIG_CPU_AARCH32_CORTEX_A)
	__asm__ volatile ("udf #0");
#elif defined(CONFIG_CPU_CORTEX_M)
	__asm__ volatile ("udf #0");
#elif defined(CONFIG_RX)
	__asm__ volatile ("brk");
#elif defined(CONFIG_SOC_FAMILY_MAX32_RV32)
	/* The MAX32 RV32 core does not trap on writes to
	 * non-existent CSRs, so use a different illegal instruction
	 * for this test.
	 */
	__asm__ volatile (".word 0");
#elif defined(CONFIG_RISCV)
	/* In riscv architecture, use an undefined
	 * instruction to trigger illegal instruction on RISCV.
	 */
	__asm__ volatile ("unimp");
#elif defined(CONFIG_ARC)
	/* In arc architecture, SWI instruction is used
	 * to trigger soft interrupt.
	 */
	__asm__ volatile ("swi");
#elif defined(CONFIG_OPENRISC)
	__asm__ volatile ("l.trap 0");
#else
	/* used to create a divide by zero error on X86 and MIPS */
	volatile int error;
	volatile int zero = 0;

	error = 32;     /* avoid static checker uninitialized warnings */
	error = error / zero;
#endif
	rv = TC_FAIL;
}

void entry_oops(void *p1, void *p2, void *p3)
{
	expected_reason = K_ERR_KERNEL_OOPS;

	k_oops();
	TC_ERROR("SHOULD NEVER SEE THIS\n");
	rv = TC_FAIL;
}

void entry_panic(void *p1, void *p2, void *p3)
{
	expected_reason = K_ERR_KERNEL_PANIC;

	k_panic();
	TC_ERROR("SHOULD NEVER SEE THIS\n");
	rv = TC_FAIL;
}

void entry_zephyr_assert(void *p1, void *p2, void *p3)
{
	expected_reason = K_ERR_KERNEL_PANIC;

	__ASSERT(0, "intentionally failed assertion");
	rv = TC_FAIL;
}

void entry_arbitrary_reason(void *p1, void *p2, void *p3)
{
	expected_reason = INT_MAX;

	z_except_reason(INT_MAX);
	TC_ERROR("SHOULD NEVER SEE THIS\n");
	rv = TC_FAIL;
}

void entry_arbitrary_reason_negative(void *p1, void *p2, void *p3)
{
	expected_reason = -2;

	z_except_reason(-2);
	TC_ERROR("SHOULD NEVER SEE THIS\n");
	rv = TC_FAIL;
}

#ifndef CONFIG_ARCH_POSIX
#ifdef CONFIG_STACK_SENTINEL
__no_optimization void blow_up_stack(void)
{
	char buf[OVERFLOW_STACKSIZE];

	expected_reason = K_ERR_STACK_CHK_FAIL;
	TC_PRINT("posting %zu bytes of junk to stack...\n", sizeof(buf));
	(void)memset(buf, 0xbb, sizeof(buf));
}
#else
/* stack sentinel doesn't catch it in time before it trashes the entire kernel
 */

TOOLCHAIN_DISABLE_WARNING(TOOLCHAIN_WARNING_PRAGMAS)
TOOLCHAIN_DISABLE_WARNING(TOOLCHAIN_WARNING_INFINITE_RECURSION)

__no_optimization int stack_smasher(int val)
{
	return stack_smasher(val * 2) + stack_smasher(val * 3);
}

TOOLCHAIN_ENABLE_WARNING(TOOLCHAIN_WARNING_PRAGMAS)
TOOLCHAIN_ENABLE_WARNING(TOOLCHAIN_WARNING_INFINITE_RECURSION)

void blow_up_stack(void)
{
	expected_reason = K_ERR_STACK_CHK_FAIL;

	stack_smasher(37);
}

#if defined(CONFIG_USERSPACE)

void z_impl_blow_up_priv_stack(void)
{
	blow_up_stack();
}

static inline void z_vrfy_blow_up_priv_stack(void)
{
	z_impl_blow_up_priv_stack();
}
#include <zephyr/syscalls/blow_up_priv_stack_mrsh.c>

#endif /* CONFIG_USERSPACE */
#endif /* CONFIG_STACK_SENTINEL */

void stack_sentinel_timer(void *p1, void *p2, void *p3)
{
	/* We need to guarantee that we receive an interrupt, so set a
	 * k_timer and spin until we die.  Spinning alone won't work
	 * on a tickless kernel.
	 */
	static struct k_timer timer;

	blow_up_stack();
	k_timer_init(&timer, NULL, NULL);
	k_timer_start(&timer, K_MSEC(1), K_NO_WAIT);
	while (true) {
	}
}

void stack_sentinel_swap(void *p1, void *p2, void *p3)
{
	/* Test that stack overflow check due to swap works */
	blow_up_stack();
	TC_PRINT("swapping...\n");
	z_swap_unlocked();
	TC_ERROR("should never see this\n");
	rv = TC_FAIL;
}

void stack_hw_overflow(void *p1, void *p2, void *p3)
{
	/* Test that HW stack overflow check works */
	blow_up_stack();
	TC_ERROR("should never see this\n");
	rv = TC_FAIL;
}

#if defined(CONFIG_USERSPACE)
void user_priv_stack_hw_overflow(void *p1, void *p2, void *p3)
{
	/* Test that HW stack overflow check works
	 * on a user thread's privilege stack.
	 */
	blow_up_priv_stack();
	TC_ERROR("should never see this\n");
	rv = TC_FAIL;
}
#endif /* CONFIG_USERSPACE */

void check_stack_overflow(k_thread_entry_t handler, uint32_t flags)
{
#ifdef CONFIG_STACK_SENTINEL
	/* When testing stack sentinel feature, the overflow stack is a
	 * smaller section of alt_stack near the end.
	 * In this way when it gets overflowed by blow_up_stack() we don't
	 * corrupt anything else and prevent the test case from completing.
	 */
	k_thread_create(&alt_thread, overflow_stack, OVERFLOW_STACKSIZE,
#else
	k_thread_create(&alt_thread, alt_stack,
			K_THREAD_STACK_SIZEOF(alt_stack),
#endif /* CONFIG_STACK_SENTINEL */
			handler,
			NULL, NULL, NULL, K_PRIO_PREEMPT(PRIORITY), flags,
			K_NO_WAIT);

	zassert_not_equal(rv, TC_FAIL, "thread was not aborted");
}
#endif /* !CONFIG_ARCH_POSIX */

/*
 * Spawn an alternate thread whose entry point triggers a fatal condition and
 * wait for the offending thread to be terminated. Triggers that return to the
 * scheduler before the thread is reaped (k_oops()/k_panic()/__ASSERT()/
 * z_except_reason()) are joined with an explicit k_thread_abort(); CPU
 * exceptions terminate the cooperative thread inline, so they pass abort=false.
 */
static void run_crash_thread(k_thread_entry_t entry, bool abort)
{
	k_thread_create(&alt_thread, alt_stack, K_THREAD_STACK_SIZEOF(alt_stack),
			entry, NULL, NULL, NULL, K_PRIO_COOP(PRIORITY), 0,
			K_NO_WAIT);

	if (abort) {
		k_thread_abort(&alt_thread);
	}

	zassert_not_equal(rv, TC_FAIL, "thread was not aborted");
}

#ifndef CONFIG_ARCH_POSIX
/*
 * CPU exceptions are skipped on the POSIX arch on purpose: we want the native
 * OS to handle segfaults so the test can be debugged with normal Linux tools.
 */

/**
 * @brief Verify a CPU exception in a thread is caught by the default handler.
 *
 * @details
 * An alternate thread executes an illegal instruction (architecture specific),
 * which raises a CPU exception. The kernel's default exception handler reports
 * K_ERR_CPU_EXCEPTION to the installed fatal error handler, which validates the
 * reason code, and the offending thread is terminated while the test thread
 * resumes.
 *
 * Test steps:
 * - Create a cooperative thread whose entry point triggers a CPU exception.
 * - Let the default exception handler run and deliver the reason code.
 *
 * Expected result:
 * - The handler is invoked with K_ERR_CPU_EXCEPTION and the offending thread is
 *   terminated (rv is not TC_FAIL).
 *
 * @see k_sys_fatal_error_handler()
 * @ingroup kernel_fatal_tests
 * @verifies ZEP-SRS-16-1
 * @verifies ZEP-SRS-16-7
 * @verifies ZEP-SRS-16-8
 */
ZTEST(fatal_exception, test_cpu_exception)
{
	TC_PRINT("test generic CPU exception\n");
	run_crash_thread(entry_cpu_exception, false);
}

/**
 * @brief Verify an alternate CPU exception trigger is caught by the handler.
 *
 * @details
 * Same contract as test_cpu_exception() but using a second architecture
 * specific trigger (e.g. divide-by-zero on x86/MIPS, a distinct illegal
 * instruction elsewhere). This widens coverage of the default exception path
 * across the trigger mechanisms a given architecture provides.
 *
 * Test steps:
 * - Create a cooperative thread whose entry point triggers a CPU exception via
 *   the alternate mechanism.
 *
 * Expected result:
 * - The handler is invoked with K_ERR_CPU_EXCEPTION and the offending thread is
 *   terminated (rv is not TC_FAIL).
 *
 * @see k_sys_fatal_error_handler()
 * @ingroup kernel_fatal_tests
 * @verifies ZEP-SRS-16-1
 * @verifies ZEP-SRS-16-7
 * @verifies ZEP-SRS-16-8
 */
ZTEST(fatal_exception, test_cpu_exception_extended)
{
	TC_PRINT("test generic CPU exception (alternate trigger)\n");
	run_crash_thread(entry_cpu_exception_extend, false);
}
#endif /* !CONFIG_ARCH_POSIX */

/**
 * @brief Verify k_oops() triggers a fatal error terminating the current thread.
 *
 * @details
 * An alternate thread calls k_oops(). This is the mechanism for code to flag a
 * fatal error in the current context. The fatal handler receives
 * K_ERR_KERNEL_OOPS and the offending thread is terminated, demonstrating both
 * the trigger mechanism and that the application-supplied handler is consulted.
 *
 * Test steps:
 * - Create a thread whose entry point calls k_oops().
 * - Abort/join the thread and confirm it crashed as expected.
 *
 * Expected result:
 * - The handler is invoked with K_ERR_KERNEL_OOPS and the offending thread is
 *   terminated (rv is not TC_FAIL).
 *
 * @see k_oops()
 * @see k_sys_fatal_error_handler()
 * @ingroup kernel_fatal_tests
 * @verifies ZEP-SRS-16-4
 * @verifies ZEP-SRS-16-5
 * @verifies ZEP-SRS-16-7
 * @verifies ZEP-SRS-16-8
 */
ZTEST(fatal_exception, test_oops)
{
	TC_PRINT("test kernel oops\n");
	run_crash_thread(entry_oops, true);
}

/**
 * @brief Verify k_panic() triggers a fatal system error.
 *
 * @details
 * An alternate thread calls k_panic(), the mechanism for code to signal an
 * unrecoverable system error. The fatal handler receives K_ERR_KERNEL_PANIC.
 * Because the test installs its own handler that returns, the offending thread
 * is terminated and the test resumes.
 *
 * Test steps:
 * - Create a thread whose entry point calls k_panic().
 * - Abort/join the thread and confirm it crashed as expected.
 *
 * Expected result:
 * - The handler is invoked with K_ERR_KERNEL_PANIC and the offending thread is
 *   terminated (rv is not TC_FAIL).
 *
 * @see k_panic()
 * @see k_sys_fatal_error_handler()
 * @ingroup kernel_fatal_tests
 * @verifies ZEP-SRS-16-4
 * @verifies ZEP-SRS-16-6
 * @verifies ZEP-SRS-16-7
 * @verifies ZEP-SRS-16-8
 */
ZTEST(fatal_exception, test_panic)
{
	TC_PRINT("test kernel panic\n");
	run_crash_thread(entry_panic, true);
}

#if defined(CONFIG_ASSERT)
/**
 * @brief Verify a failed __ASSERT() is handled as a fatal error.
 *
 * @details
 * A failed kernel assertion must be routed to the default fatal error handling
 * path with no dedicated handler of its own. An alternate thread triggers a
 * failing __ASSERT(); the fatal handler receives K_ERR_KERNEL_PANIC and the
 * offending thread is terminated. Only built when CONFIG_ASSERT is enabled.
 *
 * Test steps:
 * - Create a thread whose entry point fails a __ASSERT().
 * - Abort/join the thread and confirm it crashed as expected.
 *
 * Expected result:
 * - The handler is invoked with K_ERR_KERNEL_PANIC and the offending thread is
 *   terminated (rv is not TC_FAIL).
 *
 * @see k_sys_fatal_error_handler()
 * @ingroup kernel_fatal_tests
 * @verifies ZEP-SRS-16-2
 * @verifies ZEP-SRS-16-7
 * @verifies ZEP-SRS-16-8
 */
ZTEST(fatal_exception, test_assert_fail)
{
	TC_PRINT("test failed assertion\n");
	run_crash_thread(entry_zephyr_assert, true);
}
#endif /* CONFIG_ASSERT */

/**
 * @brief Verify an arbitrary positive reason code is propagated to the handler.
 *
 * @details
 * An alternate thread raises a fatal error with an application-defined reason
 * (INT_MAX) via z_except_reason(). A reason code that has no dedicated handler
 * must still reach the default fatal handler unchanged, confirming the reason
 * code identifies the error to the handler.
 *
 * Test steps:
 * - Create a thread that calls z_except_reason() with a positive reason.
 * - Abort/join the thread and confirm it crashed as expected.
 *
 * Expected result:
 * - The handler is invoked with the exact reason code supplied and the
 *   offending thread is terminated (rv is not TC_FAIL).
 *
 * @see k_sys_fatal_error_handler()
 * @ingroup kernel_fatal_tests
 * @verifies ZEP-SRS-16-2
 * @verifies ZEP-SRS-16-7
 * @verifies ZEP-SRS-16-8
 */
ZTEST(fatal_exception, test_except_reason_positive)
{
	TC_PRINT("test arbitrary SW exception (positive reason)\n");
	run_crash_thread(entry_arbitrary_reason, true);
}

/**
 * @brief Verify an arbitrary negative reason code is propagated to the handler.
 *
 * @details
 * Companion to test_except_reason_positive() using a negative reason (-2) to
 * confirm the reason code is delivered verbatim regardless of sign and is
 * handled by the default fatal path.
 *
 * Test steps:
 * - Create a thread that calls z_except_reason() with a negative reason.
 * - Abort/join the thread and confirm it crashed as expected.
 *
 * Expected result:
 * - The handler is invoked with the exact reason code supplied and the
 *   offending thread is terminated (rv is not TC_FAIL).
 *
 * @see k_sys_fatal_error_handler()
 * @ingroup kernel_fatal_tests
 * @verifies ZEP-SRS-16-2
 * @verifies ZEP-SRS-16-7
 * @verifies ZEP-SRS-16-8
 */
ZTEST(fatal_exception, test_except_reason_negative)
{
	TC_PRINT("test arbitrary SW exception (negative reason)\n");
	run_crash_thread(entry_arbitrary_reason_negative, true);
}

#if defined(CONFIG_STACK_SENTINEL) && !defined(CONFIG_HW_SHADOW_STACK) && \
	!defined(CONFIG_ARCH_POSIX)
/**
 * @brief Verify the stack sentinel catches an overflow detected at a timer IRQ.
 *
 * @details
 * The software stack sentinel is checked on context switches and on interrupt
 * exit. A thread overflows its stack and then relies on a timer interrupt to
 * force a sentinel check; the corrupted sentinel raises a fatal
 * K_ERR_STACK_CHK_FAIL handled by the default path, terminating the thread.
 *
 * Test steps:
 * - Create a thread that overflows its stack and spins until a timer IRQ fires.
 *
 * Expected result:
 * - The fatal handler is invoked with K_ERR_STACK_CHK_FAIL and the offending
 *   thread is terminated (rv is not TC_FAIL).
 *
 * @see k_sys_fatal_error_handler()
 * @ingroup kernel_fatal_tests
 * @verifies ZEP-SRS-16-2
 * @verifies ZEP-SRS-16-8
 */
ZTEST(fatal_exception, test_stack_sentinel_timer)
{
	TC_PRINT("test stack sentinel overflow - timer irq\n");
	check_stack_overflow(stack_sentinel_timer, 0);
}

/**
 * @brief Verify the stack sentinel catches an overflow detected at swap.
 *
 * @details
 * Companion to test_stack_sentinel_timer() that forces the sentinel check via
 * an explicit context switch (swap) instead of an interrupt, covering the
 * second point at which the sentinel is validated.
 *
 * Test steps:
 * - Create a thread that overflows its stack and then swaps out.
 *
 * Expected result:
 * - The fatal handler is invoked with K_ERR_STACK_CHK_FAIL and the offending
 *   thread is terminated (rv is not TC_FAIL).
 *
 * @see k_sys_fatal_error_handler()
 * @ingroup kernel_fatal_tests
 * @verifies ZEP-SRS-16-2
 * @verifies ZEP-SRS-16-8
 */
ZTEST(fatal_exception, test_stack_sentinel_swap)
{
	TC_PRINT("test stack sentinel overflow - swap\n");
	check_stack_overflow(stack_sentinel_swap, 0);
}
#endif /* CONFIG_STACK_SENTINEL && !CONFIG_HW_SHADOW_STACK && !CONFIG_ARCH_POSIX */

#if defined(CONFIG_HW_STACK_PROTECTION) && !defined(CONFIG_ARCH_POSIX)
/**
 * @brief Verify HW stack protection catches a supervisor stack overflow.
 *
 * @details
 * Hardware stack protection raises a CPU exception when a thread writes past
 * its stack bounds. A supervisor thread overflows its stack; the resulting
 * fault is reported to the fatal handler and the thread is terminated. The
 * scenario is run twice to show HW-based detection works on more than the
 * first overflow.
 *
 * Test steps:
 * - Overflow a supervisor thread's stack and confirm it is terminated.
 * - Repeat once more.
 *
 * Expected result:
 * - Both overflows fault into the fatal handler and terminate the offending
 *   thread (rv is not TC_FAIL).
 *
 * @see k_sys_fatal_error_handler()
 * @ingroup kernel_fatal_tests
 * @verifies ZEP-SRS-16-1
 * @verifies ZEP-SRS-16-8
 */
ZTEST(fatal_exception, test_stack_hw_overflow)
{
	TC_PRINT("test stack HW-based overflow - supervisor 1\n");
	check_stack_overflow(stack_hw_overflow, 0);

	TC_PRINT("test stack HW-based overflow - supervisor 2\n");
	check_stack_overflow(stack_hw_overflow, 0);
}

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
/**
 * @brief Verify HW stack protection catches an overflow in an FPU thread.
 *
 * @details
 * Same as test_stack_hw_overflow() but the offending thread uses the FPU
 * (K_FP_REGS), exercising the overflow-detection path for threads with an
 * extended (floating-point) context. Run twice to show repeatability.
 *
 * Test steps:
 * - Overflow an FPU supervisor thread's stack and confirm it is terminated.
 * - Repeat once more.
 *
 * Expected result:
 * - Both overflows fault into the fatal handler and terminate the offending
 *   thread (rv is not TC_FAIL).
 *
 * @see k_sys_fatal_error_handler()
 * @ingroup kernel_fatal_tests
 * @verifies ZEP-SRS-16-1
 * @verifies ZEP-SRS-16-8
 */
ZTEST(fatal_exception, test_stack_hw_overflow_fpu)
{
	TC_PRINT("test stack HW-based overflow (FPU thread) - supervisor 1\n");
	check_stack_overflow(stack_hw_overflow, K_FP_REGS);

	TC_PRINT("test stack HW-based overflow (FPU thread) - supervisor 2\n");
	check_stack_overflow(stack_hw_overflow, K_FP_REGS);
}
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

#ifdef CONFIG_USERSPACE
/**
 * @brief Verify HW stack protection catches a user thread stack overflow.
 *
 * @details
 * A user-mode thread (K_USER) overflows its stack; HW stack protection must
 * fault and route to the fatal handler, terminating the thread. Run twice to
 * show repeatability.
 *
 * Test steps:
 * - Overflow a user thread's stack and confirm it is terminated.
 * - Repeat once more.
 *
 * Expected result:
 * - Both overflows fault into the fatal handler and terminate the offending
 *   thread (rv is not TC_FAIL).
 *
 * @see k_sys_fatal_error_handler()
 * @ingroup kernel_fatal_tests
 * @verifies ZEP-SRS-16-1
 * @verifies ZEP-SRS-16-8
 */
ZTEST(fatal_exception, test_stack_hw_overflow_user)
{
	TC_PRINT("test stack HW-based overflow - user 1\n");
	check_stack_overflow(stack_hw_overflow, K_USER);

	TC_PRINT("test stack HW-based overflow - user 2\n");
	check_stack_overflow(stack_hw_overflow, K_USER);
}

/**
 * @brief Verify HW stack protection catches a user privilege-stack overflow.
 *
 * @details
 * A user thread overflows its privilege stack (the supervisor-mode stack used
 * while servicing system calls) via a dedicated syscall. HW stack protection
 * must detect the overflow of the privilege stack and route to the fatal
 * handler, terminating the thread. Run twice to show repeatability.
 *
 * Test steps:
 * - From a user thread, overflow the privilege stack through a system call.
 * - Repeat once more.
 *
 * Expected result:
 * - Both overflows fault into the fatal handler and terminate the offending
 *   thread (rv is not TC_FAIL).
 *
 * @see k_sys_fatal_error_handler()
 * @ingroup kernel_fatal_tests
 * @verifies ZEP-SRS-16-1
 * @verifies ZEP-SRS-16-8
 */
ZTEST(fatal_exception, test_stack_hw_overflow_user_priv)
{
	TC_PRINT("test stack HW-based overflow - user priv stack 1\n");
	check_stack_overflow(user_priv_stack_hw_overflow, K_USER);

	TC_PRINT("test stack HW-based overflow - user priv stack 2\n");
	check_stack_overflow(user_priv_stack_hw_overflow, K_USER);
}

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
/**
 * @brief Verify HW stack protection catches an overflow in a user FPU thread.
 *
 * @details
 * Combines the user-mode and FPU contexts (K_USER | K_FP_REGS): a user thread
 * with an extended floating-point context overflows its stack and must be
 * faulted by HW stack protection and terminated. Run twice to show
 * repeatability.
 *
 * Test steps:
 * - Overflow a user FPU thread's stack and confirm it is terminated.
 * - Repeat once more.
 *
 * Expected result:
 * - Both overflows fault into the fatal handler and terminate the offending
 *   thread (rv is not TC_FAIL).
 *
 * @see k_sys_fatal_error_handler()
 * @ingroup kernel_fatal_tests
 * @verifies ZEP-SRS-16-1
 * @verifies ZEP-SRS-16-8
 */
ZTEST(fatal_exception, test_stack_hw_overflow_user_fpu)
{
	TC_PRINT("test stack HW-based overflow (FPU thread) - user 1\n");
	check_stack_overflow(stack_hw_overflow, K_USER | K_FP_REGS);

	TC_PRINT("test stack HW-based overflow (FPU thread) - user 2\n");
	check_stack_overflow(stack_hw_overflow, K_USER | K_FP_REGS);
}
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */
#endif /* CONFIG_USERSPACE */
#endif /* CONFIG_HW_STACK_PROTECTION && !CONFIG_ARCH_POSIX */

/*
 * Prepare each test case: reset the pass/fail flag and align the test thread's
 * priority with MAIN_PRIORITY. The ztest thread runs at priority -1, which is
 * higher than the alternate crash threads; lowering it here (on the same thread
 * that runs the test) lets those threads preempt and crash deterministically.
 * This must be done per test because each case runs on its own ztest thread.
 */
static void fatal_before(void *fixture)
{
	ARG_UNUSED(fixture);

	rv = TC_PASS;
	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(MAIN_PRIORITY));
}

ZTEST_SUITE(fatal_exception, NULL, NULL, fatal_before, NULL, NULL);
