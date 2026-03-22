/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/sys/crc.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent_ipc.h>
#include "mock_ipc.h"

DEFINE_FFF_GLOBALS;

#define FAKE_IPC_NODE DT_NODELABEL(fake_ipc)

FAKE_VALUE_FUNC2(int, fake_recv_callback, const struct zbus_proxy_agent *,
		 const struct zbus_proxy_msg *);

/* Workqueue for manual callback triggering in tests */
static void bound_callback_work_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(bound_callback_work, bound_callback_work_handler);

static void bound_callback_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	trigger_bound_callback();
}

static void schedule_bound_callback(int delay_ms)
{
	k_work_schedule(&bound_callback_work, K_MSEC(delay_ms));
}

static struct zbus_proxy_agent
create_test_proxy_agent(const struct zbus_proxy_agent_ipc_config *config)
{
	struct zbus_proxy_agent agent_config = {
		.name = "test_agent",
		.type = ZBUS_PROXY_AGENT_BACKEND_IPC,
		.backend_config = config,
		.backend_api = _ZBUS_PROXY_AGENT_GET_BACKEND_API_ZBUS_PROXY_AGENT_BACKEND_IPC,
	};

	return agent_config;
}

struct zbus_proxy_msg get_last_sent_message(void)
{
	uint8_t *data;
	size_t len;
	struct zbus_proxy_msg msg;

	get_stored_message(&data, &len);

	if (len != sizeof(struct zbus_proxy_msg)) {
		struct zbus_proxy_msg empty_msg = {0};
		return empty_msg;
	}

	memcpy(&msg, data, sizeof(msg));
	return msg;
}

void init_backend(struct zbus_proxy_agent *agent)
{
	int ret;

	schedule_bound_callback(1);
	ret = agent->backend_api->backend_init(agent);
	zassert_equal(ret, 0, "Backend init should succeed with valid config");
	ret = agent->backend_api->backend_set_recv_cb(agent, fake_recv_callback);
	zassert_equal(ret, 0, "Setting receive callback should succeed with valid parameters");
}

/* Test backend config macros */
_ZBUS_PROXY_AGENT_BACKEND_CONFIG_DEFINE_ZBUS_PROXY_AGENT_BACKEND_IPC(test_agent, FAKE_IPC_NODE)

ZTEST(ipc_backend, test_get_backend_config_macro)
{
	const struct zbus_proxy_agent_ipc_config *config =
		_ZBUS_PROXY_AGENT_GET_BACKEND_CONFIG_ZBUS_PROXY_AGENT_BACKEND_IPC(test_agent);

	zassert_not_null(config, "Config should not be NULL");
	zassert_equal(config->dev, DEVICE_DT_GET(FAKE_IPC_NODE),
		      "Device should match the one from devicetree");
	zassert_str_equal(config->ept_name, "test_agent_ipc_ept",
		   "Endpoint name should be correctly set by macro");
}

/* Test set_recv_cb */
ZTEST(ipc_backend, test_set_recv_cb)
{
	int ret;
	struct zbus_proxy_agent_ipc_data ipc_data = {0};
	struct zbus_proxy_agent_ipc_config ipc_config = {
		.dev = DEVICE_DT_GET(FAKE_IPC_NODE),
		.ept_name = "test_ept",
		.data = &ipc_data,
	};

	struct zbus_proxy_agent agent_config = create_test_proxy_agent(&ipc_config);

	ret = agent_config.backend_api->backend_set_recv_cb(NULL, fake_recv_callback);
	zassert_equal(ret, -EFAULT, "Setting receive callback should fail when config is NULL");

	ret = agent_config.backend_api->backend_set_recv_cb(&agent_config, NULL);
	zassert_equal(ret, -EFAULT, "Setting receive callback should fail when callback is NULL");

	ret = agent_config.backend_api->backend_set_recv_cb(&agent_config, fake_recv_callback);
	zassert_equal(ret, 0, "Setting receive callback should succeed with valid parameters");
	zassert_equal(ipc_data.recv_cb, fake_recv_callback,
		      "Receive callback should be set in IPC config");
	zassert_equal(ipc_data.recv_cb_config_ptr, &agent_config,
		      "Receive callback user data should point to the agent config");
}

