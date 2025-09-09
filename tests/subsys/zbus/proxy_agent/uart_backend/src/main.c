/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/serial/uart_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent_types.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent_uart.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_backend_test, LOG_LEVEL_DBG);

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC3(int, fake_proxy_agent_backend_recv_cb, const uint8_t *, size_t, void *);
FAKE_VALUE_FUNC3(int, fake_proxy_agent_backend_recv_cb_error, const uint8_t *, size_t, void *);

/* Generate a backend config for the test agent using proxy agent node */
#define TEST_PROXY_AGENT_NODE DT_NODELABEL(euart0)
_ZBUS_GENERATE_BACKEND_CONFIG_ZBUS_PROXY_AGENT_TYPE_UART(test_agent, TEST_PROXY_AGENT_NODE);

ZTEST(uart_backend, test_backend_macros)
{
	struct zbus_proxy_agent_uart_config *config;
	const struct zbus_proxy_agent_backend_api *api;

	/* API from zbus_proxy_agent_uart.c */
	extern const struct zbus_proxy_agent_backend_api zbus_proxy_agent_uart_backend_api;

	zassert_not_null(test_agent_uart_config.dev, "UART device in config is NULL");
	zassert_equal_ptr(test_agent_uart_config.dev, DEVICE_DT_GET(DT_NODELABEL(euart0)),
			  "UART device in config does not match expected device");
	zassert_equal(test_agent_uart_config.rx.buf_idx, 0, "Initial async_rx_buf_idx is not 0");
	zassert_equal(test_agent_uart_config.tx.frame_buffer_size,
		      CONFIG_ZBUS_PROXY_AGENT_MESSAGE_SIZE +
			      CONFIG_ZBUS_PROXY_AGENT_CHANNEL_NAME_SIZE + 128,
		      "tx_frame_buffer_size is incorrect");

	api = _ZBUS_GET_BACKEND_API_ZBUS_PROXY_AGENT_TYPE_UART();
	zassert_not_null(api, "API macro returned NULL");
	zassert_equal_ptr(api, &zbus_proxy_agent_uart_backend_api,
			  "API macro returned incorrect API");

	config = _ZBUS_GET_CONFIG_ZBUS_PROXY_AGENT_TYPE_UART(test_agent);
	zassert_not_null(config, "Config macro returned NULL");
	zassert_equal_ptr(config, &test_agent_uart_config,
			  "Config macro returned incorrect config");
}

ZTEST(uart_backend, test_backend_init)
{
	int ret;
	struct zbus_proxy_agent_uart_config *config;
	const struct zbus_proxy_agent_backend_api *api;

	config = _ZBUS_GET_CONFIG_ZBUS_PROXY_AGENT_TYPE_UART(test_agent);
	api = _ZBUS_GET_BACKEND_API_ZBUS_PROXY_AGENT_TYPE_UART();

	ret = api->backend_init(NULL);
	zassert_equal(ret, -EFAULT, "Expected error on NULL config");

	ret = api->backend_init((void *)config);
	zassert_equal(ret, 0, "Failed to initialize UART backend: %d", ret);
}

ZTEST(uart_backend, test_backend_set_recv_cb)
{
	int ret;
	struct zbus_proxy_agent_uart_config *config;
	const struct zbus_proxy_agent_backend_api *api;

	config = _ZBUS_GET_CONFIG_ZBUS_PROXY_AGENT_TYPE_UART(test_agent);
	api = _ZBUS_GET_BACKEND_API_ZBUS_PROXY_AGENT_TYPE_UART();

	ret = api->backend_set_recv_cb(NULL, fake_proxy_agent_backend_recv_cb, NULL);
	zassert_equal(ret, -EFAULT, "Expected error on NULL config");

	ret = api->backend_set_recv_cb((void *)config, NULL, NULL);
	zassert_equal(ret, -EFAULT, "Expected error on NULL callback");

	ret = api->backend_set_recv_cb((void *)config, fake_proxy_agent_backend_recv_cb, NULL);
	zassert_equal(ret, 0, "Failed to set recv callback: %d", ret);
	zassert_equal_ptr(config->callbacks.recv_cb, fake_proxy_agent_backend_recv_cb,
			  "Recv callback not set correctly");
}

