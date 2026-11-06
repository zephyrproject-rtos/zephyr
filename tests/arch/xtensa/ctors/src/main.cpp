/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

static unsigned int initialized;

struct instance {
	instance()
	{
		initialized++;
	}
};

static instance first;
static instance second;

extern "C" void crt_init(void)
{
	initialized++;
}

static void legacy_init(void)
{
	initialized++;
}

static void (*const legacy_callback)(void) __attribute__((section(".ctors"), used)) = legacy_init;

ZTEST(ctors, test_initializers)
{
	zassert_equal(initialized, 4U, "initializer missing or called more than once");
}

ZTEST_SUITE(ctors, NULL, NULL, NULL, NULL, NULL);