/* Test init */
ZTEST(ipc_backend, test_backend_init)
{
	int ret;
	struct zbus_proxy_agent_ipc_data ipc_data = {0};
	struct zbus_proxy_agent_ipc_config ipc_config = {
		.dev = DEVICE_DT_GET(FAKE_IPC_NODE),
		.ept_name = "test_ept",
		.data = &ipc_data,
	};

	struct zbus_proxy_agent agent_config = create_test_proxy_agent(&ipc_config);

	ret = agent_config.backend_api->backend_init(NULL);
	zassert_equal(ret, -EFAULT, "Backend init should fail when config is NULL");

	ipc_config.data = NULL;
	ret = agent_config.backend_api->backend_init(&agent_config);
	zassert_equal(ret, -EINVAL, "Backend init should fail when runtime data is NULL");
	ipc_config.data = &ipc_data;

	ipc_config.dev = NULL;
	ret = agent_config.backend_api->backend_init(&agent_config);
	zassert_equal(ret, -ENODEV, "Backend init should fail when IPC device is NULL");
	ipc_config.dev = DEVICE_DT_GET(FAKE_IPC_NODE);

	/* fail ipc_service_open_instance */
	fake_ipc_open_instance_fake.return_val = -ENODEV;
	ret = agent_config.backend_api->backend_init(&agent_config);
	zassert_equal(ret, -ENODEV,
		      "Backend init should fail when ipc_service_open_instance fails");
	fake_ipc_open_instance_fake.return_val = 0;

	/* fail ipc_service_register_endpoint */
	fake_ipc_register_endpoint_fake.return_val = -EINVAL;
	ret = agent_config.backend_api->backend_init(&agent_config);
	zassert_equal(ret, -EINVAL,
		      "Backend init should fail when ipc_service_register_endpoint fails");
	fake_ipc_register_endpoint_fake.return_val = 0;

	/* Schedule manual bound callback trigger while backend_init waits. */
	schedule_bound_callback(1);

	ret = agent_config.backend_api->backend_init(&agent_config);
	zassert_equal(ret, 0, "Backend init should succeed with valid config");

	/* second successful init call should be rejected */
	ret = agent_config.backend_api->backend_init(&agent_config);
	zassert_equal(ret, -EALREADY,
		      "Backend init should fail when backend is already initialized");
}

/* Test send */
ZTEST(ipc_backend, test_backend_send)
{
	int ret;
	struct zbus_proxy_agent_ipc_data ipc_data = {0};
	struct zbus_proxy_agent_ipc_config ipc_config = {
		.dev = DEVICE_DT_GET(FAKE_IPC_NODE),
		.ept_name = "test_ept",
		.data = &ipc_data,
	};
	char channel_name[] = "test_channel";
	uint8_t payload[] = {0x01, 0x02, 0x03, 0x04};
	struct zbus_proxy_msg msg = {0};
	struct zbus_proxy_msg sent_msg;

	struct zbus_proxy_agent agent_config = create_test_proxy_agent(&ipc_config);

	memcpy(msg.channel_name, channel_name, sizeof(channel_name));
	msg.channel_name_len = sizeof(channel_name);
	memcpy(msg.message, payload, sizeof(payload));
	msg.message_size = sizeof(payload);

	init_backend(&agent_config);

	ret = agent_config.backend_api->backend_send(NULL, &msg);
	zassert_equal(ret, -EFAULT, "Backend send should fail when config is NULL");

	ret = agent_config.backend_api->backend_send(&agent_config, NULL);
	zassert_equal(ret, -EFAULT, "Backend send should fail when message is NULL");

	ret = agent_config.backend_api->backend_send(&agent_config, &msg);
	zassert_equal(ret, 0, "Backend send should succeed with valid parameters");
	sent_msg = get_last_sent_message();
	zassert_equal(sent_msg.message_size, msg.message_size, "Sent message size should match");
	zassert_mem_equal(sent_msg.message, msg.message, msg.message_size,
			  "Sent message payload should match");
	zassert_equal(sent_msg.channel_name_len, msg.channel_name_len,
		      "Sent message channel name length should match");
	zassert_mem_equal(sent_msg.channel_name, msg.channel_name, msg.channel_name_len,
			  "Sent message channel name should match");

	/* fail ipc_service_send */
	fake_ipc_send_fake.return_val = -EBUSY;
	ret = agent_config.backend_api->backend_send(&agent_config, &msg);
	zassert_equal(ret, -EBUSY, "Backend send should fail when ipc_service_send fails");
	fake_ipc_send_fake.return_val = sizeof(struct zbus_proxy_msg) +
					1; /* Simulate invalid return value from ipc_service_send */
	ret = agent_config.backend_api->backend_send(&agent_config, &msg);
	zassert_equal(ret, -EIO,
		      "Backend send should fail when ipc_service_send returns invalid value");
	fake_ipc_send_fake.return_val = 0;

	/* Failure is transient */
	payload[0] = 0xFF;
	memcpy(msg.message, payload, sizeof(payload));
	ret = agent_config.backend_api->backend_send(&agent_config, &msg);
	zassert_equal(ret, 0, "Backend send should succeed with valid parameters");
	sent_msg = get_last_sent_message();
	zassert_equal(sent_msg.message_size, msg.message_size, "Sent message size should match");
	zassert_mem_equal(sent_msg.message, msg.message, msg.message_size,
			  "Sent message payload should match");
	zassert_equal(sent_msg.channel_name_len, msg.channel_name_len,
		      "Sent message channel name length should match");
	zassert_mem_equal(sent_msg.channel_name, msg.channel_name, msg.channel_name_len,
			  "Sent message channel name should match");
}

