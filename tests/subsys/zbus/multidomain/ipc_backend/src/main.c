/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/zbus/multidomain/zbus_multidomain_ipc.h>
#include <zephyr/zbus/multidomain/zbus_multidomain_types.h>
#include "mock_ipc.h"

DEFINE_FFF_GLOBALS;

/* Get pointer to the device tree registered fake IPC device */
#define FAKE_IPC_DEVICE DEVICE_DT_GET(DT_NODELABEL(fake_ipc))

FAKE_VOID_FUNC1(fake_bound_callback, void *);
FAKE_VOID_FUNC3(fake_received_callback, const void *, size_t, void *);

FAKE_VALUE_FUNC1(int, fake_multidomain_backend_recv_cb, const struct zbus_proxy_agent_msg *);
FAKE_VALUE_FUNC2(int, fake_multidomain_backend_ack_cb, uint32_t, void *);

/* Generate backend config using the macro,
 * generates zbus_multidomain_ipc_config name##_ipc_config (test_agent_ipc_config)
 */
#define FAKE_IPC_NODE DT_NODELABEL(fake_ipc)
_ZBUS_GENERATE_BACKEND_CONFIG_ZBUS_MULTIDOMAIN_TYPE_IPC(test_agent, FAKE_IPC_NODE);

/* Delayed work handler function */
static void delayed_bound_callback_work_handler(struct k_work *work)
{
	zassert_false(was_bound_callback_triggered(),
		      "Bound callback should not have been called yet");
	/* Trigger the bound callback to unblock backend_init */
	trigger_bound_callback();
}

/* Define and initialize delayed work globally */
K_WORK_DELAYABLE_DEFINE(bound_callback_work, delayed_bound_callback_work_handler);

void schedule_delayed_bound_callback_work(int delay_ms)
{
	k_work_schedule(&bound_callback_work, K_MSEC(delay_ms));
}

/* Verify that the mock IPC backend and its integration with the IPC service works as expected */
ZTEST(ipc_backend, test_ipc_mock_backend)
{
	/* Test fake IPC device structure */
	zassert_not_null(FAKE_IPC_DEVICE, "Fake IPC device is NULL");
	zassert_not_null(FAKE_IPC_DEVICE->api, "Fake IPC device API is NULL");
	zassert_equal_ptr(FAKE_IPC_DEVICE->api, &fake_backend_ops,
			  "Device API doesn't match fake backend ops");

	/* Test endpoint registration with callbacks */
	struct ipc_ept test_ept = {0};
	struct ipc_ept_cfg test_cfg = {
		.name = "test_endpoint",
		.cb = {.bound = fake_bound_callback, .received = fake_received_callback},
		.priv = &test_ept};
	int result;

	fake_ipc_register_endpoint_fake.return_val = 0;
	fake_ipc_deregister_endpoint_fake.return_val = 0;

	result = ipc_service_register_endpoint(FAKE_IPC_DEVICE, &test_ept, &test_cfg);
	zassert_equal(result, 0, "Expected successful endpoint registration");
	zassert_equal(fake_ipc_register_endpoint_fake.call_count, 1,
		      "Expected exactly one register call");

	/* Test bound callback */
	trigger_bound_callback();
	zassert_equal(fake_bound_callback_fake.call_count, 1, "Expected bound callback called");
	zassert_equal_ptr(fake_bound_callback_fake.arg0_val, &test_ept,
			  "Expected correct private data");

	/* Test data sending */
	static const char test_data[] = "test";

	fake_ipc_send_fake.return_val = sizeof(test_data);
	result = ipc_service_send(&test_ept, test_data, sizeof(test_data));
	zassert_equal(result, sizeof(test_data), "Expected successful send");
	zassert_equal(fake_ipc_send_fake.call_count, 1, "Expected exactly one send call");
	fake_ipc_send_fake.return_val = 0;

	/* Test received callback */
	static const char received_data[] = "hello";

	trigger_received_callback(received_data, sizeof(received_data));
	zassert_equal(fake_received_callback_fake.call_count, 1,
		      "Expected received callback called");
	zassert_equal_ptr(fake_received_callback_fake.arg0_val, received_data,
			  "Expected correct data");
	zassert_equal(fake_received_callback_fake.arg1_val, sizeof(received_data),
		      "Expected correct length");
	zassert_equal_ptr(fake_received_callback_fake.arg2_val, &test_ept,
			  "Expected correct private data");

	/* Test cleanup */
	result = ipc_service_deregister_endpoint(&test_ept);
	zassert_equal(result, 0, "Expected successful endpoint deregistration");
	zassert_equal(fake_ipc_deregister_endpoint_fake.call_count, 1,
		      "Expected exactly one deregister call");
}

