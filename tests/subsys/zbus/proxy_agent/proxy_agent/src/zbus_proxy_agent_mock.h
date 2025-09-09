/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZBUS_PROXY_AGENT_MOCK_H_
#define ZEPHYR_INCLUDE_ZBUS_PROXY_AGENT_MOCK_H_

#include <zephyr/kernel.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent.h>
#include <zephyr/zbus/proxy_agent/zbus_proxy_agent_types.h>

enum ack_mode {
	MOCK_BACKEND_ACK_MODE_AUTO,
	MOCK_BACKEND_ACK_MODE_MANUAL,
};

/* Helper functions for testing */
void set_ack_mode(enum ack_mode mode);
void trigger_ack(uint32_t msg_id);
void trigger_nack(uint32_t msg_id);
void trigger_receive(uint8_t *data, size_t length);
uint32_t get_mock_backend_send_count(void);
void set_mock_backend_send_failure(bool failure);
uint32_t get_last_sent_message_id(void);
void get_last_sent_message(uint8_t *data, size_t *length);
void trigger_receive_message(struct zbus_proxy_agent_msg *msg);
void reset_mock_backend_counters(void);

extern const struct zbus_proxy_agent_backend_api zbus_proxy_agent_mock_backend_api;

struct zbus_proxy_agent_mock_config {
	char name[20]; /* Name of the mock proxy agent, used to validate config generation */
};

enum test_zbus_proxy_agent_type {
	ZBUS_PROXY_AGENT_TYPE_MOCK = 99
};

#define ZBUS_PROXY_AGENT_INSTANTIATE_MOCK(node_id)                                                 \
	ZBUS_PROXY_AGENT_DEFINE(node_id, zbus_##node_id, ZBUS_PROXY_AGENT_TYPE_MOCK)

#define _ZBUS_GET_BACKEND_API_ZBUS_PROXY_AGENT_TYPE_MOCK() &zbus_proxy_agent_mock_backend_api
#define _ZBUS_GET_CONFIG_ZBUS_PROXY_AGENT_TYPE_MOCK(_name) (void *)&_name##_mock_config

#define _ZBUS_GENERATE_BACKEND_CONFIG_ZBUS_PROXY_AGENT_TYPE_MOCK(_name, _node_id)                  \
	static struct zbus_proxy_agent_mock_config _name##_mock_config = {                         \
		.name = STRINGIFY(_node_id),                                                       \
	};

#endif /* ZEPHYR_INCLUDE_ZBUS_PROXY_AGENT_MOCK_H_ */
