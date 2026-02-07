/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/sys/crc.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent_ipc.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent_types.h>
#include "mock_ipc.h"

DEFINE_FFF_GLOBALS;

#define FAKE_IPC_NODE DT_NODELABEL(fake_ipc)

FAKE_VALUE_FUNC3(int, fake_recv_cb, const uint8_t *, size_t, void *);

/* Generate backend config using the macro */
#define FAKE_IPC_NODE DT_NODELABEL(fake_ipc)
_ZBUS_GENERATE_BACKEND_CONFIG_ZBUS_PROXY_AGENT_TYPE_IPC(test_agent, FAKE_IPC_NODE);

static void delayed_bound_callback_work_handler(struct k_work *work)
{
	zassert_false(was_bound_callback_triggered(),
		      "Bound callback should not have been called yet");
	/* Trigger the bound callback to unblock backend_init */
	trigger_bound_callback();
}

K_WORK_DELAYABLE_DEFINE(bound_callback_work, delayed_bound_callback_work_handler);

void schedule_delayed_bound_callback_work(int delay_ms)
{
	k_work_schedule(&bound_callback_work, K_MSEC(delay_ms));
}

ZTEST(ipc_backend, test_ipc_backend_init)
{
	int ret;
	struct zbus_proxy_agent_ipc_config *config = &test_agent_ipc_config;

	/* Get backend API */
	extern const struct zbus_proxy_agent_backend_api zbus_proxy_agent_ipc_backend_api;
	const struct zbus_proxy_agent_backend_api *api = &zbus_proxy_agent_ipc_backend_api;

	/* Schedule work to trigger bound callback after a short delay
	 * unblocking backend_init.
	 */
	schedule_delayed_bound_callback_work(1);

	ret = api->backend_init(config);
	zassert_equal(ret, 0, "Expected successful backend initialization");
	zassert_not_null(config->ept_cfg->cb.bound, "Expected bound callback to be set");
	zassert_not_null(config->ept_cfg->cb.received, "Expected received callback to be set");
	zassert_not_null(config->ept_cfg->cb.error, "Expected error callback to be set");
	zassert_equal_ptr(config->ept_cfg->priv, config, "Expected private data to be config");
	zassert_equal(fake_ipc_register_endpoint_fake.call_count, 1,
		      "Expected register_endpoint called");
	zassert_equal(fake_ipc_open_instance_fake.call_count, 1, "Expected open_instance called");
	zassert_true(was_bound_callback_triggered(),
		     "Expected bound callback to have been triggered");
}

ZTEST(ipc_backend, test_ipc_backend_send_valid)
{
	int ret;
	uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
	size_t expected_total_size = sizeof(test_data) + sizeof(uint32_t); /* data + CRC */
	size_t sent_size;
	uint32_t sent_crc, expected_crc;

	struct zbus_proxy_agent_ipc_config *config = &test_agent_ipc_config;

	extern const struct zbus_proxy_agent_backend_api zbus_proxy_agent_ipc_backend_api;
	const struct zbus_proxy_agent_backend_api *api = &zbus_proxy_agent_ipc_backend_api;

	schedule_delayed_bound_callback_work(1);
	ret = api->backend_init(config);
	zassert_equal(ret, 0, "Expected successful backend initialization");

	fake_ipc_send_fake.return_val = expected_total_size;

	ret = api->backend_send(config, test_data, sizeof(test_data));
	zassert_equal(ret, 0, "Expected successful send");
	zassert_equal(fake_ipc_send_fake.call_count, 1, "Expected send called once");

	const void *sent_data = fake_ipc_send_fake.arg2_val;

	sent_size = fake_ipc_send_fake.arg3_val;

	zassert_not_null(sent_data, "Sent data should not be NULL");
	zassert_equal(sent_size, expected_total_size, "Sent size should include CRC");

	zassert_mem_equal(sent_data, test_data, sizeof(test_data),
			  "Sent payload should match original data");

	expected_crc = crc32_ieee(test_data, sizeof(test_data));
	memcpy(&sent_crc, (const uint8_t *)sent_data + sizeof(test_data), sizeof(sent_crc));
	zassert_equal(sent_crc, expected_crc, "Sent CRC should match calculated CRC");
}

