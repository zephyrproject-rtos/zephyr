/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/mbox.h>

int dummy_value;

#if defined(CONFIG_SOC_NRF54L05) || defined(CONFIG_SOC_NRF54L10) || \
	defined(CONFIG_SOC_NRF54L15) || defined(CONFIG_SOC_NRF54H20)
#define EXPECTED_MTU_VALUE				(0)
#define DATA_TRANSFER_MODE_SUPPORTED	(0)
#define REMOTE_BUSY_SUPPORTED			(0)
#else
#define EXPECTED_MTU_VALUE				(4)
#define DATA_TRANSFER_MODE_SUPPORTED	(1)
#define REMOTE_BUSY_SUPPORTED			(1)
#endif

static void dummy_callback(const struct device *dev, mbox_channel_id_t channel_id,
		     void *user_data, struct mbox_msg *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);
	ARG_UNUSED(user_data);
	ARG_UNUSED(data);
	/* Nothing here */
}

static void dummy_callback_2(const struct device *dev, mbox_channel_id_t channel_id,
		     void *user_data, struct mbox_msg *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);
	ARG_UNUSED(data);

	int *temp = (int *) user_data;
	(*temp)++;
}

/**
 * @brief mbox_is_ready_dt() positive test
 *
 * Confirm that mbox_is_ready_dt() returns True
 * on valid local and remote mbox channels.
 *
 */
ZTEST(mbox_error_cases, test_01a_mbox_is_ready_positive)
{
	const struct mbox_dt_spec rx_channel =
		MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), local_valid);
	const struct mbox_dt_spec tx_channel =
		MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), remote_valid);
	bool ret;

	ret = mbox_is_ready_dt(&tx_channel);
	zassert_true(
		ret,
		"mbox_is_ready_dt(tx_channel) should return True,"
		" got unexpected value of %d",
		ret
	);

	ret = mbox_is_ready_dt(&rx_channel);
	zassert_true(
		ret,
		"mbox_is_ready_dt(rx_channel) should return True,"
		" got unexpected value of %d",
		ret
	);
}

/**
 * @brief mbox_is_ready_dt() on incorrect channels
 *
 * Confirm that mbox_is_ready_dt() returns True
 * on invalid local and remote mbox channel.
 * (Device is ready, channel is not checked.)
 *
 */
ZTEST(mbox_error_cases, test_01b_mbox_is_ready_negative)
{
	const struct mbox_dt_spec rx_channel =
		MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), local_incorrect);
	const struct mbox_dt_spec tx_channel =
		MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), remote_incorrect);
	bool ret;

	ret = mbox_is_ready_dt(&tx_channel);
	zassert_true(
		ret,
		"mbox_is_ready_dt(tx_invalid_channel) should return True,"
		" got unexpected value of %d",
		ret
	);

	ret = mbox_is_ready_dt(&rx_channel);
	zassert_true(
		ret,
		"mbox_is_ready_dt(rx_invalid_channel) should return True,"
		" got unexpected value of %d", ret
	);
}

/**
 * @brief mbox_send_dt() on invalid TX channel shall fail
 *
 * Confirm that mbox_send_dt() returns
 * -EINVAL when TX channel is invalid.
 *
 */
ZTEST(mbox_error_cases, test_02a_mbox_send_on_invalid_tx_channel)
{
	const struct mbox_dt_spec tx_channel =
		MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), remote_incorrect);
	int ret;

	ret = mbox_send_dt(&tx_channel, NULL);
	zassert_true(
		(ret == -EINVAL),
		"mbox_send_dt(incorrect_tx_channel) shall return -EINVAL (-22)"
		" got unexpected %d",
		ret
	);
}

/**
 * @brief mbox_send_dt() on RX channel shall fail
 *
 * Confirm that mbox_send_dt() returns
 * -ENOSYS when user tries to send on RX channel.
 *
 */
ZTEST(mbox_error_cases, test_02b_mbox_send_on_rx_channel)
{
	const struct mbox_dt_spec rx_channel =
		MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), local_valid);
	int ret;

	ret = mbox_send_dt(&rx_channel, NULL);
	zassert_true(
		(ret == -ENOSYS),
		"mbox_send_dt(rx_channel) shall return -ENOSYS (-88)"
		" got unexpected %d",
		ret
	);
}

/**
 * @brief mbox_send_dt() with nonzero data field
 *
 * Confirm that mbox_send_dt() returns
 * -EMSGSIZE when driver does NOT support DATA transfer.
 *
 */