/* Test recv */
ZTEST(ipc_backend, test_backend_recv)
{
	struct zbus_proxy_agent_ipc_data ipc_data = {0};
	struct zbus_proxy_agent_ipc_config ipc_config = {
		.dev = DEVICE_DT_GET(FAKE_IPC_NODE),
		.ept_name = "test_ept",
		.data = &ipc_data,
	};
	char channel_name[] = "test_channel";
	uint8_t payload[] = {0x01, 0x02, 0x03, 0x04};
	struct zbus_proxy_msg msg = {0};
	struct zbus_proxy_msg recv_msg;

	memcpy(msg.channel_name, channel_name, sizeof(channel_name));
	msg.channel_name_len = sizeof(channel_name);
	memcpy(msg.message, payload, sizeof(payload));
	msg.message_size = sizeof(payload);

	struct zbus_proxy_agent agent_config = create_test_proxy_agent(&ipc_config);

	init_backend(&agent_config);

	/* Trigger receive callback with valid message */
	trigger_received_callback(&msg, sizeof(msg));
	zassert_equal(fake_recv_callback_fake.call_count, 1,
		      "Receive callback should be called once");
	zassert_equal(fake_recv_callback_fake.arg0_val, &agent_config,
		      "Receive callback should receive correct agent config pointer");
	recv_msg = *(const struct zbus_proxy_msg *)fake_recv_callback_fake.arg1_val;
	zassert_equal(recv_msg.message_size, msg.message_size,
		      "Received message size should match");
	zassert_mem_equal(recv_msg.message, msg.message, msg.message_size,
			  "Received message payload should match");
	zassert_equal(recv_msg.channel_name_len, msg.channel_name_len,
		      "Received message channel name length should match");
	zassert_mem_equal(recv_msg.channel_name, msg.channel_name, msg.channel_name_len,
			  "Received message channel name should match");

	/* Trigger receive callback with invalid data */
	trigger_received_callback(NULL, sizeof(msg));
	zassert_equal(fake_recv_callback_fake.call_count, 1,
		      "Receive callback should not be called when data is NULL");

	trigger_received_callback(&msg, 0);
	zassert_equal(fake_recv_callback_fake.call_count, 1,
		      "Receive callback should not be called when length is 0");

	trigger_received_callback(&msg, sizeof(msg) - 1);
	zassert_equal(fake_recv_callback_fake.call_count, 1,
		      "Receive callback should not be called when length is invalid");
}

ZTEST(ipc_backend, test_ipc_error)
{
	struct zbus_proxy_agent_ipc_data ipc_data = {0};
	struct zbus_proxy_agent_ipc_config ipc_config = {
		.dev = DEVICE_DT_GET(FAKE_IPC_NODE),
		.ept_name = "test_ept",
		.data = &ipc_data,
	};

	struct zbus_proxy_agent agent_config = create_test_proxy_agent(&ipc_config);

	init_backend(&agent_config);

	trigger_error_callback("Test error message");
	/* Since the error callback just logs the error it is verified from regex in testcase.yaml
	 */
}

static void test_setup(void *fixture)
{
	ARG_UNUSED(fixture);

	RESET_FAKE(fake_recv_callback);
	init_mock_ipc();
}

static void test_cleanup(void *fixture)
{
	ARG_UNUSED(fixture);

	deinit_mock_ipc();
}

ZTEST_SUITE(ipc_backend, NULL, NULL, test_setup, test_cleanup, NULL);
