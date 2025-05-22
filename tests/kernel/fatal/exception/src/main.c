/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/tc_util.h>
#include <zephyr/test_toolchain.h>
#include <zephyr/kernel_structs.h>
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
#elif defined(CONFIG_NIOS2)
	__asm__ volatile ("trap");
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
#elif defined(CONFIG_NIOS2)
	__asm__ volatile ("trap");
#elif defined(CONFIG_RISCV)
	/* In riscv architecture, use an undefined
	 * instruction to trigger illegal instruction on RISCV.
	 */
	__asm__ volatile ("unimp");
	/* In arc architecture, SWI instruction is used
	 * to trigger soft interrupt.
	 */
#elif defined(CONFIG_ARC)
	__asm__ volatile ("swi");
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

/**
 * @brief Test the kernel fatal error handling works correctly
 * @details Manually trigger the crash with various ways and check
 * that the kernel is handling that properly or not. Also the crash reason
 * should match. Check for stack sentinel feature by overflowing the
 * thread's stack and check for the exception.
 *
 * @ingroup kernel_fatal_tests
 */
ZTEST(fatal_exception, test_fatal)
{
	rv = TC_PASS;

	/*
	 * Main thread(test_main) priority was 10 but ztest thread runs at
	 * priority -1. To run the test smoothly make both main and ztest
	 * threads run at same priority level.
	 */
	k_thread_priority_set(_current, K_PRIO_PREEMPT(MAIN_PRIORITY));

#ifndef CONFIG_ARCH_POSIX
	TC_PRINT("test alt thread 1: generic CPU exception\n");
	k_thread_create(&alt_thread, alt_stack,
			K_THREAD_STACK_SIZEOF(alt_stack),
			entry_cpu_exception,
			NULL, NULL, NULL, K_PRIO_COOP(PRIORITY), 0,
			K_NO_WAIT);
	zassert_not_equal(rv, TC_FAIL, "thread was not aborted");

	TC_PRINT("test alt thread 1: generic CPU exception divide zero\n");
	k_thread_create(&alt_thread, alt_stack,
			K_THREAD_STACK_SIZEOF(alt_stack),
			entry_cpu_exception_extend,
			NULL, NULL, NULL, K_PRIO_COOP(PRIORITY), 0,
			K_NO_WAIT);
	zassert_not_equal(rv, TC_FAIL, "thread was not aborted");
#else
	/*
	 * We want the native OS to handle segfaults so we can debug it
	 * with the normal linux tools
	 */
	TC_PRINT("test alt thread 1: skipped for POSIX ARCH\n");
#endif

	TC_PRINT("test alt thread 2: initiate kernel oops\n");
	k_thread_create(&alt_thread, alt_stack,
			K_THREAD_STACK_SIZEOF(alt_stack),
			entry_oops,
			NULL, NULL, NULL, K_PRIO_COOP(PRIORITY), 0,
			K_NO_WAIT);
	k_thread_abort(&alt_thread);
	zassert_not_equal(rv, TC_FAIL, "thread was not aborted");

	TC_PRINT("test alt thread 3: initiate kernel panic\n");
	k_thread_create(&alt_thread, alt_stack,
			K_THREAD_STACK_SIZEOF(alt_stack),
			entry_panic,
			NULL, NULL, NULL, K_PRIO_COOP(PRIORITY), 0,
			K_NO_WAIT);
	k_thread_abort(&alt_thread);
	zassert_not_equal(rv, TC_FAIL, "thread was not aborted");

#if defined(CONFIG_ASSERT)
	/* This test shall be skip while ASSERT is off */
	TC_PRINT("test alt thread 4: fail assertion\n");
	k_thread_create(&alt_thread, alt_stack,
			K_THREAD_STACK_SIZEOF(alt_stack),
			entry_zephyr_assert,
			NULL, NULL, NULL, K_PRIO_COOP(PRIORITY), 0,
			K_NO_WAIT);
	k_thread_abort(&alt_thread);
	zassert_not_equal(rv, TC_FAIL, "thread was not aborted");
#endif

	TC_PRINT("test alt thread 5: initiate arbitrary SW exception\n");
	k_thread_create(&alt_thread, alt_stack,
			K_THREAD_STACK_SIZEOF(alt_stack),
			entry_arbitrary_reason,
			NULL, NULL, NULL, K_PRIO_COOP(PRIORITY), 0,
			K_NO_WAIT);
	k_thread_abort(&alt_thread);

	zassert_not_equal(rv, TC_FAIL, "thread was not aborted");
	TC_PRINT("test alt thread 6: initiate arbitrary SW exception negative\n");
	k_thread_create(&alt_thread, alt_stack,
			K_THREAD_STACK_SIZEOF(alt_stack),
			entry_arbitrary_reason_negative,
			NULL, NULL, NULL, K_PRIO_COOP(PRIORITY), 0,
			K_NO_WAIT);
	k_thread_abort(&alt_thread);
	zassert_not_equal(rv, TC_FAIL, "thread was not aborted");

#ifndef CONFIG_ARCH_POSIX

#ifdef CONFIG_STACK_SENTINEL
	TC_PRINT("test stack sentinel overflow - timer irq\n");
	check_stack_overflow(stack_sentinel_timer, 0);

	TC_PRINT("test stack sentinel overflow - swap\n");
	check_stack_overflow(stack_sentinel_swap, 0);
#endif /* CONFIG_STACK_SENTINEL */

#ifdef CONFIG_HW_STACK_PROTECTION
	/* HW based stack overflow detection.
	 * Do this twice to show that HW-based solutions work more than
	 * once.
	 */

	TC_PRINT("test stack HW-based overflow - supervisor 1\n");
	check_stack_overflow(stack_hw_overflow, 0);

	TC_PRINT("test stack HW-based overflow - supervisor 2\n");
	check_stack_overflow(stack_hw_overflow, 0);

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	TC_PRINT("test stack HW-based overflow (FPU thread) - supervisor 1\n");
	check_stack_overflow(stack_hw_overflow, K_FP_REGS);

	TC_PRINT("test stack HW-based overflow (FPU thread) - supervisor 2\n");
	check_stack_overflow(stack_hw_overflow, K_FP_REGS);
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

#ifdef CONFIG_USERSPACE

	TC_PRINT("test stack HW-based overflow - user 1\n");
	check_stack_overflow(stack_hw_overflow, K_USER);

	TC_PRINT("test stack HW-based overflow - user 2\n");
	check_stack_overflow(stack_hw_overflow, K_USER);

	TC_PRINT("test stack HW-based overflow - user priv stack 1\n");
	check_stack_overflow(user_priv_stack_hw_overflow, K_USER);

	TC_PRINT("test stack HW-based overflow - user priv stack 2\n");
	check_stack_overflow(user_priv_stack_hw_overflow, K_USER);

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	TC_PRINT("test stack HW-based overflow (FPU thread) - user 1\n");
	check_stack_overflow(stack_hw_overflow, K_USER | K_FP_REGS);

	TC_PRINT("test stack HW-based overflow (FPU thread) - user 2\n");
	check_stack_overflow(stack_hw_overflow, K_USER | K_FP_REGS);
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

#endif /* CONFIG_USERSPACE */

#endif /* CONFIG_HW_STACK_PROTECTION */

#endif /* !CONFIG_ARCH_POSIX */
}

