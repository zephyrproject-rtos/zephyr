/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <system_error>
#include <thread>
#include <ztest.h>

BUILD_ASSERT(__cplusplus == 201703);

using namespace std;

void test_this_thread_get_id(void)
{
	thread::id id = this_thread::get_id();
}

void test_this_thread_yield(void)
{
	this_thread::yield();
}

void test_this_thread_sleep_until(void)
{
}

void test_this_thread_sleep_for(void)
{
}