ZTEST(uart_backend, test_device_not_ready)
{
	int ret;
	const struct zbus_proxy_agent_backend_api *api;
	struct zbus_proxy_agent_uart_config null_dev_config;

	api = _ZBUS_GET_BACKEND_API_ZBUS_PROXY_AGENT_TYPE_UART();

	ret = api->backend_init(NULL);
	zassert_equal(ret, -EFAULT, "Expected error on NULL config");

	memset(&null_dev_config, 0, sizeof(null_dev_config));
	null_dev_config.dev = NULL;
	ret = api->backend_init((void *)&null_dev_config);
	zassert_equal(ret, -EFAULT, "Expected error on NULL device");
}

ZTEST(uart_backend, test_backend_send)
{
	int ret;
	struct zbus_proxy_agent_uart_config *config;
	const struct zbus_proxy_agent_backend_api *api;
	struct zbus_proxy_agent_msg test_msg;
	uint8_t tx_buf[256];
	uint32_t transmitted_len;

	config = _ZBUS_GET_CONFIG_ZBUS_PROXY_AGENT_TYPE_UART(test_agent);
	api = _ZBUS_GET_BACKEND_API_ZBUS_PROXY_AGENT_TYPE_UART();

	/* Initialize the backend first */
	ret = api->backend_init((void *)config);
	zassert_equal(ret, 0, "Failed to initialize UART backend: %d", ret);

	ret = zbus_create_proxy_agent_msg(&test_msg, "test", 4, "chan", 4);
	zassert_equal(ret, 0, "Failed to create proxy agent message: %d", ret);

	uart_emul_flush_tx_data(config->dev);

	ret = api->backend_send((void *)config, (uint8_t *)&test_msg, sizeof(test_msg));
	zassert_equal(ret, 0, "Failed to send message via UART backend: %d", ret);

	k_sleep(K_MSEC(10));

	ret = uart_emul_get_tx_data(config->dev, tx_buf, sizeof(tx_buf));
	zassert_true(ret > 0, "No data was transmitted");
	zassert_equal(tx_buf[0], 0xAA, "First sync byte incorrect");
	zassert_equal(tx_buf[1], 0x55, "Second sync byte incorrect");
	transmitted_len = sys_get_le32(&tx_buf[2]);
	zassert_equal(transmitted_len, sizeof(test_msg), "Length field incorrect");
	zassert_mem_equal(&tx_buf[6], &test_msg, sizeof(test_msg), "Message content mismatch");

	ret = api->backend_send(NULL, (uint8_t *)&test_msg, sizeof(test_msg));
	zassert_equal(ret, -EFAULT, "Expected error on NULL config");
	ret = api->backend_send((void *)config, NULL, sizeof(test_msg));
	zassert_equal(ret, -EFAULT, "Expected error on NULL data");
}

ZTEST(uart_backend, test_send_to_large)
{
	int ret;
	uint32_t crc;
	struct zbus_proxy_agent_uart_config *config;
	const struct zbus_proxy_agent_backend_api *api;

	uint8_t large_message[CONFIG_ZBUS_PROXY_AGENT_MESSAGE_SIZE +
			      CONFIG_ZBUS_PROXY_AGENT_CHANNEL_NAME_SIZE + 128 + 10];

	config = _ZBUS_GET_CONFIG_ZBUS_PROXY_AGENT_TYPE_UART(test_agent);
	api = _ZBUS_GET_BACKEND_API_ZBUS_PROXY_AGENT_TYPE_UART();

	ret = api->backend_init((void *)config);
	zassert_equal(ret, 0, "Failed to initialize UART backend: %d", ret);

	memset(large_message, 0xCC, sizeof(large_message));
	large_message[0] = 0xAA;
	large_message[1] = 0x55;
	sys_put_le32(sizeof(large_message) - 6 - 4,
		     &large_message[2]); /* Length excludes sync and CRC */
	crc = crc32_ieee(large_message, sizeof(large_message) - 4);
	sys_put_le32(crc, &large_message[sizeof(large_message) - 4]);

	ret = api->backend_send((void *)config, large_message, sizeof(large_message));
	zassert_equal(ret, -EMSGSIZE, "Expected -EMSGSIZE for oversized message, got %d", ret);
}