ZTEST(mbox_error_cases, test_02c_mbox_send_message_with_data)
{
	const struct mbox_dt_spec tx_channel =
		MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), remote_valid);
	struct mbox_msg data_msg = {0};
	int ret;

	if (DATA_TRANSFER_MODE_SUPPORTED) {
		/* Skip this test because data transfer is supported. */
		ztest_test_skip();
	}

	data_msg.data = &dummy_value;
	data_msg.size = 4;

	ret = mbox_send_dt(&tx_channel, &data_msg);
	zassert_true(
		(ret == -EMSGSIZE),
		"mbox_send_dt(rx_channel, data) shall return -EMSGSIZE (-122)"
		" got unexpected %d",
		ret
	);
}

/**
 * @brief mbox_send_dt() remote busy
 *
 * Confirm that mbox_send_dt() returns
 * -EBUSY when remote hasn’t yet read the last data sent.
 *
 */
ZTEST(mbox_error_cases, test_02d_mbox_send_message_remote_busy)
{
	const struct mbox_dt_spec tx_channel =
		MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), remote_valid);
	int ret;

	if (!REMOTE_BUSY_SUPPORTED)	{
		/* Skip this test because driver is not
		 * capable of detecting that remote is busy.
		 */
		ztest_test_skip();
	}

	ret = mbox_send_dt(&tx_channel, NULL);
	zassert_true(
		(ret == 0),
		"mbox_send_dt(rx_channel) shall return 0"
		" got unexpected %d",
		ret
	);

	ret = mbox_send_dt(&tx_channel, NULL);
	zassert_true(
		(ret == -EBUSY),
		"mbox_send_dt(rx_channel) shall return -EBUSY (-16)"
		" got unexpected %d",
		ret
	);
}

/**
 * @brief mbox_register_callback_dt() on TX channel shall fail
 *
 * Confirm that mbox_register_callback_dt() returns
 * -ENOSYS when user tries to register callback on TX mbox channel.
 *
 */
ZTEST(mbox_error_cases, test_03a_mbox_register_callback_on_remote_channel)
{
	const struct mbox_dt_spec tx_channel =
		MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), remote_valid);
	int ret;

	ret = mbox_register_callback_dt(&tx_channel, dummy_callback, NULL);
	zassert_true(
		(ret == -ENOSYS),
		"mbox_register_callback(remote_channel) shall return -ENOSYS (-88)"
		" got unexpected %d",
		ret
	);
}

/**
 * @brief mbox_register_callback_dt() on incorrect channel shall fail
 *
 * Confirm that mbox_register_callback_dt() returns
 * -EINVAL when user tries to register callback on incorrect mbox channel.
 *
 */
ZTEST(mbox_error_cases, test_03b_mbox_register_callback_on_invalid_channel)
{
	const struct mbox_dt_spec rx_channel =
		MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), local_incorrect);
	int ret;

	ret = mbox_register_callback_dt(&rx_channel, dummy_callback, NULL);
	zassert_true(
		(ret == -EINVAL),
		"mbox_register_callback(incorrect_channel) shall return -EINVAL (-22)"
		" got unexpected %d",
		ret
	);
}

/**
 * @brief mbox_register_callback_dt() multiple use on same channel
 *
 * Confirm that mbox_register_callback_dt() returns
 * 0 when user tries to register callback
 * on already configured mbox channel.
 *
 */
ZTEST(mbox_error_cases, test_03c_mbox_register_callback_twice_on_same_channel)
{
	const struct mbox_dt_spec rx_channel =
		MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), local_valid);
	int ret;

	ret = mbox_register_callback_dt(&rx_channel, dummy_callback, NULL);
	zassert_true(
		(ret == 0),
		"mbox_register_callback(valid_channel) shall return 0"
		" got unexpected %d",
		ret
	);

	ret = mbox_register_callback_dt(&rx_channel, dummy_callback_2, &dummy_value);
	zassert_true(
		(ret == 0),
		"mbox_register_callback(valid_channel) shall return 0"
		" got unexpected %d",
		ret
	);
}

/**
 * @brief mbox_mtu_get_dt() on RX channel shall fail
 *
 * Confirm that mbox_mtu_get_dt() returns
 * -ENOSYS for RX mbox channel.
 *
 */