static void *fatal_setup(void)
{
#if defined(CONFIG_DEMAND_PAGING) && \
	!defined(CONFIG_LINKER_GENERIC_SECTIONS_PRESENT_AT_BOOT)
	uintptr_t pin_addr;
	size_t pin_size, obj_size;

	/* Need to pin the whole stack object (including reserved
	 * space), or else it would cause double faults: exception
	 * being processed while page faults on the stacks.
	 *
	 * Same applies for some variables needed during exception
	 * processing.
	 */
#if defined(CONFIG_STACK_SENTINEL) && !defined(CONFIG_ARCH_POSIX)

	obj_size = K_THREAD_STACK_SIZEOF(overflow_stack);
#if defined(CONFIG_USERSPACE)
	obj_size = K_THREAD_STACK_LEN(obj_size);
#endif

	k_mem_region_align(&pin_addr, &pin_size,
			   POINTER_TO_UINT(&overflow_stack),
			   obj_size, CONFIG_MMU_PAGE_SIZE);

	k_mem_pin(UINT_TO_POINTER(pin_addr), pin_size);
#endif /* CONFIG_STACK_SENTINEL && !CONFIG_ARCH_POSIX */

	obj_size = K_THREAD_STACK_SIZEOF(alt_stack);
#if defined(CONFIG_USERSPACE)
	obj_size = K_THREAD_STACK_LEN(obj_size);
#endif

	k_mem_region_align(&pin_addr, &pin_size,
			   POINTER_TO_UINT(&alt_stack),
			   obj_size,
			   CONFIG_MMU_PAGE_SIZE);

	k_mem_pin(UINT_TO_POINTER(pin_addr), pin_size);

	k_mem_region_align(&pin_addr, &pin_size,
			   POINTER_TO_UINT((void *)&expected_reason),
			   sizeof(expected_reason),
			   CONFIG_MMU_PAGE_SIZE);

	k_mem_pin(UINT_TO_POINTER(pin_addr), pin_size);
#endif /* CONFIG_DEMAND_PAGING
	* && !CONFIG_LINKER_GENERIC_SECTIONS_PRESENT_AT_BOOT
	*/

	return NULL;
}

ZTEST_SUITE(fatal_exception, NULL, fatal_setup, NULL, NULL, NULL);