ZTEST(ipc_backend, test_ipc_backend_send_various_sizes)
{
	int ret;
	struct zbus_proxy_agent_ipc_config *config = &test_agent_ipc_config;
	uint8_t small_data[] = {0xFF};
	uint8_t medium_data[64];
	uint8_t large_data[CONFIG_ZBUS_PROXY_AGENT_MESSAGE_SIZE];

	extern const struct zbus_proxy_agent_backend_api zbus_proxy_agent_ipc_backend_api;
	const struct zbus_proxy_agent_backend_api *api = &zbus_proxy_agent_ipc_backend_api;

	schedule_delayed_bound_callback_work(1);
	ret = api->backend_init(config);
	zassert_equal(ret, 0, "Expected successful backend initialization");

	fake_ipc_send_fake.return_val = 100; /* Dummy success value */

	ret = api->backend_send(config, small_data, sizeof(small_data));
	zassert_equal(ret, 0, "Expected successful send for 1 byte");

	memset(medium_data, 0xAA, sizeof(medium_data));
	ret = api->backend_send(config, medium_data, sizeof(medium_data));
	zassert_equal(ret, 0, "Expected successful send for 64 bytes");

	memset(large_data, 0x55, sizeof(large_data));
	ret = api->backend_send(config, large_data, sizeof(large_data));
	zassert_equal(ret, 0, "Expected successful send for max size");
}

ZTEST(ipc_backend, test_ipc_backend_send_errors)
{
	int ret;
	struct zbus_proxy_agent_ipc_config *config = &test_agent_ipc_config;

	uint8_t test_data[] = {0x01, 0x02, 0x03};
	size_t too_large_size = CONFIG_ZBUS_PROXY_AGENT_MESSAGE_SIZE +
				CONFIG_ZBUS_PROXY_AGENT_CHANNEL_NAME_SIZE + 65;
	uint8_t *large_buffer = k_malloc(too_large_size);

	extern const struct zbus_proxy_agent_backend_api zbus_proxy_agent_ipc_backend_api;
	const struct zbus_proxy_agent_backend_api *api = &zbus_proxy_agent_ipc_backend_api;

	schedule_delayed_bound_callback_work(1);
	ret = api->backend_init(config);
	zassert_equal(ret, 0, "Expected successful backend initialization");

	ret = api->backend_send(NULL, test_data, sizeof(test_data));
	zassert_equal(ret, -EINVAL, "Expected error on NULL config");

	ret = api->backend_send(config, NULL, sizeof(test_data));
	zassert_equal(ret, -EINVAL, "Expected error on NULL data");

	ret = api->backend_send(config, test_data, 0);
	zassert_equal(ret, -EINVAL, "Expected error on zero length");

	zassert_not_null(large_buffer, "Failed to allocate test buffer");

	ret = api->backend_send(config, large_buffer, too_large_size);
	zassert_equal(ret, -EMSGSIZE, "Expected error on oversized data");

	k_free(large_buffer);

	fake_ipc_send_fake.return_val = -EIO;
	ret = api->backend_send(config, test_data, sizeof(test_data));
	zassert_equal(ret, -EIO, "Expected IPC send error to propagate");

	fake_ipc_send_fake.return_val = 0;
}

ZTEST(ipc_backend, test_ipc_backend_set_recv_cb)
{
	int ret;
	struct zbus_proxy_agent_ipc_config *config = &test_agent_ipc_config;

	extern const struct zbus_proxy_agent_backend_api zbus_proxy_agent_ipc_backend_api;
	const struct zbus_proxy_agent_backend_api *api = &zbus_proxy_agent_ipc_backend_api;

	void *test_user_data = (void *)0x12345678;

	ret = api->backend_set_recv_cb(config, fake_recv_cb, test_user_data);
	zassert_equal(ret, 0, "Expected successful recv callback setup");
	zassert_equal_ptr(config->recv_cb, fake_recv_cb, "Expected recv callback to be set");
	zassert_equal_ptr(config->recv_cb_user_data, test_user_data,
			  "Expected recv user data to be set");

	ret = api->backend_set_recv_cb(NULL, fake_recv_cb, test_user_data);
	zassert_equal(ret, -EINVAL, "Expected error on NULL config");

	ret = api->backend_set_recv_cb(config, NULL, test_user_data);
	zassert_equal(ret, -EINVAL, "Expected error on NULL callback");
}

