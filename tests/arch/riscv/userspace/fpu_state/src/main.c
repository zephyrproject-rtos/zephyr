/*
 * Copyright (c) 2026 Hongquan Li
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

static void user_entry(void *p1, void *p2, void *p3)
{
	uint32_t value;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

#ifdef CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION
	__asm__ volatile ("fcvt.w.d %0, fa0" : "=r" (value));
#else
	__asm__ volatile ("fcvt.w.s %0, fa0" : "=r" (value));
#endif

	zassert_equal(value, 0U, "kernel FPU state leaked to userspace: 0x%x", value);
	ztest_test_pass();
}

ZTEST(riscv_userspace_fpu_state, test_fpu_state_is_cleared)
{
	int32_t secret = 0x12345678;

#ifdef CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION
	__asm__ volatile ("fcvt.d.w fa0, %0" : : "r" (secret) : "fa0");
#else
	__asm__ volatile ("fcvt.s.w fa0, %0" : : "r" (secret) : "fa0");
#endif

	k_thread_user_mode_enter(user_entry, NULL, NULL, NULL);
}

ZTEST_SUITE(riscv_userspace_fpu_state, NULL, NULL, NULL, NULL, NULL);
