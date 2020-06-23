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

void test_thread_joinable(void)
{
	/* implicitly tests the move assignment operator (operator=)*/
	thread th = thread([]() {});
	zassert_true(th.joinable(),
		     "non-default constructed thread should be joinable");

#ifdef __cpp_exceptions
	try {
		th.join();
	} catch (const system_error &e) {
		zassert_true(false, "joinable thread should join");
	}
#else
	th.join();
#endif

	zassert_false(th.joinable(),
		      "previously joined thread should not be joinable");

	th = thread([]() {});
	th.detach();
	zassert_false(th.joinable(), "detached thread should not be joinable");
}

void test_thread_get_id(void)
{
	thread::id tid = this_thread::get_id();
}

void test_thread_native_handle(void)
{
	thread th([]() {});
	th.native_handle();
	th.join();
}

void test_thread_hardware_concurrency(void)
{
#if defined(CONFIG_BOARD_NATIVE_POSIX) ||                                      \
	defined(CONFIG_BOARD_NATIVE_POSIX_64BIT) ||                            \
	defined(CONFIG_BOARD_NRF52_BSIM)
	zassert_true(thread::hardware_concurrency() >= 1,
		     "actual: %u, expected: >= 1",
		     thread::hardware_concurrency());
#else
	zassert_equal(thread::hardware_concurrency(), CONFIG_MP_NUM_CPUS,
		      "actual: %u, expected: %u",
		      thread::hardware_concurrency(), CONFIG_MP_NUM_CPUS);
#endif
}

void test_thread_join(void)
{
	thread th;

#ifdef __cpp_exceptions
	bool caught = false;
	try {
		th.join();
	} catch (const system_error &e) {
		caught = true;
		zassert_equal(e.code(), errc::no_such_process,
			      "expected errc::no_such_process");
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
		zassert_equal(e.code(), errc::invalid_argument,
			      "expected errc::invalid_argument");
	}
	zassert_true(caught, "join should throw with already-joined thread");
#endif
}

void test_thread_detach(void)
{
	thread th;

#ifdef __cpp_exceptions
	bool caught;
	/* this is the behaviour in linux for detach() with an invalid
     * thread object */
	caught = false;
	try {
		th.detach();
	} catch (const system_error &e) {
		caught = true;
		zassert_equal(e.code(), errc::invalid_argument,
			      "expected errc::invalid_argument");
	}
	zassert_true(caught,
		     "detach should throw with default-constructed thread");
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
	zassert_true(caught,
		     "detach on an already-detached thread should throw");
#endif
}

void test_thread_swap(void)
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
