/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/arch/riscv/reg.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#if !defined(CONFIG_RISCV_GP) && !defined(CONFIG_RISCV_CURRENT_VIA_GP)
#error "CONFIG_RISCV_GP or CONFIG_RISCV_CURRENT_VIA_GP must be enabled for this test"
#endif

#define ROGUE_USER_STACK_SZ 2048

static struct k_thread rogue_user_thread;
static K_THREAD_STACK_DEFINE(rogue_user_stack, ROGUE_USER_STACK_SZ);

static void rogue_user_fn(void *p1, void *p2, void *p3)
{
	zassert_true(k_is_user_context());
	uintptr_t gp_val = reg_read(gp);
	uintptr_t gp_test_val;

	/* Make sure that `gp` is as expected */
	if (IS_ENABLED(CONFIG_RISCV_GP)) {
		__asm__ volatile("la %0, __global_pointer$" : "=r" (gp_test_val));
	} else { /* CONFIG_RISCV_CURRENT_VIA_GP */
		gp_test_val = (uintptr_t)k_current_get();
	}

	/* Corrupt `gp` reg */
	reg_write(gp, 0xbad);

	/* Make sure that `gp` is corrupted */
	if (IS_ENABLED(CONFIG_RISCV_GP)) {
		zassert_equal(reg_read(gp), 0xbad);
	} else { /* CONFIG_RISCV_CURRENT_VIA_GP */
		zassert_equal((uintptr_t)_current, 0xbad);
	}

	/* Sleep to force a context switch, which will sanitize `gp` */
	k_msleep(50);

	/* Make sure that `gp` is sane again */
	if (IS_ENABLED(CONFIG_RISCV_GP)) {
		__asm__ volatile("la %0, __global_pointer$" : "=r" (gp_test_val));
	} else { /* CONFIG_RISCV_CURRENT_VIA_GP */
		gp_test_val = (uintptr_t)k_current_get();
	}

	zassert_equal(gp_val, gp_test_val);
}

ZTEST_USER(riscv_gp, test_gp_value)
{
	uintptr_t gp_val = reg_read(gp);
	uintptr_t gp_test_val;
	k_tid_t th;

	if (IS_ENABLED(CONFIG_RISCV_GP)) {
		__asm__ volatile("la %0, __global_pointer$" : "=r" (gp_test_val));
	} else { /* CONFIG_RISCV_CURRENT_VIA_GP */
		gp_test_val = (uintptr_t)k_current_get();
	}
	zassert_equal(gp_val, gp_test_val);

	/* Create and run a rogue thread to corrupt the `gp` */
	th = k_thread_create(&rogue_user_thread, rogue_user_stack, ROGUE_USER_STACK_SZ,
			     rogue_user_fn, NULL, NULL, NULL, -1, K_USER, K_NO_WAIT);
	zassert_ok(k_thread_join(th, K_FOREVER));

	/* Make sure that `gp` is the same as before a rogue thread was executed */
	zassert_equal(reg_read(gp), gp_val, "`gp` corrupted by user thread");
}

static void *userspace_setup(void)
{
	k_thread_access_grant(k_current_get(), &rogue_user_thread, &rogue_user_stack);

	return NULL;
}

ZTEST_SUITE(riscv_gp, NULL, userspace_setup, NULL, NULL, NULL);
