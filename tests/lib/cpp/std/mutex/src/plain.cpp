/*
 * Copyright (c) 2020, Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_main.hpp"

#include <zephyr/ztest.h>

static std::mutex mu;

ZTEST(std_mutex, test_plain)
{
	mu.lock();

	std::thread th([] {
		zassert_false(mu.try_lock());
		mu.lock();
		lock_succeeded = true;
		mu.unlock();
	});
	std::this_thread::sleep_until(t1);
	zassert_false(lock_succeeded);
	mu.unlock();

	th.join();

	zassert_true(lock_succeeded);
}
