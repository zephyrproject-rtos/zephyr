/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/arch/riscv/reg.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define ROGUE_USER_STACK_SZ 2048

static struct k_thread rogue_user_thread;
static K_THREAD_STACK_DEFINE(rogue_user_stack, ROGUE_USER_STACK_SZ);

static void rogue_user_fn(void *p1, void *p2, void *p3)
{
	zassert_true(k_is_user_context());

	reg_write(gp, 0xbad);
	zassert_equal(reg_read(gp), 0xbad);
}

ZTEST_USER(riscv_gp, test_gp_value)
{
	uintptr_t gp_val = reg_read(gp);
	k_tid_t th;

	zassert_not_equal(gp_val, 0);

	th = k_thread_create(&rogue_user_thread, rogue_user_stack, ROGUE_USER_STACK_SZ,
			     rogue_user_fn, NULL, NULL, NULL, -1, K_USER, K_NO_WAIT);
	zassert_ok(k_thread_join(th, K_FOREVER));

	zassert_equal(reg_read(gp), gp_val, "`gp` corrupted by user thread");
}

static void *userspace_setup(void)
{
	k_thread_access_grant(k_current_get(), &rogue_user_thread, &rogue_user_stack);

	return NULL;
}

ZTEST_SUITE(riscv_gp, NULL, userspace_setup, NULL, NULL, NULL);
