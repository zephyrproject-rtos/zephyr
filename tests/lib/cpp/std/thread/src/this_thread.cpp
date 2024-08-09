/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <chrono>
#include <system_error>
#include <thread>

#include <zephyr/ztest.h>

using namespace std;
using namespace std::chrono_literals;

static auto now()
{
	return chrono::steady_clock::now();
}

ZTEST(this_thread, test_get_id)
{
	thread::id id = this_thread::get_id();
}

ZTEST(this_thread, test_yield)
{
	this_thread::yield();
}

ZTEST(this_thread, test_sleep_until)
{
	this_thread::sleep_until(now() + 10ms);
}

ZTEST(this_thread, test_sleep_for)
{
	this_thread::sleep_for(10ms);
}

ZTEST_SUITE(this_thread, NULL, NULL, NULL, NULL, NULL);
