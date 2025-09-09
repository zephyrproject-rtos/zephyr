/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZBUS_MULTIDOMAIN_MOCK_BACKEND_H_
#define ZBUS_MULTIDOMAIN_MOCK_BACKEND_H_

#include <zephyr/kernel.h>
#include <zephyr/fff.h>
#include <zephyr/zbus/multidomain/zbus_multidomain_types.h>

/* Define MOCK backend type as a simple token for macro concatenation */
enum test_zbus_multidomain_type {
	ZBUS_MULTIDOMAIN_TYPE_MOCK = 99
};

typedef int (*zbus_recv_cb_t)(const struct zbus_proxy_agent_msg *);
typedef int (*zbus_ack_cb_t)(uint32_t, void *);

/* Global state for auto-ACK functionality */
extern bool mock_backend_auto_ack_enabled;
extern zbus_ack_cb_t mock_backend_stored_ack_cb;
extern void *mock_backend_stored_ack_user_data;

DECLARE_FAKE_VALUE_FUNC1(int, mock_backend_init, void *);
DECLARE_FAKE_VALUE_FUNC2(int, mock_backend_send, void *, struct zbus_proxy_agent_msg *);
DECLARE_FAKE_VALUE_FUNC2(int, mock_backend_set_recv_cb, void *, zbus_recv_cb_t);
DECLARE_FAKE_VALUE_FUNC3(int, mock_backend_set_ack_cb, void *, zbus_ack_cb_t, void *);

/* Custom implementations for auto-ACK and callback storage */
int mock_backend_send_with_auto_ack(void *config, struct zbus_proxy_agent_msg *msg);
int mock_backend_set_ack_cb_with_storage(void *config, zbus_ack_cb_t ack_cb, void *user_data);
int mock_backend_set_recv_cb_with_storage(void *config, zbus_recv_cb_t recv_cb);

/* Helper to enable/disable auto-ACK */
void mock_backend_set_auto_ack(bool enabled);

/* Helper to manually send duplicate ACKs for testing */
void mock_backend_send_duplicate_ack(uint32_t msg_id);

/* Mock state management functions */
zbus_recv_cb_t mock_backend_get_stored_recv_cb(void);
void mock_backend_reset_callbacks(void);
bool mock_backend_has_recv_callback(void);

/* Test helper functions */
void mock_backend_create_test_message(struct zbus_proxy_agent_msg *msg, const char *channel_name,
				      const void *data, size_t data_size);

/* Get the copied message to avoid use-after-scope issues */
struct zbus_proxy_agent_msg *mock_backend_get_last_sent_message(void);

static struct zbus_proxy_agent_api mock_backend_api __attribute__((unused)) = {
	.backend_init = mock_backend_init,
	.backend_send = mock_backend_send_with_auto_ack,
	.backend_set_recv_cb = mock_backend_set_recv_cb_with_storage,
	.backend_set_ack_cb = mock_backend_set_ack_cb_with_storage,
};

struct zbus_multidomain_mock_config {
	char *nodeid;
};

/* Define the required macros for MOCK backend type */
#define _ZBUS_GENERATE_BACKEND_CONFIG_ZBUS_MULTIDOMAIN_TYPE_MOCK(_name, _nodeid)                   \
	static struct zbus_multidomain_mock_config _name##_backend_config = {.nodeid = _nodeid};

#define _ZBUS_GET_API_ZBUS_MULTIDOMAIN_TYPE_MOCK()         (&mock_backend_api)
#define _ZBUS_GET_CONFIG_ZBUS_MULTIDOMAIN_TYPE_MOCK(_name) ((void *)&_name##_backend_config)

#endif /* ZBUS_MULTIDOMAIN_MOCK_BACKEND_H_ */
