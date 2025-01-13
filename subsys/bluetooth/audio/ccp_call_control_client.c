/* Bluetooth CCP - Call Control Profile Call Control Server
 *
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/tbs.h>
#include <zephyr/bluetooth/audio/ccp.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

LOG_MODULE_REGISTER(bt_ccp_call_control_client, CONFIG_BT_CCP_CALL_CONTROL_CLIENT_LOG_LEVEL);

static sys_slist_t ccp_call_control_client_cbs =
	SYS_SLIST_STATIC_INIT(&ccp_call_control_client_cbs);

static struct bt_tbs_client_cb tbs_client_cbs;

/* A service instance can either be a GTBS or a TBS insttance */
struct bt_ccp_call_control_client_bearer {
	uint8_t tbs_index;
	bool discovered;
};

enum ccp_call_control_client_flag {
	CCP_CALL_CONTROL_CLIENT_FLAG_BUSY,

	CCP_CALL_CONTROL_CLIENT_FLAG_NUM_FLAGS, /* keep as last */
};

struct bt_ccp_call_control_client {
	struct bt_ccp_call_control_client_bearer
		bearers[CONFIG_BT_CCP_CALL_CONTROL_CLIENT_BEARER_COUNT];
	struct bt_conn *conn;

	ATOMIC_DEFINE(flags, CCP_CALL_CONTROL_CLIENT_FLAG_NUM_FLAGS);
};

static struct bt_ccp_call_control_client clients[CONFIG_BT_MAX_CONN];

static struct bt_ccp_call_control_client *get_client_by_conn(const struct bt_conn *conn)
{
	return &clients[bt_conn_index(conn)];
}

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	static bool cbs_registered;

	/* We register the callbacks in the connected callback. That way we ensure that they are
	 * registered before any procedures are completed or we receive any notifications, while
	 * registering them as late as possible
	 */
	if (err == BT_HCI_ERR_SUCCESS && !cbs_registered) {
		int cb_err;

		cb_err = bt_tbs_client_register_cb(&tbs_client_cbs);
		__ASSERT(cb_err == 0, "Failed to register TBS callbacks: %d", cb_err);

		cbs_registered = true;
	}
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	struct bt_ccp_call_control_client *client = get_client_by_conn(conn);

	/* client->conn may be NULL */
	if (client->conn == conn) {
		bt_conn_unref(client->conn);
		client->conn = NULL;
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
};

static void populate_bearers(struct bt_ccp_call_control_client *client,
			     struct bt_ccp_call_control_client_bearers *bearers)
{
	size_t i = 0;

#if defined(CONFIG_BT_TBS_CLIENT_GTBS)
	if (client->bearers[i].discovered) {
		bearers->gtbs_bearer = &client->bearers[i++];
	}
#endif /* CONFIG_BT_TBS_CLIENT_GTBS */

#if defined(CONFIG_BT_TBS_CLIENT_TBS)
	for (; i < ARRAY_SIZE(client->bearers); i++) {
		if (!client->bearers[i].discovered) {
			break;
		}

		bearers->tbs_bearers[bearers->tbs_count++] = &client->bearers[i];
	}
#endif /* CONFIG_BT_TBS_CLIENT_TBS */
}

static void tbs_client_discover_cb(struct bt_conn *conn, int err, uint8_t tbs_count,
				   bool gtbs_found)
{
	struct bt_ccp_call_control_client *client = get_client_by_conn(conn);
	struct bt_ccp_call_control_client_bearers bearers = {0};
	struct bt_ccp_call_control_client_cb *listener, *next;

	LOG_DBG("conn %p err %d tbs_count %u gtbs_found %d", (void *)conn, err, tbs_count,
		gtbs_found);

	memset(client->bearers, 0, sizeof((client->bearers)));

	if (IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS) && gtbs_found) {
		client->bearers[0].discovered = true;
		client->bearers[0].tbs_index = BT_TBS_GTBS_INDEX;
	}

	if (IS_ENABLED(CONFIG_BT_TBS_CLIENT_TBS)) {
		for (uint8_t i = 0U; i < tbs_count; i++) {
			const uint8_t idx = i + (gtbs_found ? 1 : 0);

			if (idx >= ARRAY_SIZE(client->bearers)) {
				LOG_WRN("Discoverd more TBS instances (%u) than the CCP Call "
					"Control Client supports %zu",
					tbs_count, ARRAY_SIZE(client->bearers));
				break;
			}

			client->bearers[idx].discovered = true;
			client->bearers[idx].tbs_index = i;
		}
	}

	populate_bearers(client, &bearers);

	atomic_clear_bit(client->flags, CCP_CALL_CONTROL_CLIENT_FLAG_BUSY);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&ccp_call_control_client_cbs, listener, next, _node) {
		if (listener->discover != NULL) {
			listener->discover(client, err, &bearers);
		}
	}
}

int bt_ccp_call_control_client_discover(struct bt_conn *conn,
					struct bt_ccp_call_control_client **out_client)
{
	struct bt_ccp_call_control_client *client;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	CHECKIF(out_client == NULL) {
		LOG_DBG("client is NULL");

		return -EINVAL;
	}

	client = get_client_by_conn(conn);
	if (atomic_test_and_set_bit(client->flags, CCP_CALL_CONTROL_CLIENT_FLAG_BUSY)) {
		return -EBUSY;
	}

	tbs_client_cbs.discover = tbs_client_discover_cb;

	err = bt_tbs_client_discover(conn);
	if (err != 0) {
		LOG_DBG("Failed to discover TBS for %p: %d", (void *)conn, err);

		atomic_clear_bit(client->flags, CCP_CALL_CONTROL_CLIENT_FLAG_BUSY);

		/* Return known errors */
		if (err == -EBUSY || err == -ENOTCONN) {
			return err;
		}

		return -ENOEXEC;
	}

	client->conn = bt_conn_ref(conn);
	*out_client = client;

	return 0;
}

int bt_ccp_call_control_client_register_cb(struct bt_ccp_call_control_client_cb *cb)
{
	CHECKIF(cb == NULL) {
		LOG_DBG("cb is NULL");

		return -EINVAL;
	}

	if (sys_slist_find(&ccp_call_control_client_cbs, &cb->_node, NULL)) {
		return -EEXIST;
	}

	sys_slist_append(&ccp_call_control_client_cbs, &cb->_node);

	return 0;
}

int bt_ccp_call_control_client_unregister_cb(struct bt_ccp_call_control_client_cb *cb)
{
	CHECKIF(cb == NULL) {
		LOG_DBG("cb is NULL");
		return -EINVAL;
	}

	if (!sys_slist_find_and_remove(&ccp_call_control_client_cbs, &cb->_node)) {
		return -EALREADY;
	}

	return 0;
}
