/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/drivers/lora.h>
#include <zephyr/drivers/lora/lora_fake.h>
#include <zephyr/fff.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/ztest.h>

static const struct device *const fake_lora_dev = DEVICE_DT_GET(DT_NODELABEL(fake_lora));
static uint8_t send_data[32];
static uint32_t send_data_len;

DEFINE_FFF_GLOBALS;

static int capture_send(const struct device *dev, uint8_t *data, uint32_t data_len)
{
	zassert_equal(dev, fake_lora_dev, "wrong device");
	zassert_true(data_len <= sizeof(send_data), "send data too large");

	memcpy(send_data, data, data_len);
	send_data_len = data_len;

	return 0;
}

static int provide_receive(const struct device *dev, uint8_t *data, uint8_t size,
			   k_timeout_t timeout, int16_t *rssi, int8_t *snr)
{
	static const uint8_t payload[] = {0x01, 0x02, 0x03};

	zassert_equal(dev, fake_lora_dev, "wrong device");
	zassert_true(size >= sizeof(payload), "receive buffer too small");
	zassert_true(K_TIMEOUT_EQ(timeout, K_MSEC(25)), "wrong timeout");

	memcpy(data, payload, sizeof(payload));
	*rssi = -42;
	*snr = 7;

	return sizeof(payload);
}

static void configure_frequency(const struct shell *sh)
{
	zassert_ok(shell_execute_cmd(sh, "lora config freq 915000000"));
}

ZTEST(lora_shell, test_config_and_send)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	const struct lora_modem_config *config;

	fake_lora_send_fake.custom_fake = capture_send;

	zassert_ok(shell_execute_cmd(
		sh, "lora config freq 915000000 tx-power -10 bw 125 sf 7 cr 8 pre-len 12"));
	zassert_ok(shell_execute_cmd(sh, "lora send hello"));

	zassert_equal(fake_lora_config_fake.call_count, 1U);
	zassert_equal(fake_lora_config_fake.arg0_val, fake_lora_dev);
	config = fake_lora_config_fake.arg1_val;
	zassert_equal(config->frequency, 915000000U);
	zassert_equal(config->tx_power, -10);
	zassert_equal(config->bandwidth, BW_125_KHZ);
	zassert_equal(config->datarate, SF_7);
	zassert_equal(config->coding_rate, CR_4_8);
	zassert_equal(config->preamble_len, 12U);
	zassert_true(config->tx);

	zassert_equal(fake_lora_send_fake.call_count, 1U);
	zassert_equal(send_data_len, 5U);
	zassert_mem_equal(send_data, "hello", send_data_len);
}

ZTEST(lora_shell, test_config_rejects_invalid_parameters)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();

	zassert_equal(shell_execute_cmd(sh, "lora config unknown 1"), -EINVAL);
	zassert_equal(shell_execute_cmd(sh, "lora config freq 0x100000000"), -EINVAL);
	zassert_equal(shell_execute_cmd(sh, "lora config bw 123"), -EINVAL);
	zassert_equal(fake_lora_config_fake.call_count, 0U);
}

ZTEST(lora_shell, test_send_requires_frequency)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();

	zassert_ok(shell_execute_cmd(sh, "lora config freq 0"));
	zassert_equal(shell_execute_cmd(sh, "lora send hello"), -ENODEV);
	zassert_equal(fake_lora_config_fake.call_count, 0U);
	zassert_equal(fake_lora_send_fake.call_count, 0U);
}

ZTEST(lora_shell, test_send_propagates_driver_error)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();

	configure_frequency(sh);
	fake_lora_send_fake.return_val = -EIO;

	zassert_equal(shell_execute_cmd(sh, "lora send hello"), -EIO);
	zassert_equal(fake_lora_config_fake.call_count, 1U);
	zassert_equal(fake_lora_send_fake.call_count, 1U);
}

ZTEST(lora_shell, test_recv)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	const struct lora_modem_config *config;

	configure_frequency(sh);
	fake_lora_recv_fake.custom_fake = provide_receive;

	zassert_ok(shell_execute_cmd(sh, "lora recv 25"));

	zassert_equal(fake_lora_config_fake.call_count, 1U);
	config = fake_lora_config_fake.arg1_val;
	zassert_false(config->tx);
	zassert_equal(fake_lora_recv_fake.call_count, 1U);
	zassert_equal(fake_lora_recv_fake.arg0_val, fake_lora_dev);
	zassert_equal(fake_lora_recv_fake.arg2_val, UINT8_MAX);
}

ZTEST(lora_shell, test_recv_rejects_invalid_timeout)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();

	configure_frequency(sh);
	zassert_equal(shell_execute_cmd(sh, "lora recv -1"), -EINVAL);
	zassert_equal(fake_lora_recv_fake.call_count, 0U);
}

#ifdef CONFIG_LORA_FAKE_OPTIONAL_API
ZTEST(lora_shell, test_continuous_wave)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();

	zassert_ok(shell_execute_cmd(sh, "lora test_cw 915000000 -4 10"));
	zassert_equal(fake_lora_test_cw_fake.call_count, 1U);
	zassert_equal(fake_lora_test_cw_fake.arg0_val, fake_lora_dev);
	zassert_equal(fake_lora_test_cw_fake.arg1_val, 915000000U);
	zassert_equal(fake_lora_test_cw_fake.arg2_val, -4);
	zassert_equal(fake_lora_test_cw_fake.arg3_val, 10U);
}

ZTEST(lora_shell, test_rssi)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();

	configure_frequency(sh);
	zassert_ok(shell_execute_cmd(sh, "lora rssi"));

	zassert_equal(fake_lora_config_fake.call_count, 1U);
	zassert_equal(fake_lora_recv_async_fake.call_count, 2U);
	zassert_not_null(fake_lora_recv_async_fake.arg1_history[0]);
	zassert_is_null(fake_lora_recv_async_fake.arg1_history[1]);
	zassert_equal(fake_lora_rssi_fake.call_count, 1U);
}

ZTEST(lora_shell, test_energy_detect)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();

	configure_frequency(sh);
	zassert_ok(shell_execute_cmd(sh, "lora energy_detect -1 1"));

	zassert_equal(fake_lora_config_fake.call_count, 1U);
	zassert_equal(fake_lora_recv_async_fake.call_count, 2U);
	zassert_equal(fake_lora_rssi_fake.call_count, 1U);
}
#else
ZTEST(lora_shell, test_optional_api_not_supported)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();

	configure_frequency(sh);

	zassert_equal(shell_execute_cmd(sh, "lora test_cw 915000000 -4 10"), -ENOSYS);
	zassert_equal(shell_execute_cmd(sh, "lora rssi"), -ENOSYS);
	zassert_equal(shell_execute_cmd(sh, "lora energy_detect -1 1"), -ENOSYS);

	zassert_equal(fake_lora_config_fake.call_count, 2U);
	zassert_equal(fake_lora_recv_async_fake.call_count, 4U);
	zassert_is_null(fake_lora_recv_async_fake.arg1_history[1]);
	zassert_is_null(fake_lora_recv_async_fake.arg1_history[3]);
}
#endif /* CONFIG_LORA_FAKE_OPTIONAL_API */

static void lora_shell_before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(send_data, 0, sizeof(send_data));
	send_data_len = 0U;
}

static void *lora_shell_setup(void)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();

	WAIT_FOR(shell_ready(sh), 20000, k_msleep(1));
	zassert_true(shell_ready(sh), "timed out waiting for dummy shell backend");

	return NULL;
}

ZTEST_SUITE(lora_shell, NULL, lora_shell_setup, lora_shell_before, NULL, NULL);