ZTEST(ipc_backend, test_backend_macros)
{
	/* Verify the config created with _ZBUS_GENERATE_BACKEND_CONFIG_ZBUS_MULTIDOMAIN_TYPE_IPC */
	zassert_not_null(&test_agent_ipc_config, "Generated config should not be NULL");
	zassert_not_null(test_agent_ipc_config.dev, "Generated config device should not be NULL");
	zassert_equal_ptr(test_agent_ipc_config.dev, FAKE_IPC_DEVICE,
			  "Generated config device should be fake IPC device");
	zassert_not_null(test_agent_ipc_config.ept_cfg,
			 "Generated config endpoint should not be NULL");
	zassert_str_equal(test_agent_ipc_config.ept_cfg->name, "ipc_ept_test_agent",
			  "Generated config endpoint name should match");

	/* Test the macros for getting API and config */
	struct zbus_multidomain_ipc_config *config;
	const struct zbus_proxy_agent_api *api;

	/* Api from zbus_multidomain_ipc.c */
	extern const struct zbus_proxy_agent_api zbus_multidomain_ipc_api;

	api = _ZBUS_GET_API_ZBUS_MULTIDOMAIN_TYPE_IPC();
	zassert_not_null(api, "API macro returned NULL");
	zassert_equal_ptr(api, &zbus_multidomain_ipc_api, "API macro returned incorrect API");

	config = _ZBUS_GET_CONFIG_ZBUS_MULTIDOMAIN_TYPE_IPC(test_agent);
	zassert_not_null(config, "Config macro returned NULL");
	zassert_equal_ptr(config, &test_agent_ipc_config, "Config macro returned incorrect config");
}

