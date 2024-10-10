/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/comparator.h>
#include <zephyr/drivers/comparator/fake_comp.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/ztest.h>

DEFINE_FFF_GLOBALS;

#define FAKE_COMP_NODE DT_NODELABEL(fake_comp)
#define FAKE_COMP_NAME DEVICE_DT_NAME(FAKE_COMP_NODE)

#define TEST_TRIGGER_DELAY K_SECONDS(1)

#define TEST_AWAIT_TRIGGER_TIMEOUT_BELOW_MIN_CMD \
	("comp await_trigger " FAKE_COMP_NAME " 0")

#define TEST_AWAIT_TRIGGER_TIMEOUT_ABOVE_MAX_CMD						\
	("comp await_trigger " FAKE_COMP_NAME " "						\
	 STRINGIFY(CONFIG_COMPARATOR_SHELL_AWAIT_TRIGGER_MAX_TIMEOUT + 1))

#define TEST_AWAIT_TRIGGER_TIMEOUT_BROKEN_CMD \
	("comp await_trigger " FAKE_COMP_NAME " d")

static const struct shell *test_sh;
static const struct device *test_dev = DEVICE_DT_GET(FAKE_COMP_NODE);
static comparator_callback_t test_callback;
static void *test_callback_user_data;
static struct k_spinlock test_callback_spinlock;
static struct k_work_delayable test_trigger_dwork;

static int test_get_output_stub_1(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 1;
}

static int test_get_output_stub_0(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int test_get_output_stub_eio(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -EIO;
}

static int test_set_trigger_stub_ok(const struct device *dev, enum comparator_trigger trigger)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(trigger);

	return 0;
}

static int test_set_trigger_stub_eio(const struct device *dev, enum comparator_trigger trigger)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(trigger);

	return -EIO;
}

static int test_set_trigger_callback_mock_0(const struct device *dev,
					    comparator_callback_t callback,
					    void *user_data)
{
	ARG_UNUSED(dev);

	K_SPINLOCK(&test_callback_spinlock) {
		test_callback = callback;
		test_callback_user_data = user_data;
	}

	return 0;
}

static int test_set_trigger_callback_stub_0(const struct device *dev,
					    comparator_callback_t callback,
					    void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(callback);
	ARG_UNUSED(user_data);

	return 0;
}

static int test_set_trigger_callback_stub_eio(const struct device *dev,
					      comparator_callback_t callback,
					      void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(callback);
	ARG_UNUSED(user_data);

	return -EIO;
}

static int test_trigger_is_pending_stub_1(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 1;
}

static int test_trigger_is_pending_stub_0(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int test_trigger_is_pending_stub_eio(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -EIO;
}


static void test_trigger_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	test_callback(test_dev, test_callback_user_data);
}

static void test_schedule_trigger(void)
{
	k_work_schedule(&test_trigger_dwork, TEST_TRIGGER_DELAY);
}

static void test_cancel_trigger(void)
{
	struct k_work_sync sync;

	k_work_cancel_delayable_sync(&test_trigger_dwork, &sync);
}

static void *test_setup(void)
{
	k_work_init_delayable(&test_trigger_dwork, test_trigger_handler);
	test_sh = shell_backend_dummy_get_ptr();
	WAIT_FOR(shell_ready(test_sh), 20000, k_msleep(1));
	zassert_true(shell_ready(test_sh), "timed out waiting for dummy shell backend");
	return NULL;
}

static void test_after(void *f)
{
	ARG_UNUSED(f);

	test_cancel_trigger();
}

ZTEST(comparator_shell, test_get_output)
{
	int ret;
	const char *out;
	size_t out_size;

	shell_backend_dummy_clear_output(test_sh);
	comp_fake_comp_get_output_fake.custom_fake = test_get_output_stub_1;
	ret = shell_execute_cmd(test_sh, "comp get_output " FAKE_COMP_NAME);
	zassert_ok(ret);
	zassert_equal(comp_fake_comp_get_output_fake.call_count, 1);
	zassert_equal(comp_fake_comp_get_output_fake.arg0_val, test_dev);
	out = shell_backend_dummy_get_output(test_sh, &out_size);
	zassert_str_equal(out, "\r\n1\r\n");

	comp_fake_comp_get_output_fake.custom_fake = test_get_output_stub_0;
	ret = shell_execute_cmd(test_sh, "comp get_output " FAKE_COMP_NAME);
	zassert_ok(ret);
	zassert_equal(comp_fake_comp_get_output_fake.call_count, 2);
	zassert_equal(comp_fake_comp_get_output_fake.arg0_val, test_dev);
	out = shell_backend_dummy_get_output(test_sh, &out_size);
	zassert_str_equal(out, "\r\n0\r\n");

	comp_fake_comp_get_output_fake.custom_fake = test_get_output_stub_eio;
	ret = shell_execute_cmd(test_sh, "comp get_output " FAKE_COMP_NAME);
	zassert_equal(ret, -EIO);
	zassert_equal(comp_fake_comp_get_output_fake.call_count, 3);
	zassert_equal(comp_fake_comp_get_output_fake.arg0_val, test_dev);
	out = shell_backend_dummy_get_output(test_sh, &out_size);
	zassert_str_equal(out, "\r\nfailed to get output\r\n");
}