ZTEST(mbox_error_cases, test_04a_mbox_mtu_get_on_rx_channel)
{
	const struct mbox_dt_spec rx_channel =
		MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), local_valid);
	int ret;

	ret = mbox_mtu_get_dt(&rx_channel);
	zassert_true(
		(ret == -ENOSYS),
		"mbox_mtu_get_dt(rx_channel) shall return -ENOSYS (-88)"
		" got unexpected %d",
		ret
	);
}

/**
 * @brief mbox_mtu_get_dt() on TX channel functional test
 *
 * Confirm that mbox_mtu_get_dt() returns
 * expected value for TX mbox channel.
 * (No matter if channel is valid or incorrect.)
 *
 */
ZTEST(mbox_error_cases, test_04b_mbox_mtu_get_on_tx_channel)
{
	const struct mbox_dt_spec tx_channel =
		MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), remote_valid);
	const struct mbox_dt_spec tx_channel_incorrect =
		MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), remote_incorrect);
	int ret;

	ret = mbox_mtu_get_dt(&tx_channel);
	zassert_true(
		(ret == EXPECTED_MTU_VALUE),
		"mbox_mtu_get_dt(tx_channel) shall return %d"
		" got unexpected %d",
		EXPECTED_MTU_VALUE,
		ret
	);

	ret = mbox_mtu_get_dt(&tx_channel_incorrect);
	zassert_true(
		(ret == EXPECTED_MTU_VALUE),
		"mbox_mtu_get_dt(tx_channel_incorrect) shall return %d"
		" got unexpected %d",
		EXPECTED_MTU_VALUE,
		ret
	);
}

/**
 * @brief mbox_set_enabled_dt() - Enable TX channel shall fail
 *
 * Confirm that mbox_set_enabled_dt() returns
 * -ENOSYS for TX mbox channel.
 *
 */
ZTEST(mbox_error_cases, test_05a_mbox_set_enabled_on_tx_channel)
{
	const struct mbox_dt_spec tx_channel =
		MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), remote_valid);
	int ret;

	ret = mbox_set_enabled_dt(&tx_channel, true);
	zassert_true(
		(ret == -ENOSYS),
		"mbox_set_enabled_dt(tx_channel, true) shall return -ENOSYS (-88)"
		" got unexpected %d",
		ret
	);
}

/**
 * @brief mbox_set_enabled_dt() - Enable incorrect channel shall fail
 *
 * Confirm that mbox_set_enabled_dt() returns
 * -EINVAL for incorrect RX mbox channel.
 *
 */
ZTEST(mbox_error_cases, test_05b_mbox_set_enabled_on_incorrect_rx_channel)
{
	const struct mbox_dt_spec rx_channel =
		MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), local_incorrect);
	int ret;

	ret = mbox_set_enabled_dt(&rx_channel, true);
	zassert_true(
		(ret == -EINVAL),
		"mbox_set_enabled_dt(incorrect_channel, true) shall return -EINVAL (-22)"
		" got unexpected %d",
		ret
	);
}

/**
 * @brief mbox_set_enabled_dt() - Enable already enabled channel shall fail
 *
 * Confirm that mbox_set_enabled_dt() returns
 * -EALREADY when user tries to enable already enabled RX mbox channel.
 *
 */
ZTEST(mbox_error_cases, test_05c_mbox_set_enabled_on_already_enabled_rx_channel)
{
	const struct mbox_dt_spec rx_channel =
		MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), local_valid);
	int ret;

	/* The user must take care of installing a proper callback
	 * on the channel before enabling it.
	 */
	ret = mbox_register_callback_dt(&rx_channel, dummy_callback, NULL);
	zassert_true(
		(ret == 0),
		"mbox_register_callback(tx_channel) shall return 0"
		" got unexpected %d",
		ret
	);

	ret = mbox_set_enabled_dt(&rx_channel, true);
	zassert_true(
		(ret == 0),
		"mbox_set_enabled_dt(tx_channel, true) shall return 0"
		" got unexpected %d",
		ret
	);

	ret = mbox_set_enabled_dt(&rx_channel, true);
	zassert_true(
		(ret == -EALREADY),
		"mbox_set_enabled_dt(enabled_tx_channel, true) shall return -EALREADY (-120)"
		" got unexpected %d",
		ret
	);

	/* Cleanup - disable mbox channel */
	ret = mbox_set_enabled_dt(&rx_channel, false);
	zassert_true(
		(ret == 0),
		"mbox_set_enabled_dt(enabled_tx_channel, false) shall return 0"
		" got unexpected %d",
		ret
	);
}

