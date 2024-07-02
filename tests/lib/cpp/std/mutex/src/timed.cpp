/*
 * Copyright (c) 2020, Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_main.hpp"

#include <zephyr/ztest.h>

static std::timed_mutex mu;

ZTEST(std_mutex, test_timed)
{
	mu.lock();

	std::thread th([] {
		zassert_false(mu.try_lock());
		zassert_false(mu.try_lock_for(2 * dt));
		mu.lock();
		lock_succeeded = true;
		mu.unlock();
	});
	std::this_thread::sleep_until(t1);
	zassert_false(lock_succeeded);
	mu.unlock();

	zassert_true(mu.try_lock_until(t3));
	mu.unlock();

	th.join();

	zassert_true(lock_succeeded);
}