ZTEST(comparator_shell, test_set_trigger)
{
	int ret;
	const char *out;
	size_t out_size;

	comp_fake_comp_set_trigger_fake.custom_fake = test_set_trigger_stub_ok;

	ret = shell_execute_cmd(test_sh, "comp set_trigger " FAKE_COMP_NAME " NONE");
	zassert_ok(ret);
	zassert_equal(comp_fake_comp_set_trigger_fake.call_count, 1);
	zassert_equal(comp_fake_comp_set_trigger_fake.arg0_val, test_dev);
	zassert_equal(comp_fake_comp_set_trigger_fake.arg1_val, COMPARATOR_TRIGGER_NONE);

	ret = shell_execute_cmd(test_sh, "comp set_trigger " FAKE_COMP_NAME " RISING_EDGE");
	zassert_ok(ret);
	zassert_equal(comp_fake_comp_set_trigger_fake.call_count, 2);
	zassert_equal(comp_fake_comp_set_trigger_fake.arg0_val, test_dev);
	zassert_equal(comp_fake_comp_set_trigger_fake.arg1_val, COMPARATOR_TRIGGER_RISING_EDGE);

	ret = shell_execute_cmd(test_sh, "comp set_trigger " FAKE_COMP_NAME " FALLING_EDGE");
	zassert_ok(ret);
	zassert_equal(comp_fake_comp_set_trigger_fake.call_count, 3);
	zassert_equal(comp_fake_comp_set_trigger_fake.arg0_val, test_dev);
	zassert_equal(comp_fake_comp_set_trigger_fake.arg1_val, COMPARATOR_TRIGGER_FALLING_EDGE);

	ret = shell_execute_cmd(test_sh, "comp set_trigger " FAKE_COMP_NAME " BOTH_EDGES");
	zassert_ok(ret);
	zassert_equal(comp_fake_comp_set_trigger_fake.call_count, 4);
	zassert_equal(comp_fake_comp_set_trigger_fake.arg0_val, test_dev);
	zassert_equal(comp_fake_comp_set_trigger_fake.arg1_val, COMPARATOR_TRIGGER_BOTH_EDGES);

	ret = shell_execute_cmd(test_sh, "comp set_trigger " FAKE_COMP_NAME " INVALID");
	zassert_equal(ret, -EINVAL);
	zassert_equal(comp_fake_comp_set_trigger_fake.call_count, 4);

	comp_fake_comp_set_trigger_fake.custom_fake = test_set_trigger_stub_eio;

	shell_backend_dummy_clear_output(test_sh);
	ret = shell_execute_cmd(test_sh, "comp set_trigger " FAKE_COMP_NAME " BOTH_EDGES");
	zassert_equal(ret, -EIO);
	zassert_equal(comp_fake_comp_set_trigger_fake.call_count, 5);
	zassert_equal(comp_fake_comp_set_trigger_fake.arg0_val, test_dev);
	zassert_equal(comp_fake_comp_set_trigger_fake.arg1_val, COMPARATOR_TRIGGER_BOTH_EDGES);
	out = shell_backend_dummy_get_output(test_sh, &out_size);
	zassert_str_equal(out, "\r\nfailed to set trigger\r\n");
}

ZTEST(comparator_shell, test_await_trigger_set_callback_fail)
{
	int ret;
	const char *out;
	size_t out_size;

	shell_backend_dummy_clear_output(test_sh);
	comp_fake_comp_set_trigger_callback_fake.custom_fake = test_set_trigger_callback_stub_eio;
	ret = shell_execute_cmd(test_sh, "comp await_trigger " FAKE_COMP_NAME);
	zassert_ok(0);
	zassert_equal(comp_fake_comp_set_trigger_callback_fake.call_count, 1);
	zassert_equal(comp_fake_comp_set_trigger_callback_fake.return_val, 0);
	out = shell_backend_dummy_get_output(test_sh, &out_size);
	zassert_str_equal(out, "\r\nfailed to set trigger callback\r\n");
}

ZTEST(comparator_shell, test_await_trigger_timeout)
{
	int ret;
	const char *out;
	size_t out_size;

	shell_backend_dummy_clear_output(test_sh);
	comp_fake_comp_set_trigger_callback_fake.custom_fake = test_set_trigger_callback_stub_0;
	ret = shell_execute_cmd(test_sh, "comp await_trigger " FAKE_COMP_NAME);
	zassert_ok(0);
	zassert_equal(comp_fake_comp_set_trigger_callback_fake.call_count, 2);
	zassert_equal(comp_fake_comp_set_trigger_callback_fake.return_val_history[0], 0);
	zassert_equal(comp_fake_comp_set_trigger_callback_fake.return_val_history[1], 0);
	out = shell_backend_dummy_get_output(test_sh, &out_size);
	zassert_str_equal(out, "\r\ntimed out\r\n");
}

