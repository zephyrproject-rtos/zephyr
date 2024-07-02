/*
 * Copyright (c) 2020, Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <array>
#include <chrono>
#include <condition_variable>
#include <thread>

#include <zephyr/ztest.h>

constexpr auto N = CONFIG_DYNAMIC_THREAD_POOL_SIZE - 1;

using namespace std::chrono_literals;

using thread_pool = std::array<std::thread, N>;
using scoped_lock = std::unique_lock<std::mutex>;
using time_point = std::chrono::time_point<std::chrono::steady_clock>;

static std::mutex m;
static std::condition_variable cv;

/* number of threads awoken before t1 */
static size_t count;
static time_point t0, t1, t2, t3;
constexpr auto dt = 25ms;

static time_point now()
{
	return std::chrono::steady_clock::now();
}

static void time_init()
{
	t0 = now();
	t1 = t0 + 2 * dt;
	t2 = t0 + 3 * dt;
	t3 = t0 + 4 * dt;
}

static void notify_common(size_t n)
{
	thread_pool tp;

	count = 0;
	time_init();

	for (auto &t : tp) {
		t = std::thread([] {
			scoped_lock lk(m);

			do {
				cv.wait_until(lk, t3);
				auto nw = now();
				if (nw >= t1 && nw < t2) {
					++count;
				} else {
					break;
				}
			} while (true);
		});
	}

	cv.notify_all();
	zassert_equal(count, 0, "expect: %zu actual: %zu", 0, count);

	std::this_thread::sleep_until(t1);

	if (n == 1) {
		cv.notify_one();
	} else {
		cv.notify_all();
	}

	for (auto &t : tp) {
		t.join();
	}

	zassert_equal(count, n, "expect: %zu actual: %zu", n, count);
}

ZTEST(condition_variable, test_notify_one)
{
	/*
	 * Take the current time, t0. Several threads wait until time t2. If any thread wakes before
	 * t2, increase count and exit. Notify one thread at time t1 = t2/2. Join all threads.
	 * Verify that count == 1.
	 */

	notify_common(1);
}

ZTEST(condition_variable, test_notify_all)
{
	/*
	 * Take the current time, t0. Several threads wait until time t2. If any thread wakes before
	 * t2, increase count and exit. Notify all threads at time t1 = t2/2. Join all threads.
	 * Verify that count == N - 1.
	 */

	notify_common(N);
}

ZTEST(condition_variable, test_wait)
{
	count = 0;
	time_init();

	auto t = std::thread([] {
		scoped_lock lk(m);

		cv.wait(lk);
		count++;
	});

	std::this_thread::sleep_until(t1);
	cv.notify_one();

	{
		scoped_lock lk(m);

		cv.wait(lk, [] { return count == 1; });
	}

	t.join();

	zassert_equal(count, 1);
}

ZTEST(condition_variable, test_wait_for)
{
	count = 0;

	auto t = std::thread([] {
		scoped_lock lk(m);

		zassert_equal(cv.wait_for(lk, 0ms), std::cv_status::timeout);
		lk.lock();
		zassert_equal(cv.wait_for(lk, 3 * dt), std::cv_status::no_timeout);
		count++;
	});

	std::this_thread::sleep_for(2 * dt);
	cv.notify_one();

	{
		scoped_lock lk(m);

		zassert_true(cv.wait_for(lk, dt, [] { return count == 1; }));
	}

	t.join();

	zassert_equal(count, 1);
}

ZTEST(condition_variable, test_wait_until)
{
	count = 0;
	time_init();

	auto t = std::thread([] {
		scoped_lock lk(m);

		zassert_equal(std::cv_status::timeout, cv.wait_until(lk, t0));
		std::this_thread::sleep_until(t1);
		count++;
	});

	{
		scoped_lock lk(m);

		zassert_true(cv.wait_until(lk, t2, [] { return count == 1; }));
	}

	t.join();

	zassert_equal(count, 1);
}

ZTEST(condition_variable, test_native_handle)
{
	zassert_not_null(cv.native_handle());
}

ZTEST_SUITE(condition_variable, NULL, NULL, NULL, NULL, NULL);
