/*
 * Copyright (c) 2022 Intel Corporation+
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/async.h>
#include <zephyr/kernel.h>
#include <ztest.h>

struct test_instance {
	int id;
};

static struct k_poll_signal sig;
static struct async_poll_signal async_sig;
static int t_result;
static void *t_user_data = &t_result;
static struct test_instance t_instance = {
	.id = 123456
};
static int user_cb_cnt;

/* This part must be implemented by the module which offers async api. */
struct test_callee {
	struct async_callee async;
	struct test_instance *instance;
};

static struct test_instance *test_cb_get_instance(struct async_callee *callee)
{
	struct test_callee *t_callee = CONTAINER_OF(callee, struct test_callee, async);

	return t_callee->instance;
}

static struct test_instance *test_signal_get_instance(struct async_poll_signal *async_signal)
{
	return (struct test_instance *)async_signal->callee_data;
}

/* _Generic can be used to provide function overloading. */
#define test_get_instance(x) _Generic((x), \
		struct async_callee *: test_cb_get_instance, \
		struct async_poll_signal *: test_signal_get_instance)(x)
/* End of adaptation part. */

static void test_async_func(struct test_instance *instance,
			    int result,
			    async_callback_t cb,
			    void *caller_data)
{
	struct test_callee callee_data = {
		.instance = instance
	};

	cb(&callee_data.async, result, caller_data);
}

static void user_cb(struct async_callee *callee_data, int result, void *caller_data)
{
	struct test_instance *instance = test_cb_get_instance(callee_data);

	zassert_equal(instance, &t_instance, NULL);

	/* Using _Generic to get function overloading. */
	instance = test_get_instance(callee_data);
	zassert_equal(instance, &t_instance, NULL);

	zassert_equal(result, t_result, NULL);
	zassert_equal(caller_data, t_user_data, NULL);
	user_cb_cnt++;
}

void test_async_cb(void)
{
	user_cb_cnt = 0;
	t_result = 124;

	test_async_func(&t_instance, t_result, user_cb, t_user_data);

	zassert_equal(user_cb_cnt, 1, NULL);
}

void test_async_signal(void)
{
	unsigned int signaled;
	int result;

	k_poll_signal_init(&sig);

	test_async_func(&t_instance, 2, async_signal_cb, &sig);

	k_poll_signal_check(&sig, &signaled, &result);

	zassert_equal(signaled, 1, "expected signal");
	zassert_equal(result, 2, "expected result");
}

void test_async_signal_with_callee_data(void)
{
	unsigned int signaled;
	int result;

	async_sig.callee_data = NULL;

	k_poll_signal_init(&async_sig.signal);

	test_async_func(&t_instance, 2, async_signal_with_data_cb, &async_sig);

	k_poll_signal_check(&async_sig.signal, &signaled, &result);

	zassert_equal(test_signal_get_instance(&async_sig), &t_instance,
			"expected instance pointer");
	/* Using _Generic to get function overloading */
	zassert_equal(test_get_instance(&async_sig), &t_instance,
			"expected instance pointer");
	zassert_equal(signaled, 1, "expected signal");
	zassert_equal(result, 2, "expected result");
}

void test_main(void)
{
	ztest_test_suite(test_async,
			 ztest_unit_test(test_async_cb),
			 ztest_unit_test(test_async_signal),
			 ztest_unit_test(test_async_signal_with_callee_data)
			 );
	ztest_run_test_suite(test_async);
}