ZTEST(uart_backend, test_backend_recv)
{
	int ret;
	struct zbus_proxy_agent_uart_config *config;
	const struct zbus_proxy_agent_backend_api *api;

	struct zbus_proxy_agent_msg test_msg;
	uint8_t frame_buf[256];
	uint32_t crc;
	size_t frame_len;
	size_t padded_len;

	config = _ZBUS_GET_CONFIG_ZBUS_PROXY_AGENT_TYPE_UART(test_agent);
	api = _ZBUS_GET_BACKEND_API_ZBUS_PROXY_AGENT_TYPE_UART();

	ret = api->backend_init((void *)config);
	zassert_equal(ret, 0, "Failed to initialize UART backend: %d", ret);

	ret = zbus_create_proxy_agent_msg(&test_msg, "test", 4, "chan", 4);
	zassert_equal(ret, 0, "Failed to create proxy agent message: %d", ret);

	/* Build frame manually */
	frame_len = 0;
	frame_buf[frame_len++] = 0xAA;
	frame_buf[frame_len++] = 0x55;
	sys_put_le32(sizeof(test_msg), &frame_buf[frame_len]);
	frame_len += sizeof(uint32_t);
	memcpy(&frame_buf[frame_len], &test_msg, sizeof(test_msg));
	frame_len += sizeof(test_msg);
	crc = crc32_ieee(frame_buf, 6 + sizeof(test_msg));
	sys_put_le32(crc, &frame_buf[frame_len]);
	frame_len += sizeof(uint32_t);
	padded_len = ((frame_len + CONFIG_ZBUS_PROXY_AGENT_RX_CHUNK_SIZE - 1) /
		      CONFIG_ZBUS_PROXY_AGENT_RX_CHUNK_SIZE) *
		     CONFIG_ZBUS_PROXY_AGENT_RX_CHUNK_SIZE;
	while (frame_len < padded_len) {
		frame_buf[frame_len++] = 0x00;
	}

	uart_emul_put_rx_data(config->dev, frame_buf, frame_len);
	k_sleep(K_MSEC(10));
	zassert_equal(fake_proxy_agent_backend_recv_cb_fake.call_count, 0,
		      "Recv callback should not be called when not set");

	ret = api->backend_set_recv_cb((void *)config, fake_proxy_agent_backend_recv_cb, NULL);
	zassert_equal(ret, 0, "Failed to set recv callback: %d", ret);

	uart_emul_flush_tx_data(config->dev);
	uart_emul_put_rx_data(config->dev, frame_buf, frame_len);
	k_sleep(K_MSEC(10));

	zassert_equal(fake_proxy_agent_backend_recv_cb_fake.call_count, 1,
		      "Recv callback should be called once");
	zassert_mem_equal(fake_proxy_agent_backend_recv_cb_fake.arg0_val, &test_msg,
			  sizeof(test_msg), "Recv callback received incorrect message");
	zassert_equal(fake_proxy_agent_backend_recv_cb_fake.arg1_val, sizeof(test_msg),
		      "Recv callback received incorrect message length");
}

