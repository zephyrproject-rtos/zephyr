/*
 * Copyright (c) 2020, Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>

#include "_main.hpp"

#include <zephyr/ztest.h>

static std::recursive_timed_mutex mu;

ZTEST(std_mutex, test_resursive_timed)
{
	mu.lock();
	zassert_true(mu.try_lock());

	std::thread th([] {
		zassert_false(mu.try_lock());
		zassert_false(mu.try_lock_for(2 * dt));
		mu.lock();
		mu.lock();
		lock_succeeded = true;
		mu.unlock();
		mu.unlock();
	});
	std::this_thread::sleep_until(t1);
	zassert_false(lock_succeeded);
	mu.unlock();
	mu.unlock();

	zassert_true(mu.try_lock_until(t3));
	mu.unlock();

	th.join();

	zassert_true(lock_succeeded);
}
