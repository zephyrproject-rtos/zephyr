/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * APPLET_HALT_ON_FAULT_SYSTEM ends in k_fatal_halt(), so the image never
 * returns to the test runner. This file replaces src/test_applet.c so that the
 * halting case is the only thing the image does, and twister scores it with
 * the console harness instead of a ztest verdict.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/applet/applet.h>

#define HALT_STACK_SIZE (CONFIG_APPLET_THREAD_STACK_SIZE_DEFAULT * 2)

APPLET_THREAD_STACK_DEFINE(halt_stack, HALT_STACK_SIZE);

static struct applet halt_applet;

static void fault_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_oops();
}

ZTEST(applet_halt_system, test_halt_system_on_fault)
{
	struct applet_opts opts = APPLET_OPTS_DEFAULT;

	/* User mode would report the fault as a different reason. */
	opts.user_mode = false;
	opts.halt_on_fault = APPLET_HALT_ON_FAULT_SYSTEM;

	zassert_ok(applet_init(&halt_applet, "halt-system", &opts));
	zassert_ok(applet_add_thread(&halt_applet, halt_stack, K_THREAD_STACK_SIZEOF(halt_stack),
				     fault_entry, NULL, "faulter"));
	zassert_ok(applet_start(&halt_applet));

	k_sleep(K_SECONDS(2));

	zassert_unreachable("the fatal handler should have halted the system");
}

ZTEST_SUITE(applet_halt_system, NULL, NULL, NULL, NULL, NULL);
