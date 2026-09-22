/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZBUS_PROXY_AGENT_MOCK_H_
#define ZEPHYR_INCLUDE_ZBUS_PROXY_AGENT_MOCK_H_

#include <zephyr/kernel.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent.h>

struct zbus_proxy_agent_mock_config {
	/* Name of the mock proxy agent, used to validate config generation */
	char name[20];

	/* Pointer to mutable mock backend runtime data */
	struct zbus_proxy_agent_mock_data *data;
};

struct zbus_proxy_agent_mock_data {
	/* Callback function for simulating received messages in tests */
	zbus_proxy_agent_recv_cb_t recv_cb;
	/* pointer to proxy agent config, used as user data for the receive callback */
	const struct zbus_proxy_agent *recv_cb_config_ptr;
};

enum zbus_proxy_agent_backend_test_type {
	ZBUS_PROXY_AGENT_MOCK = 99
};

/* Helper functions */
void mock_backend_reset(void);
void mock_backend_set_init_retval(int retval);
void mock_backend_set_send_retval(int retval);
void mock_backend_set_set_recv_cb_retval(int retval);
uint32_t mock_backend_get_send_count(void);
int mock_backend_trigger_receive(const struct zbus_proxy_agent *config, struct zbus_proxy_msg *msg);

/* backend config creation for MOCK type */
#define _ZBUS_PROXY_AGENT_BACKEND_CONFIG_DEFINE_ZBUS_PROXY_AGENT_MOCK(_name, _backend_dt_node)     \
	static struct zbus_proxy_agent_mock_data _name##_mock_data;                                \
	static struct zbus_proxy_agent_mock_config _name##_mock_config = {                         \
		.name = STRINGIFY(_name), .data = &_name##_mock_data,                              \
	};

/* backend config getter for MOCK type */
#define _ZBUS_PROXY_AGENT_GET_BACKEND_CONFIG_ZBUS_PROXY_AGENT_MOCK(_name) (&_name##_mock_config)

/* Forward declaration and backend API getter for MOCK type */
extern const struct zbus_proxy_agent_backend_api zbus_proxy_agent_mock_backend_api;

#define _ZBUS_PROXY_AGENT_GET_BACKEND_API_ZBUS_PROXY_AGENT_MOCK (&zbus_proxy_agent_mock_backend_api)

#endif /* ZEPHYR_INCLUDE_ZBUS_PROXY_AGENT_MOCK_H_ */