ZTEST(ipc_backend, test_ipc_backend_receive_valid)
{
	int ret;
	struct zbus_proxy_agent_ipc_config *config = &test_agent_ipc_config;

	uint8_t test_payload[] = {0x10, 0x20, 0x30, 0x40};
	uint32_t test_crc = crc32_ieee(test_payload, sizeof(test_payload));
	const uint8_t *received_data;
	size_t received_len;
	void *received_user_data;

	struct {
		uint8_t payload[sizeof(test_payload)];
		uint32_t crc;
	} __packed transport_msg;

	extern const struct zbus_proxy_agent_backend_api zbus_proxy_agent_ipc_backend_api;
	const struct zbus_proxy_agent_backend_api *api = &zbus_proxy_agent_ipc_backend_api;

	void *test_user_data = (void *)0x87654321U;

	schedule_delayed_bound_callback_work(1);
	ret = api->backend_init(config);
	zassert_equal(ret, 0, "Expected successful backend initialization");

	ret = api->backend_set_recv_cb(config, fake_recv_cb, test_user_data);
	zassert_equal(ret, 0, "Expected successful recv callback setup");

	memcpy(transport_msg.payload, test_payload, sizeof(test_payload));
	transport_msg.crc = test_crc;

	/* Trigger receive callback */
	fake_recv_cb_fake.return_val = 0;
	trigger_received_callback(&transport_msg, sizeof(transport_msg));

	zassert_equal(fake_recv_cb_fake.call_count, 1, "Expected recv callback called once");

	/* Verify callback received payload without CRC */
	received_data = fake_recv_cb_fake.arg0_val;
	received_len = fake_recv_cb_fake.arg1_val;
	received_user_data = fake_recv_cb_fake.arg2_val;

	zassert_not_null(received_data, "Expected received data not NULL");
	zassert_equal(received_len, sizeof(test_payload),
		      "Expected received length to be payload size (without CRC)");
	zassert_mem_equal(received_data, test_payload, sizeof(test_payload),
			  "Expected received data to match original payload");
	zassert_equal_ptr(received_user_data, test_user_data,
			  "Expected correct user data passed to callback");
}

ZTEST(ipc_backend, test_ipc_backend_receive_crc_error)
{
	int ret;
	struct zbus_proxy_agent_ipc_config *config = &test_agent_ipc_config;

	uint8_t test_payload[] = {0xAA, 0xBB, 0xCC, 0xDD};
	uint32_t wrong_crc = 0xDEADBEEFU; /* Intentionally wrong */

	struct {
		uint8_t payload[sizeof(test_payload)];
		uint32_t crc;
	} __packed transport_msg;

	extern const struct zbus_proxy_agent_backend_api zbus_proxy_agent_ipc_backend_api;
	const struct zbus_proxy_agent_backend_api *api = &zbus_proxy_agent_ipc_backend_api;

	void *test_user_data = (void *)0x11223344;

	schedule_delayed_bound_callback_work(1);
	ret = api->backend_init(config);
	zassert_equal(ret, 0, "Expected successful backend initialization");

	ret = api->backend_set_recv_cb(config, fake_recv_cb, test_user_data);
	zassert_equal(ret, 0, "Expected successful recv callback setup");

	memcpy(transport_msg.payload, test_payload, sizeof(test_payload));
	transport_msg.crc = wrong_crc;

	fake_recv_cb_fake.return_val = 0;
	trigger_received_callback(&transport_msg, sizeof(transport_msg));

	zassert_equal(fake_recv_cb_fake.call_count, 0,
		      "Expected recv callback NOT called due to CRC error");
}