ZTEST(comparator_shell, test_await_trigger_invalid_timeout_arg)
{
	int ret;

	ret = shell_execute_cmd(test_sh, TEST_AWAIT_TRIGGER_TIMEOUT_BELOW_MIN_CMD);
	zassert_not_ok(ret);
	zassert_equal(comp_fake_comp_set_trigger_callback_fake.call_count, 0);

	ret = shell_execute_cmd(test_sh, TEST_AWAIT_TRIGGER_TIMEOUT_ABOVE_MAX_CMD);
	zassert_not_ok(ret);
	zassert_equal(comp_fake_comp_set_trigger_callback_fake.call_count, 0);

	ret = shell_execute_cmd(test_sh, TEST_AWAIT_TRIGGER_TIMEOUT_BROKEN_CMD);
	zassert_not_ok(ret);
	zassert_equal(comp_fake_comp_set_trigger_callback_fake.call_count, 0);
}

ZTEST(comparator_shell, test_await_trigger)
{
	int ret;
	const char *out;
	size_t out_size;
	comparator_api_set_trigger_callback seq[2];

	shell_backend_dummy_clear_output(test_sh);
	seq[0] = test_set_trigger_callback_mock_0;
	seq[1] = test_set_trigger_callback_stub_0;
	comp_fake_comp_set_trigger_callback_fake.custom_fake_seq = seq;
	comp_fake_comp_set_trigger_callback_fake.custom_fake_seq_len = ARRAY_SIZE(seq);
	test_schedule_trigger();
	ret = shell_execute_cmd(test_sh, "comp await_trigger " FAKE_COMP_NAME);
	zassert_ok(0);
	zassert_equal(comp_fake_comp_set_trigger_callback_fake.call_count, 2);
	zassert_equal(comp_fake_comp_set_trigger_callback_fake.arg0_history[0], test_dev);
	zassert_not_equal(comp_fake_comp_set_trigger_callback_fake.arg1_history[0], NULL);
	zassert_equal(comp_fake_comp_set_trigger_callback_fake.return_val_history[0], 0);
	zassert_equal(comp_fake_comp_set_trigger_callback_fake.arg0_history[1], test_dev);
	zassert_equal(comp_fake_comp_set_trigger_callback_fake.arg1_history[1], NULL);
	zassert_equal(comp_fake_comp_set_trigger_callback_fake.return_val_history[1], 0);
	out = shell_backend_dummy_get_output(test_sh, &out_size);
	zassert_str_equal(out, "\r\ntriggered\r\n");
}

ZTEST(comparator_shell, test_trigger_is_pending)
{
	int ret;
	const char *out;
	size_t out_size;

	shell_backend_dummy_clear_output(test_sh);
	comp_fake_comp_trigger_is_pending_fake.custom_fake = test_trigger_is_pending_stub_1;
	ret = shell_execute_cmd(test_sh, "comp trigger_is_pending " FAKE_COMP_NAME);
	zassert_ok(ret);
	zassert_equal(comp_fake_comp_trigger_is_pending_fake.call_count, 1);
	zassert_equal(comp_fake_comp_trigger_is_pending_fake.arg0_val, test_dev);
	out = shell_backend_dummy_get_output(test_sh, &out_size);
	zassert_str_equal(out, "\r\n1\r\n");

	comp_fake_comp_trigger_is_pending_fake.custom_fake = test_trigger_is_pending_stub_0;
	ret = shell_execute_cmd(test_sh, "comp trigger_is_pending " FAKE_COMP_NAME);
	zassert_ok(ret);
	zassert_equal(comp_fake_comp_trigger_is_pending_fake.call_count, 2);
	zassert_equal(comp_fake_comp_trigger_is_pending_fake.arg0_val, test_dev);
	out = shell_backend_dummy_get_output(test_sh, &out_size);
	zassert_str_equal(out, "\r\n0\r\n");

	comp_fake_comp_trigger_is_pending_fake.custom_fake = test_trigger_is_pending_stub_eio;
	ret = shell_execute_cmd(test_sh, "comp trigger_is_pending " FAKE_COMP_NAME);
	zassert_equal(ret, -EIO);
	zassert_equal(comp_fake_comp_trigger_is_pending_fake.call_count, 3);
	zassert_equal(comp_fake_comp_trigger_is_pending_fake.arg0_val, test_dev);
	out = shell_backend_dummy_get_output(test_sh, &out_size);
	zassert_str_equal(out, "\r\nfailed to get trigger status\r\n");
}

ZTEST_SUITE(comparator_shell, NULL, test_setup, test_after, NULL, NULL);
