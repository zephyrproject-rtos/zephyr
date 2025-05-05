/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/tc_util.h>
#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>
#include <assert.h>

static ZTEST_DMEM volatile int expected_reason = -1;

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *pEsf)
{
	int rv = TC_PASS;

	TC_PRINT("Caught system error -- reason %d\n", reason);
	if (reason != expected_reason) {
		TC_PRINT("Unexpected reason (exp: %d)\n", expected_reason);
		rv = TC_FAIL;
	}
	TC_END_RESULT_CUSTOM(rv, "test_fatal");
	TC_END_REPORT(rv);
	arch_system_halt(reason);
}

static void entry_cpu_exception(void)
{
	expected_reason = K_ERR_CPU_EXCEPTION;

	TC_PRINT("cpu exception\n");
#if defined(CONFIG_X86)
	__asm__ volatile ("ud2");
#elif defined(CONFIG_NIOS2)
	__asm__ volatile ("trap");
#elif defined(CONFIG_ARC)
	__asm__ volatile ("swi");
#else
	/* Triggers usage fault on ARM, illegal instruction on RISCV
	 * and xtensa
	 */
	{
		volatile long illegal = 0;
		((void(*)(void))&illegal)();
	}
#endif
}

static void entry_oops(void)
{
	unsigned int key;

	TC_PRINT("oops\n");
	expected_reason = K_ERR_KERNEL_OOPS;

	key = irq_lock();
	k_oops();
	irq_unlock(key);
}

static void entry_panic(void)
{
	unsigned int key;

	TC_PRINT("panic\n");
	expected_reason = K_ERR_KERNEL_PANIC;

	key = irq_lock();
	k_panic();
	irq_unlock(key);
}

static void entry_zephyr_assert(void)
{
	TC_PRINT("assert\n");
	expected_reason = K_ERR_KERNEL_PANIC;

	__ASSERT(0, "intentionally failed assertion");
}

static void entry_arbitrary_reason(void)
{
	unsigned int key;

	TC_PRINT("arbitrary reason\n");
	expected_reason = INT_MAX;

	key = irq_lock();
	z_except_reason(INT_MAX);
	irq_unlock(key);
}

static void entry_arbitrary_reason_negative(void)
{
	unsigned int key;

	TC_PRINT("arbitrary reason (negative)\n");
	expected_reason = -2;

	key = irq_lock();
	z_except_reason(-2);
	irq_unlock(key);
}

typedef void (*exc_trigger_func_t)(void);

static const exc_trigger_func_t exc_trigger_func[] = {
	entry_cpu_exception,
	entry_oops,
	entry_panic,
	entry_zephyr_assert,
	entry_arbitrary_reason,
	entry_arbitrary_reason_negative,
};

/**
 * @brief Verify the kernel fatal error handling works correctly
 * @details Manually trigger the crash with various ways and check
 * that the kernel is handling that properly or not. Also the crash reason
 * should match.
 *
 * @ingroup kernel_fatal_tests
 */
ZTEST(fatal_no_mt, test_fatal_no_mt)
{
#ifdef VIA_TWISTER
#define EXC_TRIGGER_FUNC_IDX VIA_TWISTER
#else
#define EXC_TRIGGER_FUNC_IDX 0
#endif
	exc_trigger_func[EXC_TRIGGER_FUNC_IDX]();

	ztest_test_fail();
	TC_END_REPORT(TC_FAIL);
}

ZTEST_SUITE(fatal_no_mt, NULL, NULL, NULL, NULL, NULL);
