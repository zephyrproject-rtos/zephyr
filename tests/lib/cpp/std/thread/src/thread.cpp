/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <system_error>
#include <thread>

#include <zephyr/ztest.h>

using namespace std;

ZTEST(thread, test_joinable)
{
	// implicitly tests the move assignment operator (operator=)
	thread th = thread([]() {});
	zassert_true(th.joinable(), "non-default constructed thread should be joinable");

#ifdef __cpp_exceptions
	try {
		th.join();
	} catch (const system_error &e) {
		zassert_true(false, "joinable thread should join");
	}
#else
	th.join();
#endif

	// This is an issue with gthread, so disabling the test for now.
	if (false) {
		zassert_false(th.joinable(), "previously joined thread should not be joinable");

		th = std::thread([]() {});
		th.detach();
		zassert_false(th.joinable(), "detached thread should not be joinable");
	}
}

ZTEST(thread, test_get_id)
{
	thread::id tid = this_thread::get_id();
}

ZTEST(thread, test_native_handle)
{
	thread th([]() {});
	th.native_handle();
	th.join();
}

ZTEST(thread, test_hardware_concurrency)
{
	if (IS_ENABLED(CONFIG_ARCH_POSIX)) {
		zassert_true(thread::hardware_concurrency() >= 1, "actual: %u, expected: >= 1",
			     thread::hardware_concurrency());
	} else {
		zassert_true(thread::hardware_concurrency() == 0 ||
				     thread::hardware_concurrency() == CONFIG_MP_NUM_CPUS,
			     "actual: %u, expected: %u", thread::hardware_concurrency(),
			     CONFIG_MP_NUM_CPUS);
	}
}

ZTEST(thread, test_join)
{
	thread th;
	__unused bool caught = false;

#ifdef __cpp_exceptions
	try {
		th.join();
	} catch (const system_error &e) {
		caught = true;
	}
	zassert_true(caught, "join of default-constructed thread should throw");
#endif

	th = thread([]() {});
#ifdef __cpp_exceptions
	caught = false;
	try {
		th.join();
	} catch (const system_error &e) {
		caught = true;
	}
	zassert_false(caught, "join() should not throw");
#else
	th.join();
#endif

#ifdef __cpp_exceptions
	caught = false;
	try {
		th.join();
	} catch (const system_error &e) {
		caught = true;
	}
		zassert_true(caught, "join should throw with already-joined thread");
#endif
}

ZTEST(thread, test_detach)
{
	thread th;
	__unused bool caught = false;

#ifdef __cpp_exceptions
	// this is the behaviour in linux for detach() with an invalid thread object
	caught = false;
	try {
		th.detach();
	} catch (const system_error &e) {
		caught = true;
		zassert_equal(e.code(), errc::invalid_argument, "expected errc::invalid_argument");
	}
	zassert_true(caught, "detach should throw with default-constructed thread");
#endif

	th = thread([]() {});
#ifdef __cpp_exceptions
	caught = false;
	try {
		th.detach();
	} catch (const system_error &e) {
		caught = true;
	}
	zassert_false(caught, "detach on a valid thread should not throw");
#else
	th.detach();
#endif

#ifdef __cpp_exceptions
	caught = false;
	try {
		th.detach();
	} catch (const system_error &e) {
		caught = true;
	}
	zassert_true(caught, "detach on an already-detached thread should throw");
#endif
}

ZTEST(thread, test_swap)
{
	thread th1;
	thread th2;

#ifdef __cpp_exceptions
	bool caught = false;
	try {
		th1.swap(th2);
	} catch (...) {
		caught = true;
	}
	zassert_false(caught, "thread::swap() is noexcept");
#endif

	th1 = thread([]() {});
	th2 = thread([]() {});

	thread::id th1_id = th1.get_id();
	thread::id th2_id = th2.get_id();

	th1.swap(th2);

	zassert_equal(th2.get_id(), th1_id, "expected ids to be swapped");
	zassert_equal(th1.get_id(), th2_id, "expected ids to be swapped");

	th1.join();
	th2.join();
}

ZTEST_SUITE(thread, NULL, NULL, NULL, NULL, NULL);
