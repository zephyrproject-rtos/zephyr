/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zbus_proxy_agent_mock.h"
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(proxy_agent_mock, LOG_LEVEL_DBG);

/* Mock backend state for testing */
struct mock_state {
	int init_retval;
	int send_retval;
	int set_recv_cb_retval;
	uint32_t send_count;
};

static struct mock_state backend_mock_state = {
	.init_retval = 0,
	.send_retval = 0,
	.set_recv_cb_retval = 0,
	.send_count = 0,
};

void mock_backend_reset(void)
{
	backend_mock_state.init_retval = 0;
	backend_mock_state.send_retval = 0;
	backend_mock_state.set_recv_cb_retval = 0;
	backend_mock_state.send_count = 0;
}

void mock_backend_set_init_retval(int retval)
{
	backend_mock_state.init_retval = retval;
}

void mock_backend_set_send_retval(int retval)
{
	backend_mock_state.send_retval = retval;
}

void mock_backend_set_set_recv_cb_retval(int retval)
{
	backend_mock_state.set_recv_cb_retval = retval;
}

uint32_t mock_backend_get_send_count(void)
{
	return backend_mock_state.send_count;
}

int mock_backend_trigger_receive(const struct zbus_proxy_agent *config, struct zbus_proxy_msg *msg)
{
	const struct zbus_proxy_agent_mock_config *mock_config =
		(const struct zbus_proxy_agent_mock_config *)config->backend_config;

	if (mock_config->data != NULL && mock_config->data->recv_cb != NULL) {
		return mock_config->data->recv_cb(mock_config->data->recv_cb_config_ptr, msg);
	}

	return -ENOSYS;
}

static int zbus_proxy_agent_mock_backend_init(const struct zbus_proxy_agent *config)
{
	LOG_DBG("Mock backend init called, returning %d", backend_mock_state.init_retval);
	return backend_mock_state.init_retval;
}

static int zbus_proxy_agent_mock_backend_send(const struct zbus_proxy_agent *config,
					      struct zbus_proxy_msg *msg)
{
	if (backend_mock_state.send_retval == 0) {
		backend_mock_state.send_count++;
		LOG_DBG("Mock backend send: channel=%s, size=%u, count=%u", msg->channel_name,
			msg->message_size, backend_mock_state.send_count);
	} else {
		LOG_DBG("Mock backend send failing with %d", backend_mock_state.send_retval);
	}
	return backend_mock_state.send_retval;
}

static int zbus_proxy_agent_mock_backend_set_recv_cb(const struct zbus_proxy_agent *agent_config,
						     zbus_proxy_agent_recv_cb_t recv_cb)
{
	if (backend_mock_state.set_recv_cb_retval != 0) {
		LOG_DBG("Mock backend set_recv_cb failing with %d",
			backend_mock_state.set_recv_cb_retval);
		return backend_mock_state.set_recv_cb_retval;
	}

	_ZBUS_ASSERT(recv_cb != NULL, "Receive callback is NULL in set_recv_cb for MOCK backend");

	const struct zbus_proxy_agent_mock_config *mock_config =
		(const struct zbus_proxy_agent_mock_config *)agent_config->backend_config;

	_ZBUS_ASSERT(mock_config->data != NULL, "Mock backend data is NULL in set_recv_cb");

	mock_config->data->recv_cb = recv_cb;
	mock_config->data->recv_cb_config_ptr = agent_config;

	LOG_DBG("Mock backend set_recv_cb success");
	return 0;
}

const struct zbus_proxy_agent_backend_api zbus_proxy_agent_mock_backend_api = {
	.backend_init = zbus_proxy_agent_mock_backend_init,
	.backend_send = zbus_proxy_agent_mock_backend_send,
	.backend_set_recv_cb = zbus_proxy_agent_mock_backend_set_recv_cb,
};

ZBUS_PROXY_AGENT_BACKEND_DEFINE(zbus_proxy_agent_mock_backend_desc, ZBUS_PROXY_AGENT_MOCK,
				&zbus_proxy_agent_mock_backend_api);