/**
 * @brief mbox_set_enabled_dt() - Dsiable already disabled channel shall fail
 *
 * Confirm that mbox_set_enabled_dt() returns
 * -EALREADY when user tries to disable already disabled RX mbox channel.
 *
 */
ZTEST(mbox_error_cases, test_05d_mbox_set_disable_on_already_disabled_rx_channel)
{
	const struct mbox_dt_spec rx_channel =
		MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), local_valid);
	int ret;

	/* The user must take care of installing a proper callback
	 * on the channel before enabling it.
	 */
	ret = mbox_register_callback_dt(&rx_channel, dummy_callback, NULL);
	zassert_true(
		(ret == 0),
		"mbox_register_callback(tx_channel) shall return 0"
		" got unexpected %d",
		ret
	);

	ret = mbox_set_enabled_dt(&rx_channel, true);
	zassert_true(
		(ret == 0),
		"mbox_set_enabled_dt(tx_channel, true) shall return 0"
		" got unexpected %d",
		ret
	);

	ret = mbox_set_enabled_dt(&rx_channel, false);
	zassert_true(
		(ret == 0),
		"mbox_set_enabled_dt(enabled_tx_channel, false) shall return 0"
		" got unexpected %d",
		ret
	);

	ret = mbox_set_enabled_dt(&rx_channel, false);
	zassert_true(
		(ret == -EALREADY),
		"mbox_set_enabled_dt(disabled_tx_channel, false) shall return -EALREADY (-120)"
		" got unexpected %d",
		ret
	);
}

/**
 * @brief mbox_max_channels_get_dt() functional test
 *
 * Confirm that mbox_max_channels_get_dt() returns
 * >0 Maximum possible number of supported channels on success
 * (No matter if channel is valid or incorrect.)
 *
 */
ZTEST(mbox_error_cases, test_06_mbox_max_channels_get_functional)
{
	const struct mbox_dt_spec tx_channel =
		MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), remote_valid);
	const struct mbox_dt_spec tx_channel_incorrect =
		MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), remote_incorrect);
	const struct mbox_dt_spec rx_channel =
		MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), local_valid);
	const struct mbox_dt_spec rx_channel_incorrect =
		MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), local_incorrect);
	int ret1, ret2;

	ret1 = mbox_max_channels_get_dt(&tx_channel);
	TC_PRINT("mbox_max_channels_get_dt(tx_channel): %d\n", ret1);
	zassert_true(
		(ret1 > 0),
		"mbox_max_channels_get_dt(tx_channel) shall return value greater than 0"
		" got unexpected %d",
		ret1
	);

	ret2 = mbox_max_channels_get_dt(&tx_channel_incorrect);
	TC_PRINT("mbox_max_channels_get_dt(tx_channel_incorrect): %d\n", ret2);
	zassert_true(
		(ret2 > 0),
		"mbox_max_channels_get_dt(tx_channel_incorrect) shall return value greater than 0"
		" got unexpected %d",
		ret2
	);

	zassert_true(
		(ret1 == ret2),
		"mbox_max_channels_get_dt() shall return same value disregarding channel No."
		" got unexpected %d and %d",
		ret1,
		ret2
	);

	ret1 = mbox_max_channels_get_dt(&rx_channel);
	TC_PRINT("mbox_max_channels_get_dt(rx_channel): %d\n", ret1);
	zassert_true(
		(ret1 > 0),
		"mbox_max_channels_get_dt(rx_channel) shall return value greater than 0"
		" got unexpected %d",
		ret1
	);

	ret2 = mbox_max_channels_get_dt(&rx_channel_incorrect);
	TC_PRINT("mbox_max_channels_get_dt(rx_channel_incorrect): %d\n", ret2);
	zassert_true(
		(ret2 > 0),
		"mbox_max_channels_get_dt(rx_channel_incorrect) shall return value greater than 0"
		" got unexpected %d",
		ret2
	);

	zassert_true(
		(ret1 == ret2),
		"mbox_max_channels_get_dt() shall return same value disregarding channel No."
		" got unexpected %d and %d",
		ret1,
		ret2
	);
}

static void *suite_setup(void)
{
	TC_PRINT("Test executed on %s\n", CONFIG_BOARD_TARGET);
	TC_PRINT("===================================================================\n");

	return NULL;
}

ZTEST_SUITE(mbox_error_cases, NULL, suite_setup, NULL, NULL, NULL);
