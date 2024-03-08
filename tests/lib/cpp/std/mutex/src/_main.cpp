/*
 * Copyright (c) 2020, Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_main.hpp"

#include <zephyr/ztest.h>

bool lock_succeeded;
time_point t0, t1, t2, t3;

time_point now()
{
	return std::chrono::steady_clock::now();
}

void time_init()
{
	t0 = now();
	t1 = t0 + 2 * dt;
	t2 = t0 + 3 * dt;
	t3 = t0 + 4 * dt;
}

static void before(void *arg)
{
	ARG_UNUSED(arg);

	lock_succeeded = false;
	time_init();
}

ZTEST_SUITE(std_mutex, nullptr, nullptr, before, nullptr, nullptr);