ZTEST(ipc_backend, test_ipc_backend_receive_invalid_size)
{
	int ret;
	struct zbus_proxy_agent_ipc_config *config = &test_agent_ipc_config;
	uint8_t small_data[] = {0x01, 0x02};
	uint8_t dummy_data[] = {0x00};

	extern const struct zbus_proxy_agent_backend_api zbus_proxy_agent_ipc_backend_api;
	const struct zbus_proxy_agent_backend_api *api = &zbus_proxy_agent_ipc_backend_api;

	schedule_delayed_bound_callback_work(1);
	ret = api->backend_init(config);
	zassert_equal(ret, 0, "Expected successful backend initialization");

	ret = api->backend_set_recv_cb(config, fake_recv_cb, NULL);
	zassert_equal(ret, 0, "Expected successful recv callback setup");

	fake_recv_cb_fake.return_val = 0;
	trigger_received_callback(NULL, 10);
	zassert_equal(fake_recv_cb_fake.call_count, 0,
		      "Expected callback NOT called for NULL data");

	trigger_received_callback(small_data, sizeof(small_data));
	zassert_equal(fake_recv_cb_fake.call_count, 0,
		      "Expected callback NOT called for too small data");

	trigger_received_callback(dummy_data, 0);
	zassert_equal(fake_recv_cb_fake.call_count, 0,
		      "Expected callback NOT called for zero length");
}

ZTEST(ipc_backend, test_ipc_backend_init_null_config)
{
	int ret;

	extern const struct zbus_proxy_agent_backend_api zbus_proxy_agent_ipc_backend_api;
	const struct zbus_proxy_agent_backend_api *api = &zbus_proxy_agent_ipc_backend_api;

	ret = api->backend_init(NULL);
	zassert_equal(ret, -EINVAL, "Expected error on NULL config");
}

ZTEST(ipc_backend, test_ipc_backend_init_null_device)
{
	int ret;

	extern const struct zbus_proxy_agent_backend_api zbus_proxy_agent_ipc_backend_api;
	const struct zbus_proxy_agent_backend_api *api = &zbus_proxy_agent_ipc_backend_api;

	struct zbus_proxy_agent_ipc_config config = {0};

	config.dev = NULL; /* Invalid device */
	config.ept_cfg = &(struct ipc_ept_cfg){.name = "test"};

	ret = api->backend_init(&config);
	zassert_equal(ret, -ENODEV, "Expected error on NULL device");
}

ZTEST(ipc_backend, test_ipc_backend_init_null_ept_cfg)
{
	int ret;

	extern const struct zbus_proxy_agent_backend_api zbus_proxy_agent_ipc_backend_api;
	const struct zbus_proxy_agent_backend_api *api = &zbus_proxy_agent_ipc_backend_api;

	struct zbus_proxy_agent_ipc_config config = {0};

	config.dev = DEVICE_DT_GET(FAKE_IPC_NODE);
	config.ept_cfg = NULL; /* Invalid endpoint config */

	ret = api->backend_init(&config);
	zassert_equal(ret, -EINVAL, "Expected error on NULL endpoint config");
}

ZTEST(ipc_backend, test_ipc_backend_init_ipc_open_failure)
{
	int ret;

	extern const struct zbus_proxy_agent_backend_api zbus_proxy_agent_ipc_backend_api;
	const struct zbus_proxy_agent_backend_api *api = &zbus_proxy_agent_ipc_backend_api;

	struct zbus_proxy_agent_ipc_config *config = &test_agent_ipc_config;

	/* Force IPC open instance to fail */
	fake_ipc_open_instance_fake.return_val = -EIO;

	ret = api->backend_init(config);
	zassert_equal(ret, -EIO, "Expected IPC open failure to propagate");

	fake_ipc_open_instance_fake.return_val = 0;
}

ZTEST(ipc_backend, test_ipc_backend_init_register_endpoint_failure)
{
	int ret;

	extern const struct zbus_proxy_agent_backend_api zbus_proxy_agent_ipc_backend_api;
	const struct zbus_proxy_agent_backend_api *api = &zbus_proxy_agent_ipc_backend_api;

	struct zbus_proxy_agent_ipc_config *config = &test_agent_ipc_config;

	/* Force endpoint registration to fail */
	fake_ipc_register_endpoint_fake.return_val = -ENOMEM;

	ret = api->backend_init(config);
	zassert_equal(ret, -ENOMEM, "Expected endpoint registration failure to propagate");

	fake_ipc_register_endpoint_fake.return_val = 0;
}