ZTEST(ipc_backend, test_backend_init_valid)
{
	int ret;
	struct zbus_multidomain_ipc_config *config =
		_ZBUS_GET_CONFIG_ZBUS_MULTIDOMAIN_TYPE_IPC(test_agent);
	const struct zbus_proxy_agent_api *api = _ZBUS_GET_API_ZBUS_MULTIDOMAIN_TYPE_IPC();

	/* Schedule work to trigger bound callback after a short delay */
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

ZTEST(ipc_backend, test_backend_init_null)
{
	int ret;
	struct zbus_multidomain_ipc_config *config = NULL;
	const struct zbus_proxy_agent_api *api = _ZBUS_GET_API_ZBUS_MULTIDOMAIN_TYPE_IPC();

	ret = api->backend_init(config);
	zassert_equal(ret, -EINVAL, "Expected error on NULL config");
	/* Ensure backend_init still works with valid config afterwards */
	schedule_delayed_bound_callback_work(1);
	config = _ZBUS_GET_CONFIG_ZBUS_MULTIDOMAIN_TYPE_IPC(test_agent);
	ret = api->backend_init(config);
	zassert_equal(ret, 0, "Expected successful backend initialization after NULL test");

	/* Cleanup */
	ret = ipc_service_deregister_endpoint(&config->ipc_ept);
	zassert_equal(ret, 0, "Expected successful endpoint deregistration");
	zassert_equal(fake_ipc_deregister_endpoint_fake.call_count, 1,
		      "Expected exactly one deregister call");
	ret = ipc_service_close_instance(config->dev);
	zassert_equal(ret, 0, "Expected successful instance close");
	zassert_equal(fake_ipc_close_instance_fake.call_count, 1, "Expected close_instance called");

	fake_ipc_open_instance_fake.return_val = -1;
	reset_bound_callback_flag();
	schedule_delayed_bound_callback_work(1);
	ret = api->backend_init(config);
	zassert_equal(ret, fake_ipc_open_instance_fake.return_val,
		      "Expected fake_ipc_open_instance_fake failure to propagate out");
	zassert_false(was_bound_callback_triggered(), "Expected bound callback to not be called");
	fake_ipc_open_instance_fake.return_val = 0;

	fake_ipc_register_endpoint_fake.return_val = -1;
	reset_bound_callback_flag();
	schedule_delayed_bound_callback_work(1);
	ret = api->backend_init(config);
	zassert_equal(ret, fake_ipc_register_endpoint_fake.return_val,
		      "Expected fake_ipc_register_endpoint_fake failure to propagate out");
	fake_ipc_register_endpoint_fake.return_val = 0;

	/* Initialize again to ensure no side effects from previous NULL test */
	reset_bound_callback_flag();
	schedule_delayed_bound_callback_work(1);
	ret = api->backend_init(config);
	zassert_equal(ret, 0, "Expected successful backend initialization after NULL test");
}

ZTEST(ipc_backend, test_backend_init_null_device)
{
	int ret;
	struct zbus_multidomain_ipc_config *config =
		_ZBUS_GET_CONFIG_ZBUS_MULTIDOMAIN_TYPE_IPC(test_agent);
	const struct zbus_proxy_agent_api *api = _ZBUS_GET_API_ZBUS_MULTIDOMAIN_TYPE_IPC();

	/* Save original device */
	const struct device *original_dev = config->dev;

	/* Test NULL device */
	config->dev = NULL;
	ret = api->backend_init(config);
	zassert_equal(ret, -ENODEV, "Expected error on NULL device");

	/* Restore valid device */
	config->dev = original_dev;

	/* Ensure backend_init still works with valid config afterwards */
	schedule_delayed_bound_callback_work(1);
	ret = api->backend_init(config);
	zassert_equal(ret, 0, "Expected successful backend initialization after NULL device test");
}

ZTEST(ipc_backend, test_backend_init_missing_endpoint)
{
	int ret;
	struct zbus_multidomain_ipc_config *config =
		_ZBUS_GET_CONFIG_ZBUS_MULTIDOMAIN_TYPE_IPC(test_agent);
	const struct zbus_proxy_agent_api *api = _ZBUS_GET_API_ZBUS_MULTIDOMAIN_TYPE_IPC();

	/* Save original endpoint config */
	struct ipc_ept_cfg *original_ept_cfg = config->ept_cfg;

	/* Test NULL endpoint config */
	config->ept_cfg = NULL;
	ret = api->backend_init(config);
	zassert_equal(ret, -EINVAL, "Expected error on NULL endpoint config");

	/* Restore valid endpoint config */
	config->ept_cfg = original_ept_cfg;

	/* Ensure backend_init still works with valid config afterwards */
	schedule_delayed_bound_callback_work(1);
	ret = api->backend_init(config);
	zassert_equal(ret, 0,
		      "Expected successful backend initialization after NULL endpoint test");
}

ZTEST(ipc_backend, test_backend_send_valid)
{
	int ret;
	struct zbus_multidomain_ipc_config *config =
		_ZBUS_GET_CONFIG_ZBUS_MULTIDOMAIN_TYPE_IPC(test_agent);
	const struct zbus_proxy_agent_api *api = _ZBUS_GET_API_ZBUS_MULTIDOMAIN_TYPE_IPC();

	/* Initialize backend first */
	schedule_delayed_bound_callback_work(1);
	ret = api->backend_init(config);
	zassert_equal(ret, 0, "Expected successful backend initialization");

	/* Set up fake send to return success */
	fake_ipc_send_fake.return_val = sizeof(struct zbus_proxy_agent_msg);

	/* Test valid message send */
	struct zbus_proxy_agent_msg test_msg;

	test_msg.type = ZBUS_PROXY_AGENT_MSG_TYPE_MSG;
	test_msg.id = 1;
	test_msg.message_size = 4;
	memcpy(test_msg.message_data, "test", 4);
	strcpy(test_msg.channel_name, "chan");
	test_msg.channel_name_len = strlen(test_msg.channel_name);

	ret = api->backend_send(config, &test_msg);
	zassert_equal(ret, 0, "Expected successful message send");
	zassert_equal(fake_ipc_send_fake.call_count, 1, "Expected send called once");

	/* Verify sent data */
	const struct zbus_proxy_agent_msg *sent_msg =
		(const struct zbus_proxy_agent_msg *)fake_ipc_send_fake.arg2_val;
	zassert_not_null(sent_msg, "Sent message should not be NULL");
	zassert_equal(sent_msg->type, test_msg.type, "Sent message type should match");
	zassert_equal(sent_msg->id, test_msg.id, "Sent message ID should match");
	zassert_equal(sent_msg->message_size, test_msg.message_size,
		      "Sent message size should match");
	zassert_mem_equal(sent_msg->message_data, test_msg.message_data, test_msg.message_size,
			  "Sent message data should match");
	zassert_equal(sent_msg->channel_name_len, test_msg.channel_name_len,
		      "Sent channel name length should match");
	zassert_str_equal(sent_msg->channel_name, test_msg.channel_name,
			  "Sent channel name should match");

	/* Send fails */
	fake_ipc_send_fake.return_val = -1;
	ret = api->backend_send(config, &test_msg);
	zassert_equal(ret, fake_ipc_send_fake.return_val,
		      "Expected fake_ipc_send_fake failure to propagate out");
	fake_ipc_send_fake.return_val = 0;
}

ZTEST(ipc_backend, test_backend_send_invalid)
{
	int ret;
	struct zbus_multidomain_ipc_config *config =
		_ZBUS_GET_CONFIG_ZBUS_MULTIDOMAIN_TYPE_IPC(test_agent);
	const struct zbus_proxy_agent_api *api = _ZBUS_GET_API_ZBUS_MULTIDOMAIN_TYPE_IPC();

	/* Initialize backend first */
	schedule_delayed_bound_callback_work(1);
	ret = api->backend_init(config);
	zassert_equal(ret, 0, "Expected successful backend initialization");

	/* Test NULL message */
	ret = api->backend_send(config, NULL);
	zassert_equal(ret, -EINVAL, "Expected error on NULL message");
	zassert_equal(fake_ipc_send_fake.call_count, 0, "Expected send not called on NULL message");

	/* Test zero-length message - backend should reject before calling IPC send */
	struct zbus_proxy_agent_msg empty_msg = {0};

	ret = api->backend_send(config, &empty_msg);
	zassert_equal(ret, -EINVAL, "Expected error on zero-length message");
	zassert_equal(fake_ipc_send_fake.call_count, 0, "Expected send not called for zero-length");

	/* Ensure backend_send still works with valid message afterwards */
	fake_ipc_send_fake.call_count = 0; /* Reset call count */
	fake_ipc_send_fake.return_val = sizeof(struct zbus_proxy_agent_msg);

	struct zbus_proxy_agent_msg valid_msg;

	valid_msg.type = ZBUS_PROXY_AGENT_MSG_TYPE_MSG;
	valid_msg.id = 2;
	valid_msg.message_size = 4;
	memcpy(valid_msg.message_data, "data", 4);
	strcpy(valid_msg.channel_name, "chan");
	valid_msg.channel_name_len = strlen(valid_msg.channel_name);

	ret = api->backend_send(config, &valid_msg);
	zassert_equal(ret, 0, "Expected successful message send after invalid tests");
	zassert_equal(fake_ipc_send_fake.call_count, 1, "Expected send called once for valid msg");
}

ZTEST(ipc_backend, test_backend_send_invalid_config)
{
	int ret;
	struct zbus_multidomain_ipc_config *config =
		_ZBUS_GET_CONFIG_ZBUS_MULTIDOMAIN_TYPE_IPC(test_agent);
	const struct zbus_proxy_agent_api *api = _ZBUS_GET_API_ZBUS_MULTIDOMAIN_TYPE_IPC();

	/* Initialize backend first */
	schedule_delayed_bound_callback_work(1);
	ret = api->backend_init(config);
	zassert_equal(ret, 0, "Expected successful backend initialization");

	/* Test NULL config */
	struct zbus_proxy_agent_msg test_msg;

	test_msg.type = ZBUS_PROXY_AGENT_MSG_TYPE_MSG;
	test_msg.id = 1;
	test_msg.message_size = 4;
	memcpy(test_msg.message_data, "test", 4);
	strcpy(test_msg.channel_name, "chan");
	test_msg.channel_name_len = strlen(test_msg.channel_name);

	ret = api->backend_send(NULL, &test_msg);
	zassert_equal(ret, -EINVAL, "Expected error on NULL config");
	zassert_equal(fake_ipc_send_fake.call_count, 0, "Expected send not called on NULL config");

	/* Ensure backend_send still works with valid config afterwards */
	fake_ipc_send_fake.call_count = 0; /* Reset call count */
	fake_ipc_send_fake.return_val = sizeof(struct zbus_proxy_agent_msg);
	ret = api->backend_send(config, &test_msg);
	zassert_equal(ret, 0, "Expected successful message send after NULL config test");
	zassert_equal(fake_ipc_send_fake.call_count, 1, "Expected send called once for valid msg");
}

ZTEST(ipc_backend, test_backend_set_recv_cb)
{
	int ret;
	struct zbus_multidomain_ipc_config *config =
		_ZBUS_GET_CONFIG_ZBUS_MULTIDOMAIN_TYPE_IPC(test_agent);
	const struct zbus_proxy_agent_api *api = _ZBUS_GET_API_ZBUS_MULTIDOMAIN_TYPE_IPC();

	ret = api->backend_set_recv_cb(config, fake_multidomain_backend_recv_cb);
	zassert_equal(ret, 0, "Expected successful recv callback set");
	zassert_equal_ptr(config->recv_cb, fake_multidomain_backend_recv_cb,
			  "Expected recv callback to be set correctly");

	ret = api->backend_set_recv_cb(config, NULL);
	zassert_equal(ret, -EINVAL, "Expected error on NULL recv callback");
	zassert_equal_ptr(config->recv_cb, fake_multidomain_backend_recv_cb,
			  "Expected recv callback to remain unchanged after NULL set");

	ret = api->backend_set_recv_cb(NULL, fake_multidomain_backend_recv_cb);
	zassert_equal(ret, -EINVAL, "Expected error on NULL config");
	zassert_equal_ptr(config->recv_cb, fake_multidomain_backend_recv_cb,
			  "Expected recv callback to remain unchanged after NULL config");
}

ZTEST(ipc_backend, test_backend_set_ack_cb)
{
	int ret;
	struct zbus_multidomain_ipc_config *config =
		_ZBUS_GET_CONFIG_ZBUS_MULTIDOMAIN_TYPE_IPC(test_agent);
	const struct zbus_proxy_agent_api *api = _ZBUS_GET_API_ZBUS_MULTIDOMAIN_TYPE_IPC();

	void *user_data = (void *)0x12345678U;

	ret = api->backend_set_ack_cb(config, fake_multidomain_backend_ack_cb, user_data);
	zassert_equal(ret, 0, "Expected successful ack callback set");
	zassert_equal_ptr(config->ack_cb, fake_multidomain_backend_ack_cb,
			  "Expected ack callback to be set correctly");
	zassert_equal_ptr(config->ack_cb_user_data, user_data,
			  "Expected ack user data to be set correctly");

	ret = api->backend_set_ack_cb(config, NULL, user_data);
	zassert_equal(ret, -EINVAL, "Expected error on NULL ack callback");
	zassert_equal_ptr(config->ack_cb, fake_multidomain_backend_ack_cb,
			  "Expected ack callback to remain unchanged after NULL set");
	zassert_equal_ptr(config->ack_cb_user_data, user_data,
			  "Expected ack user data to remain unchanged after NULL set");

	ret = api->backend_set_ack_cb(NULL, fake_multidomain_backend_ack_cb, user_data);
	zassert_equal(ret, -EINVAL, "Expected error on NULL config");
	zassert_equal_ptr(config->ack_cb, fake_multidomain_backend_ack_cb,
			  "Expected ack callback to remain unchanged after NULL config");
	zassert_equal_ptr(config->ack_cb_user_data, user_data,
			  "Expected ack user data to remain unchanged after NULL config");
}

ZTEST(ipc_backend, test_backend_recv)
{
	int ret;
	struct zbus_multidomain_ipc_config *config =
		_ZBUS_GET_CONFIG_ZBUS_MULTIDOMAIN_TYPE_IPC(test_agent);
	const struct zbus_proxy_agent_api *api = _ZBUS_GET_API_ZBUS_MULTIDOMAIN_TYPE_IPC();

	struct zbus_proxy_agent_msg test_msg;

	ret = zbus_create_proxy_agent_msg(&test_msg, "test", 4, "chan", 4);
	zassert_equal(ret, 0, "Expected successful test message creation");

	/* Initialize backend first */
	schedule_delayed_bound_callback_work(1);
	ret = api->backend_init(config);
	zassert_equal(ret, 0, "Expected successful backend initialization");

	trigger_received_callback(&test_msg, sizeof(test_msg));
	zassert_equal(fake_multidomain_backend_recv_cb_fake.call_count, 0,
		      "Expected recv callback to not be called when not set");

	ret = api->backend_set_recv_cb(config, fake_multidomain_backend_recv_cb);
	zassert_equal(ret, 0, "Expected successful recv callback set");
	zassert_equal_ptr(config->recv_cb, fake_multidomain_backend_recv_cb,
			  "Expected recv callback to be set correctly");

	/* Valid message */
	trigger_received_callback(&test_msg, sizeof(test_msg));
	zassert_equal(fake_multidomain_backend_recv_cb_fake.call_count, 1,
		      "Expected recv callback to be called once");
	zassert_equal_ptr(fake_multidomain_backend_recv_cb_fake.arg0_val, &test_msg,
			  "Expected recv callback to receive correct message");

	k_sleep(K_MSEC(5)); /* Ensure works finish */
	zassert_equal(fake_ipc_send_fake.call_count, 1, "Expected send called once for ACK");
	const struct zbus_proxy_agent_msg *ack_msg =
		(const struct zbus_proxy_agent_msg *)fake_ipc_send_fake.arg2_val;
	zassert_not_null(ack_msg, "ACK message should not be NULL");
	zassert_equal(ack_msg->type, ZBUS_PROXY_AGENT_MSG_TYPE_ACK,
		      "ACK message type should match");
	zassert_equal(ack_msg->id, test_msg.id, "ACK message ID should match");

	/* Invalid messages */
	trigger_received_callback(NULL, sizeof(test_msg));
	zassert_equal(fake_multidomain_backend_recv_cb_fake.call_count, 1,
		      "Expected recv callback not to be called on NULL message");

	trigger_received_callback(&test_msg, 0);
	zassert_equal(fake_multidomain_backend_recv_cb_fake.call_count, 1,
		      "Expected recv callback not to be called on zero-length message");

	trigger_received_callback(&test_msg, sizeof(test_msg) - 5);
	zassert_equal(fake_multidomain_backend_recv_cb_fake.call_count, 1,
		      "Expected recv callback not to be called on wrong length message");

	fake_multidomain_backend_recv_cb_fake.return_val = -1;
	/* Update message ID to differentiate calls */
	test_msg.id = 2;
	test_msg.crc32 =
		crc32_ieee((const uint8_t *)&test_msg, sizeof(test_msg) - sizeof(test_msg.crc32));
	trigger_received_callback(&test_msg, sizeof(test_msg));
	zassert_equal(fake_multidomain_backend_recv_cb_fake.call_count, 2,
		      "Expected recv callback to be called again");
	zassert_equal_ptr(fake_multidomain_backend_recv_cb_fake.arg0_val, &test_msg,
			  "Expected recv callback to receive correct message again");

	fake_multidomain_backend_recv_cb_fake.return_val = 0;

	fake_ipc_send_fake.return_val = -1;
	/* Update message ID to differentiate calls */
	test_msg.id = 3;
	test_msg.crc32 =
		crc32_ieee((const uint8_t *)&test_msg, sizeof(test_msg) - sizeof(test_msg.crc32));
	trigger_received_callback(&test_msg, sizeof(test_msg));
	k_sleep(K_MSEC(5)); /* Ensure works finish */
	fake_ipc_send_fake.return_val = 0;
}

ZTEST(ipc_backend, test_backend_ack)
{
	int ret;
	struct zbus_multidomain_ipc_config *config =
		_ZBUS_GET_CONFIG_ZBUS_MULTIDOMAIN_TYPE_IPC(test_agent);
	const struct zbus_proxy_agent_api *api = _ZBUS_GET_API_ZBUS_MULTIDOMAIN_TYPE_IPC();

	void *user_data = (void *)0x87654321U;

	struct zbus_proxy_agent_msg ack_msg;

	ret = zbus_create_proxy_agent_ack_msg(&ack_msg, 42);
	zassert_equal(ret, 0, "Expected successful ACK message creation");

	/* Initialize backend first */
	schedule_delayed_bound_callback_work(1);
	ret = api->backend_init(config);
	zassert_equal(ret, 0, "Expected successful backend initialization");

	trigger_received_callback(&ack_msg, sizeof(ack_msg));
	zassert_equal(fake_multidomain_backend_ack_cb_fake.call_count, 0,
		      "Expected ack callback to not be called when not set");

	ret = api->backend_set_ack_cb(config, fake_multidomain_backend_ack_cb, user_data);
	zassert_equal(ret, 0, "Expected successful ack callback set");
	zassert_equal_ptr(config->ack_cb, fake_multidomain_backend_ack_cb,
			  "Expected ack callback to be set correctly");

	trigger_received_callback(&ack_msg, sizeof(ack_msg));
	zassert_equal(fake_multidomain_backend_ack_cb_fake.call_count, 1,
		      "Expected ack callback to be called once");
	zassert_equal(fake_multidomain_backend_ack_cb_fake.arg0_val, 42,
		      "Expected ack callback to receive correct message ID");
	zassert_equal_ptr(fake_multidomain_backend_ack_cb_fake.arg1_val, user_data,
			  "Expected ack callback to receive correct user data");

	trigger_received_callback(NULL, sizeof(ack_msg));
	zassert_equal(fake_multidomain_backend_ack_cb_fake.call_count, 1,
		      "Expected ack callback not to be called on NULL message");
	trigger_received_callback(&ack_msg, 0);
	zassert_equal(fake_multidomain_backend_ack_cb_fake.call_count, 1,
		      "Expected ack callback not to be called on zero-length message");

	fake_multidomain_backend_ack_cb_fake.return_val = -1;
	trigger_received_callback(&ack_msg, sizeof(ack_msg));
	zassert_equal(fake_multidomain_backend_ack_cb_fake.call_count, 2,
		      "Expected ack callback to be called again");
	zassert_equal(fake_multidomain_backend_ack_cb_fake.arg0_val, 42,
		      "Expected ack callback to receive correct message ID again");
	zassert_equal_ptr(fake_multidomain_backend_ack_cb_fake.arg1_val, user_data,
			  "Expected ack callback to receive correct user data again");
}

ZTEST(ipc_backend, test_backend_invalid_message)
{
	int ret;
	struct zbus_multidomain_ipc_config *config =
		_ZBUS_GET_CONFIG_ZBUS_MULTIDOMAIN_TYPE_IPC(test_agent);
	const struct zbus_proxy_agent_api *api = _ZBUS_GET_API_ZBUS_MULTIDOMAIN_TYPE_IPC();

	/* Initialize backend first */
	schedule_delayed_bound_callback_work(1);
	ret = api->backend_init(config);
	zassert_equal(ret, 0, "Expected successful backend initialization");

	/* Setup invalid message */
	struct zbus_proxy_agent_msg invalid_msg;

	ret = zbus_create_proxy_agent_msg(&invalid_msg, "invalid", 7, "chan", 4);

	invalid_msg.type = 99; /* Invalid type */
	invalid_msg.crc32 = crc32_ieee((const uint8_t *)&invalid_msg,
				       sizeof(invalid_msg) - sizeof(invalid_msg.crc32));

	trigger_received_callback(&invalid_msg, sizeof(invalid_msg));
	zassert_equal(fake_multidomain_backend_recv_cb_fake.call_count, 0,
		      "Expected recv callback not to be called on invalid message type");
	zassert_equal(fake_multidomain_backend_ack_cb_fake.call_count, 0,
		      "Expected ack callback not to be called on invalid message type");

	invalid_msg.type = ZBUS_PROXY_AGENT_MSG_TYPE_MSG;
	invalid_msg.id = 1;
	invalid_msg.crc32 = 0; /* Invalid CRC */

	trigger_received_callback(&invalid_msg, sizeof(invalid_msg));
	zassert_equal(fake_multidomain_backend_recv_cb_fake.call_count, 0,
		      "Expected recv callback not to be called on invalid CRC");
	zassert_equal(fake_multidomain_backend_ack_cb_fake.call_count, 0,
		      "Expected ack callback not to be called on invalid CRC");
}

ZTEST(ipc_backend, test_backend_ipc_error)
{
	int ret;
	struct zbus_multidomain_ipc_config *config =
		_ZBUS_GET_CONFIG_ZBUS_MULTIDOMAIN_TYPE_IPC(test_agent);
	const struct zbus_proxy_agent_api *api = _ZBUS_GET_API_ZBUS_MULTIDOMAIN_TYPE_IPC();

	/* Initialize backend first */
	schedule_delayed_bound_callback_work(1);
	ret = api->backend_init(config);
	zassert_equal(ret, 0, "Expected successful backend initialization");

	/* Trigger error callback */
	trigger_error_callback("Test error");
	/* Asserted with regex in testcase.yaml */
}

static void test_setup(void *fixture)
{
	RESET_FAKE(fake_ipc_open_instance);
	RESET_FAKE(fake_ipc_close_instance);
	RESET_FAKE(fake_ipc_send);
	RESET_FAKE(fake_ipc_register_endpoint);
	RESET_FAKE(fake_ipc_deregister_endpoint);
	RESET_FAKE(fake_bound_callback);
	RESET_FAKE(fake_received_callback);
	RESET_FAKE(fake_multidomain_backend_recv_cb);
	RESET_FAKE(fake_multidomain_backend_ack_cb);
	reset_bound_callback_flag();

	/* Cancel any pending delayed work from previous tests */
	k_work_cancel_delayable(&bound_callback_work);
}

ZTEST_SUITE(ipc_backend, NULL, NULL, test_setup, NULL, NULL);
