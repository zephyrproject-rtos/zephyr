/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zbus_multidomain_mock_backend.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mock_backend, LOG_LEVEL_DBG);

/* Define FFF globals */
DEFINE_FFF_GLOBALS;

/* Define fake function instances */
DEFINE_FAKE_VALUE_FUNC1(int, mock_backend_init, void *);
DEFINE_FAKE_VALUE_FUNC2(int, mock_backend_send, void *, struct zbus_proxy_agent_msg *);
DEFINE_FAKE_VALUE_FUNC2(int, mock_backend_set_recv_cb, void *, zbus_recv_cb_t);
DEFINE_FAKE_VALUE_FUNC3(int, mock_backend_set_ack_cb, void *, zbus_ack_cb_t, void *);

/* Global state for auto-ACK functionality */
bool mock_backend_auto_ack_enabled = true;
zbus_ack_cb_t mock_backend_stored_ack_cb;
void *mock_backend_stored_ack_user_data;

/* Global state for receive callback */
static zbus_recv_cb_t mock_backend_stored_recv_cb;

/* Store a copy of the last sent message to avoid use-after-scope issues */
static struct zbus_proxy_agent_msg last_sent_msg_copy;
static bool last_sent_msg_valid;

/* Custom send implementation that provides auto-ACK */
int mock_backend_send_with_auto_ack(void *config, struct zbus_proxy_agent_msg *msg)
{
	/* Create a copy of the message to avoid use-after-scope issues */
	if (msg) {
		memcpy(&last_sent_msg_copy, msg, sizeof(last_sent_msg_copy));
		last_sent_msg_valid = true;
	}

	LOG_DBG("Mock backend: Sending message ID %u on channel '%s'", msg ? msg->id : 0,
		msg ? msg->channel_name : "NULL");

	int ret = mock_backend_send(config, msg);

	/* If auto-ACK is enabled and ACK callback is stored, send ACK immediately */
	if (mock_backend_auto_ack_enabled && mock_backend_stored_ack_cb && msg) {
		LOG_DBG("Auto-ACK: Sending immediate ACK for message ID %u", msg->id);
		int ack_ret;

		ack_ret = mock_backend_stored_ack_cb(msg->id, mock_backend_stored_ack_user_data);
		LOG_DBG("Auto-ACK: ACK callback returned %d", ack_ret);
	}

	return ret;
}

/* Custom ACK callback setter that stores the callback for auto-ACK use */
int mock_backend_set_ack_cb_with_storage(void *config, zbus_ack_cb_t ack_cb, void *user_data)
{
	/* Store the callback and user data for auto-ACK functionality */
	mock_backend_stored_ack_cb = ack_cb;
	mock_backend_stored_ack_user_data = user_data;

	LOG_DBG("Mock backend: Stored ACK callback %p with user data %p", ack_cb, user_data);

	return mock_backend_set_ack_cb(config, ack_cb, user_data);
}

/* Helper function to enable/disable auto-ACK */
void mock_backend_set_auto_ack(bool enabled)
{
	mock_backend_auto_ack_enabled = enabled;
	LOG_DBG("Mock backend: Auto-ACK %s", enabled ? "enabled" : "disabled");
}

/* Helper function to manually send duplicate ACKs for testing */
void mock_backend_send_duplicate_ack(uint32_t msg_id)
{
	if (mock_backend_stored_ack_cb) {
		mock_backend_stored_ack_cb(msg_id, mock_backend_stored_ack_user_data);

		k_sleep(K_MSEC(1));

		mock_backend_stored_ack_cb(msg_id, mock_backend_stored_ack_user_data);
	}
}

/* Custom receive callback setter that stores the callback */
int mock_backend_set_recv_cb_with_storage(void *config, zbus_recv_cb_t recv_cb)
{
	if (!recv_cb) {
		LOG_ERR("Invalid receive callback pointer");
		return -EINVAL;
	}

	/* Store the callback for state management */
	mock_backend_stored_recv_cb = recv_cb;

	LOG_DBG("Mock backend: Stored receive callback %p", recv_cb);

	return mock_backend_set_recv_cb(config, recv_cb);
}

/* Mock state management functions */
zbus_recv_cb_t mock_backend_get_stored_recv_cb(void)
{
	return mock_backend_stored_recv_cb;
}

void mock_backend_reset_callbacks(void)
{
	mock_backend_stored_recv_cb = NULL;
	mock_backend_stored_ack_cb = NULL;
	mock_backend_stored_ack_user_data = NULL;
	last_sent_msg_valid = false;
	LOG_DBG("Mock backend: All callbacks reset");
}

bool mock_backend_has_recv_callback(void)
{
	return mock_backend_stored_recv_cb;
}

/* Get the copied message to avoid use-after-scope issues */
struct zbus_proxy_agent_msg *mock_backend_get_last_sent_message(void)
{
	return last_sent_msg_valid ? &last_sent_msg_copy : NULL;
}

/* Test helper function to create messages */
void mock_backend_create_test_message(struct zbus_proxy_agent_msg *msg, const char *channel_name,
				      const void *data, size_t data_size)
{
	if (!msg) {
		LOG_ERR("Invalid message pointer");
		return;
	}

	if (!channel_name || strlen(channel_name) == 0) {
		LOG_ERR("Invalid channel name");
		return;
	}

	if (!data && data_size > 0) {
		LOG_ERR("Invalid data pointer with non-zero size");
		return;
	}

	if (data_size > sizeof(msg->message_data)) {
		LOG_ERR("Data size %zu exceeds maximum %zu", data_size, sizeof(msg->message_data));
		return;
	}

	if (strlen(channel_name) >= sizeof(msg->channel_name)) {
		LOG_ERR("Channel name too long: %zu >= %zu", strlen(channel_name),
			sizeof(msg->channel_name));
		return;
	}

	memset(msg, 0, sizeof(*msg));
	msg->type = ZBUS_PROXY_AGENT_MSG_TYPE_MSG;
	msg->id = k_cycle_get_32();
	msg->message_size = data_size;

	if (data_size > 0) {
		memcpy(msg->message_data, data, data_size);
	}

	msg->channel_name_len = strlen(channel_name);
	strncpy(msg->channel_name, channel_name, sizeof(msg->channel_name) - 1);
	msg->channel_name[sizeof(msg->channel_name) - 1] = '\0';

	LOG_DBG("Created test message for channel '%s' with %zu bytes", channel_name, data_size);
}
