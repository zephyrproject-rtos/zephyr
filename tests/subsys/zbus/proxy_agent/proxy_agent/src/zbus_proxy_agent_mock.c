/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zbus/proxy_agent/zbus_proxy_agent_types.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "zbus_proxy_agent_mock.h"

LOG_MODULE_REGISTER(proxy_agent_mock, LOG_LEVEL_INF);

static int current_ack_mode = MOCK_BACKEND_ACK_MODE_AUTO;
static uint32_t mock_send_count;
static bool mock_send_failure;
static uint32_t last_sent_message_id;
static uint8_t last_sent_message_buffer[512];
static size_t last_sent_message_size;

struct recv_cb_data {
	int (*recv_cb)(const uint8_t *data, size_t length, void *user_data);
	void *user_data;
};

struct recv_cb_data stored_recv_cb_data = {0};

void set_ack_mode(enum ack_mode mode)
{
	current_ack_mode = mode;
}

void trigger_ack(uint32_t msg_id)
{
	int ret;
	size_t ack_size;
	uint8_t ack_msg_buffer[32];
	struct zbus_proxy_agent_msg ack_msg = {0};

	if (stored_recv_cb_data.recv_cb) {
		ret = zbus_create_proxy_agent_ack_msg(&ack_msg, msg_id);
		if (ret < 0) {
			LOG_ERR("Failed to create ACK message: %d", ret);
			return;
		}
		ack_size =
			serialize_proxy_agent_msg(&ack_msg, ack_msg_buffer, sizeof(ack_msg_buffer));
		if (ack_size <= 0) {
			LOG_ERR("Failed to serialize ACK message");
			return;
		}
		ret = stored_recv_cb_data.recv_cb(ack_msg_buffer, ack_size,
						  stored_recv_cb_data.user_data);
		if (ret < 0) {
			LOG_ERR("Manual ACK callback failed: %d", ret);
		}
	}
}

void trigger_nack(uint32_t msg_id)
{
	int ret;
	size_t nack_size;
	uint8_t nack_msg_buffer[32];
	struct zbus_proxy_agent_msg nack_msg = {0};

	if (stored_recv_cb_data.recv_cb) {
		ret = zbus_create_proxy_agent_nack_msg(&nack_msg, msg_id);
		if (ret < 0) {
			LOG_ERR("Failed to create NACK message: %d", ret);
			return;
		}
		nack_size = serialize_proxy_agent_msg(&nack_msg, nack_msg_buffer,
						      sizeof(nack_msg_buffer));
		if (nack_size <= 0) {
			LOG_ERR("Failed to serialize NACK message");
			return;
		}
		ret = stored_recv_cb_data.recv_cb(nack_msg_buffer, nack_size,
						  stored_recv_cb_data.user_data);
		if (ret < 0) {
			LOG_ERR("Manual NACK callback failed: %d", ret);
		}
	}
}

void trigger_receive(uint8_t *data, size_t length)
{
	int ret;

	if (stored_recv_cb_data.recv_cb) {
		ret = stored_recv_cb_data.recv_cb(data, length, stored_recv_cb_data.user_data);
		if (ret < 0) {
			LOG_ERR("Manual receive callback failed: %d", ret);
		}
	}
}

uint32_t get_mock_backend_send_count(void)
{
	return mock_send_count;
}

void set_mock_backend_send_failure(bool failure)
{
	mock_send_failure = failure;
}

uint32_t get_last_sent_message_id(void)
{
	return last_sent_message_id;
}

void get_last_sent_message(uint8_t *data, size_t *length)
{
	*length = last_sent_message_size;
	memcpy(data, last_sent_message_buffer, last_sent_message_size);
}

uint32_t extract_msg_id(uint8_t *data, size_t length)
{
	if (length < sizeof(uint8_t) + sizeof(uint32_t)) {
		return -EINVAL;
	}
	size_t offset = 0;
	uint32_t msg_id;

	offset += sizeof(uint8_t); /* Skip type */
	memcpy(&msg_id, data + offset, sizeof(uint32_t));

	return msg_id;
}

void trigger_receive_message(struct zbus_proxy_agent_msg *msg)
{
	uint8_t buffer[512];
	size_t serialized_size;

	serialized_size = serialize_proxy_agent_msg(msg, buffer, sizeof(buffer));
	if (serialized_size > 0) {
		trigger_receive(buffer, serialized_size);
	} else {
		LOG_ERR("Failed to serialize message");
	}
}

/* Backend API implementation */
static int mock_backend_init(void *config)
{
	LOG_INF("Mock backend: Initialized with config name %s",
		((struct zbus_proxy_agent_mock_config *)config)->name);

	return 0;
}

static int mock_backend_send(void *config, uint8_t *data, size_t length)
{
	mock_send_count++;
	last_sent_message_id = extract_msg_id(data, length);
	memcpy(last_sent_message_buffer, data, length);
	last_sent_message_size = length;

	if (mock_send_failure) {
		LOG_ERR("Mock backend: Simulating send failure");
		return -EIO;
	}

	if (current_ack_mode == MOCK_BACKEND_ACK_MODE_AUTO) {
		trigger_ack(last_sent_message_id);
	}

	return 0;
}

static int mock_backend_set_recv_cb(void *config,
				    int (*recv_cb)(const uint8_t *data, size_t length,
						   void *user_data),
				    void *user_data)
{
	CHECKIF(recv_cb == NULL) {
		LOG_ERR("Invalid receive callback pointer");
		return -EINVAL;
	}

	stored_recv_cb_data.recv_cb = recv_cb;
	stored_recv_cb_data.user_data = user_data;

	LOG_DBG("Mock backend: Stored receive callback %p with user data %p", recv_cb, user_data);

	return 0;
}

/* Reset helper for tests */
void reset_mock_backend_counters(void)
{
	mock_send_count = 0;
	last_sent_message_id = 0;
	mock_send_failure = false;
}

const struct zbus_proxy_agent_backend_api zbus_proxy_agent_mock_backend_api = {
	.backend_init = mock_backend_init,
	.backend_send = mock_backend_send,
	.backend_set_recv_cb = mock_backend_set_recv_cb,
};