ZTEST(uart_backend, test_recv_callback_error)
{
	int ret;
	struct zbus_proxy_agent_uart_config *config;
	const struct zbus_proxy_agent_backend_api *api;

	struct zbus_proxy_agent_msg test_msg;
	uint8_t frame_buf[256];
	uint32_t crc;
	size_t frame_len;
	size_t padded_len;

	config = _ZBUS_GET_CONFIG_ZBUS_PROXY_AGENT_TYPE_UART(test_agent);
	api = _ZBUS_GET_BACKEND_API_ZBUS_PROXY_AGENT_TYPE_UART();

	ret = api->backend_init((void *)config);
	zassert_equal(ret, 0, "Failed to initialize UART backend: %d", ret);
	ret = zbus_create_proxy_agent_msg(&test_msg, "test", 4, "chan", 4);
	zassert_equal(ret, 0, "Failed to create proxy agent message: %d", ret);

	/* Set up error returning callback */
	fake_proxy_agent_backend_recv_cb_error_fake.return_val = -EPERM;
	ret = api->backend_set_recv_cb((void *)config, fake_proxy_agent_backend_recv_cb_error,
				       NULL);
	zassert_equal(ret, 0, "Failed to set recv callback: %d", ret);

	frame_len = 0;
	frame_buf[frame_len++] = 0xAA;
	frame_buf[frame_len++] = 0x55;
	sys_put_le32(sizeof(test_msg), &frame_buf[frame_len]);
	frame_len += sizeof(uint32_t);
	memcpy(&frame_buf[frame_len], &test_msg, sizeof(test_msg));
	frame_len += sizeof(test_msg);
	crc = crc32_ieee(frame_buf, 6 + sizeof(test_msg));
	sys_put_le32(crc, &frame_buf[frame_len]);
	frame_len += sizeof(uint32_t);
	padded_len = ((frame_len + CONFIG_ZBUS_PROXY_AGENT_RX_CHUNK_SIZE - 1) /
		      CONFIG_ZBUS_PROXY_AGENT_RX_CHUNK_SIZE) *
		     CONFIG_ZBUS_PROXY_AGENT_RX_CHUNK_SIZE;
	while (frame_len < padded_len) {
		frame_buf[frame_len++] = 0x00;
	}

	uart_emul_put_rx_data(config->dev, frame_buf, frame_len);
	k_sleep(K_MSEC(10));

	zassert_equal(fake_proxy_agent_backend_recv_cb_error_fake.call_count, 1,
		      "Error callback should be called once, is %d",
		      fake_proxy_agent_backend_recv_cb_error_fake.call_count);
	zassert_mem_equal(fake_proxy_agent_backend_recv_cb_error_fake.arg0_val, &test_msg,
			  sizeof(test_msg), "Error callback received incorrect message");
}

ZTEST(uart_backend, test_backend_recv_invalid_crc)
{
	int ret;
	struct zbus_proxy_agent_uart_config *config;
	const struct zbus_proxy_agent_backend_api *api;

	struct zbus_proxy_agent_msg test_msg;
	uint8_t frame_buf[256];
	uint32_t crc;
	size_t frame_len;
	size_t padded_len;

	config = _ZBUS_GET_CONFIG_ZBUS_PROXY_AGENT_TYPE_UART(test_agent);
	api = _ZBUS_GET_BACKEND_API_ZBUS_PROXY_AGENT_TYPE_UART();

	ret = api->backend_init((void *)config);
	zassert_equal(ret, 0, "Failed to initialize UART backend: %d", ret);

	ret = zbus_create_proxy_agent_msg(&test_msg, "test", 4, "chan", 4);
	zassert_equal(ret, 0, "Failed to create proxy agent message: %d", ret);

	/* Build frame manually */
	frame_len = 0;
	frame_buf[frame_len++] = 0xAA;
	frame_buf[frame_len++] = 0x55;
	sys_put_le32(sizeof(test_msg), &frame_buf[frame_len]);
	frame_len += sizeof(uint32_t);
	memcpy(&frame_buf[frame_len], &test_msg, sizeof(test_msg));
	frame_len += sizeof(test_msg);
	crc = 0xFFFFFFFFU; /* Invalid CRC */
	sys_put_le32(crc, &frame_buf[frame_len]);
	frame_len += sizeof(uint32_t);
	padded_len = ((frame_len + CONFIG_ZBUS_PROXY_AGENT_RX_CHUNK_SIZE - 1) /
		      CONFIG_ZBUS_PROXY_AGENT_RX_CHUNK_SIZE) *
		     CONFIG_ZBUS_PROXY_AGENT_RX_CHUNK_SIZE;
	while (frame_len < padded_len) {
		frame_buf[frame_len++] = 0x00;
	}

	ret = api->backend_set_recv_cb((void *)config, fake_proxy_agent_backend_recv_cb, NULL);
	zassert_equal(ret, 0, "Failed to set recv callback: %d", ret);

	uart_emul_flush_tx_data(config->dev);
	uart_emul_put_rx_data(config->dev, frame_buf, frame_len);
	k_sleep(K_MSEC(10));

	zassert_equal(fake_proxy_agent_backend_recv_cb_fake.call_count, 0,
		      "Recv callback should not be called for invalid CRC");
}

