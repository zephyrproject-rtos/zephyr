/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/call_offloader/call_offloader.h>

CALL_OFFLOADER_ENGINE_DEFINE(init_eng, 1, 1024);

static k_tid_t init_caller;
static k_tid_t init_runner;

static void record_runner(void *args)
{
	ARG_UNUSED(args);

	init_runner = k_current_get();
}

static int dispatch_during_init(void)
{
	init_caller = k_current_get();
	call_offloader_dispatch(&init_eng, record_runner, NULL);

	return 0;
}

SYS_INIT(dispatch_during_init, POST_KERNEL, 0);

ZTEST(call_offloader_init, test_dispatch_from_early_post_kernel_init)
{
	zassert_not_null(init_runner, "the call dispatched during POST_KERNEL init did not run");
	zassert_true(init_runner != init_caller, "the call ran on the init thread");
}

ZTEST_SUITE(call_offloader_init, NULL, NULL, NULL, NULL, NULL);