ZTEST(ipc_backend, test_ipc_backend_receive_callback_failure)
{
	int ret;
	struct zbus_proxy_agent_ipc_config *config = &test_agent_ipc_config;

	uint8_t test_payload[] = {0x10, 0x20, 0x30, 0x40};
	uint32_t test_crc = crc32_ieee(test_payload, sizeof(test_payload));

	struct {
		uint8_t payload[sizeof(test_payload)];
		uint32_t crc;
	} __packed transport_msg;

	extern const struct zbus_proxy_agent_backend_api zbus_proxy_agent_ipc_backend_api;
	const struct zbus_proxy_agent_backend_api *api = &zbus_proxy_agent_ipc_backend_api;

	schedule_delayed_bound_callback_work(1);
	ret = api->backend_init(config);
	zassert_equal(ret, 0, "Expected successful backend initialization");

	fake_recv_cb_fake.return_val = -EPROTO; /* Force callback to fail */
	ret = api->backend_set_recv_cb(config, fake_recv_cb, NULL);
	zassert_equal(ret, 0, "Expected successful recv callback setup");

	memcpy(transport_msg.payload, test_payload, sizeof(test_payload));
	transport_msg.crc = test_crc;

	trigger_received_callback(&transport_msg, sizeof(transport_msg));

	zassert_equal(fake_recv_cb_fake.call_count, 1, "Expected callback called even if it fails");

	fake_recv_cb_fake.return_val = 0;
}

ZTEST(ipc_backend, test_ipc_backend_receive_no_callback)
{
	int ret;
	struct zbus_proxy_agent_ipc_config *config = &test_agent_ipc_config;

	uint8_t test_payload[] = {0xAA, 0xBB};
	uint32_t test_crc = crc32_ieee(test_payload, sizeof(test_payload));

	struct {
		uint8_t payload[sizeof(test_payload)];
		uint32_t crc;
	} __packed transport_msg;

	extern const struct zbus_proxy_agent_backend_api zbus_proxy_agent_ipc_backend_api;
	const struct zbus_proxy_agent_backend_api *api = &zbus_proxy_agent_ipc_backend_api;

	schedule_delayed_bound_callback_work(1);
	ret = api->backend_init(config);
	zassert_equal(ret, 0, "Expected successful backend initialization");

	config->recv_cb = NULL;

	memcpy(transport_msg.payload, test_payload, sizeof(test_payload));
	transport_msg.crc = test_crc;

	trigger_received_callback(&transport_msg, sizeof(transport_msg));

	/* Verified in test case regex with ".* No receive callback configured" */
}

ZTEST(ipc_backend, test_ipc_backend_error_callback)
{
	int ret;
	struct zbus_proxy_agent_ipc_config *config = &test_agent_ipc_config;

	extern const struct zbus_proxy_agent_backend_api zbus_proxy_agent_ipc_backend_api;
	const struct zbus_proxy_agent_backend_api *api = &zbus_proxy_agent_ipc_backend_api;

	schedule_delayed_bound_callback_work(1);
	ret = api->backend_init(config);
	zassert_equal(ret, 0, "Expected successful backend initialization");

	trigger_error_callback("Test IPC error message");

	/* Verified in test case regex with ".* IPC error: .* on endpoint .*" */
}

static void test_setup(void *fixture)
{
	RESET_FAKE(fake_ipc_open_instance);
	RESET_FAKE(fake_ipc_close_instance);
	RESET_FAKE(fake_ipc_send);
	RESET_FAKE(fake_ipc_register_endpoint);
	RESET_FAKE(fake_ipc_deregister_endpoint);
	RESET_FAKE(fake_recv_cb);
	reset_bound_callback_flag();

	/* Cancel any pending delayed work from previous tests */
	k_work_cancel_delayable(&bound_callback_work);
}

ZTEST_SUITE(ipc_backend, NULL, NULL, test_setup, NULL, NULL);