ZTEST(uart_backend, test_recv_too_large)
{
	int ret;
	struct zbus_proxy_agent_uart_config *config;
	const struct zbus_proxy_agent_backend_api *api;

	uint8_t large_frame[32];
	uint32_t huge_length = 0xFFFFFFFFU;

	config = _ZBUS_GET_CONFIG_ZBUS_PROXY_AGENT_TYPE_UART(test_agent);
	api = _ZBUS_GET_BACKEND_API_ZBUS_PROXY_AGENT_TYPE_UART();

	ret = api->backend_init((void *)config);
	zassert_equal(ret, 0, "Failed to initialize UART backend: %d", ret);
	ret = api->backend_set_recv_cb((void *)config, fake_proxy_agent_backend_recv_cb, NULL);
	zassert_equal(ret, 0, "Failed to set recv callback: %d", ret);

	memset(large_frame, 0, sizeof(large_frame));
	large_frame[0] = 0xAA;
	large_frame[1] = 0x55;
	sys_put_le32(huge_length, &large_frame[2]);

	uart_emul_put_rx_data(config->dev, large_frame, 32);
	k_sleep(K_MSEC(10));

	zassert_equal(fake_proxy_agent_backend_recv_cb_fake.call_count, 0,
		      "Recv callback should not be called for oversized message");
}

ZTEST(uart_backend, test_recv_sync_recovery)
{
	int ret;
	struct zbus_proxy_agent_uart_config *config;
	const struct zbus_proxy_agent_backend_api *api;
	struct zbus_proxy_agent_msg test_msg;
	uint8_t recovery_frame[256];
	uint32_t crc;
	size_t pos = 0;
	size_t padded_len;

	config = _ZBUS_GET_CONFIG_ZBUS_PROXY_AGENT_TYPE_UART(test_agent);
	api = _ZBUS_GET_BACKEND_API_ZBUS_PROXY_AGENT_TYPE_UART();

	ret = api->backend_init((void *)config);
	zassert_equal(ret, 0, "Failed to initialize UART backend: %d", ret);
	ret = api->backend_set_recv_cb((void *)config, fake_proxy_agent_backend_recv_cb, NULL);
	zassert_equal(ret, 0, "Failed to set recv callback: %d", ret);
	ret = zbus_create_proxy_agent_msg(&test_msg, "test", 4, "chan", 4);
	zassert_equal(ret, 0, "Failed to create proxy agent message: %d", ret);

	/* Invalid sync sequence first */
	recovery_frame[pos++] = 0xAA;
	recovery_frame[pos++] = 0xFF;
	/* Valid sync sequence */
	recovery_frame[pos++] = 0xAA;
	recovery_frame[pos++] = 0x55;
	sys_put_le32(sizeof(test_msg), &recovery_frame[pos]);
	pos += sizeof(uint32_t);
	memcpy(&recovery_frame[pos], &test_msg, sizeof(test_msg));
	pos += sizeof(test_msg);
	crc = crc32_ieee(&recovery_frame[2], 6 + sizeof(test_msg));
	sys_put_le32(crc, &recovery_frame[pos]);
	pos += sizeof(uint32_t);
	padded_len = ((pos + CONFIG_ZBUS_PROXY_AGENT_RX_CHUNK_SIZE - 1) /
		      CONFIG_ZBUS_PROXY_AGENT_RX_CHUNK_SIZE) *
		     CONFIG_ZBUS_PROXY_AGENT_RX_CHUNK_SIZE;
	while (pos < padded_len) {
		recovery_frame[pos++] = 0x00;
	}
	uart_emul_put_rx_data(config->dev, recovery_frame, pos);
	k_sleep(K_MSEC(10));

	zassert_equal(fake_proxy_agent_backend_recv_cb_fake.call_count, 1,
		      "Recv callback should be called once after sync recovery, is %d",
		      fake_proxy_agent_backend_recv_cb_fake.call_count);
	zassert_mem_equal(fake_proxy_agent_backend_recv_cb_fake.arg0_val, &test_msg,
			  sizeof(test_msg), "Recovered message content should match");
	zassert_equal(fake_proxy_agent_backend_recv_cb_fake.arg1_val, sizeof(test_msg),
		      "Recovered message length should match");
}

