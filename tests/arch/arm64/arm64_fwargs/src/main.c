/*
 * Copyright (c) 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/fwargs.h>
#include <zephyr/ztest.h>

#define FWARGS_COUNT 4U

static size_t hook_argc;
static unsigned int hook_call_count;

void __wrap_fwargs_handler_hook(const uintptr_t *args, size_t argc)
{
	if (hook_call_count == 0U) {
		hook_argc = argc;
	}

	ARG_UNUSED(args);
	hook_call_count++;
}

ZTEST(arm64_fwargs, test_arch_calls_fwargs_handler)
{
	zassert_equal(hook_call_count, 1U);
	zassert_equal(hook_argc, FWARGS_COUNT);
}

ZTEST_SUITE(arm64_fwargs, NULL, NULL, NULL, NULL, NULL);