ZTEST(uart_backend, test_uart_rx_recovery)
{
	int ret;
	struct zbus_proxy_agent_uart_config *config;
	const struct zbus_proxy_agent_backend_api *api;

	config = _ZBUS_GET_CONFIG_ZBUS_PROXY_AGENT_TYPE_UART(test_agent);
	api = _ZBUS_GET_BACKEND_API_ZBUS_PROXY_AGENT_TYPE_UART();

	ret = api->backend_init((void *)config);
	zassert_equal(ret, 0, "Failed to initialize UART backend: %d", ret);
	ret = api->backend_set_recv_cb((void *)config, fake_proxy_agent_backend_recv_cb, NULL);
	zassert_equal(ret, 0, "Failed to set recv callback: %d", ret);

	/* Test the UART_RX_DISABLED recovery by disabling and re-enabling RX */
	ret = uart_rx_disable(config->dev);
	zassert_equal(ret, 0, "Failed to disable UART RX: %d", ret);
	k_sleep(K_MSEC(5));
	struct zbus_proxy_agent_msg test_msg;
	uint8_t frame_buf[256];
	uint32_t crc;
	size_t frame_len;
	size_t padded_len;

	ret = zbus_create_proxy_agent_msg(&test_msg, "test", 4, "chan", 4);
	zassert_equal(ret, 0, "Failed to create proxy agent message: %d", ret);

	frame_len = 0;
	frame_buf[frame_len++] = 0xAA;
	frame_buf[frame_len++] = 0x55;
	sys_put_le32(sizeof(test_msg), &frame_buf[frame_len]);
	frame_len += sizeof(uint32_t);
	memcpy(&frame_buf[frame_len], &test_msg, sizeof(test_msg));
	frame_len += sizeof(test_msg);
	crc = crc32_ieee(frame_buf, 6 + sizeof(test_msg));
	sys_put_le32(crc, &frame_buf[frame_len]);
	frame_len += sizeof(uint32_t);
	padded_len = ((frame_len + CONFIG_ZBUS_PROXY_AGENT_RX_CHUNK_SIZE - 1) /
		      CONFIG_ZBUS_PROXY_AGENT_RX_CHUNK_SIZE) *
		     CONFIG_ZBUS_PROXY_AGENT_RX_CHUNK_SIZE;
	while (frame_len < padded_len) {
		frame_buf[frame_len++] = 0x00;
	}
	uart_emul_put_rx_data(config->dev, frame_buf, frame_len);
	k_sleep(K_MSEC(5));

	zassert_equal(fake_proxy_agent_backend_recv_cb_fake.call_count, 1,
		      "Message should be received after RX disable/enable recovery");
	zassert_mem_equal(fake_proxy_agent_backend_recv_cb_fake.arg0_val, &test_msg,
			  sizeof(test_msg), "Received message content should match");
}

static void test_teardown(void *fixture)
{
	struct zbus_proxy_agent_uart_config *config =
		_ZBUS_GET_CONFIG_ZBUS_PROXY_AGENT_TYPE_UART(test_agent);

	/* Reset config state */
	config->rx.buf_idx = 0;
	config->rx.fsm.state = UART_RX_SYNC_SEARCH;
	config->rx.fsm.sync_bytes_found = 0;
	config->rx.fsm.expected_length = 0;
	config->rx.fsm.bytes_received = 0;
	config->rx.fsm.assembly_buffer_pos = 0;
	config->callbacks.recv_cb = NULL;
	config->callbacks.recv_cb_user_data = NULL;
	memset(config->rx.fsm.assembly_buffer, 0, config->rx.fsm.assembly_buffer_size);
	memset(config->rx.async_rx_buf, 0,
	       CONFIG_ZBUS_PROXY_AGENT_RX_CHUNK_SIZE * CONFIG_ZBUS_PROXY_AGENT_RX_BUF_COUNT);

	RESET_FAKE(fake_proxy_agent_backend_recv_cb);
	RESET_FAKE(fake_proxy_agent_backend_recv_cb_error);
	uart_emul_flush_tx_data(config->dev);
	uart_emul_flush_rx_data(config->dev);
}

ZTEST_SUITE(uart_backend, NULL, NULL, NULL, test_teardown, NULL);
