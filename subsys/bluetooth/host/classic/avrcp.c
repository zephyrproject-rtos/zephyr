/** @file
 *  @brief Audio Video Remote Control Profile
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (C) 2024 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <errno.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/avrcp.h>
#include <zephyr/bluetooth/l2cap.h>

#include "host/hci_core.h"
#include "host/conn_internal.h"
#include "host/l2cap_internal.h"
#include "avctp_internal.h"
#include "avrcp_internal.h"

#define LOG_LEVEL CONFIG_BT_AVRCP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_avrcp);

struct bt_avrcp {
	struct bt_avctp session;
};

#define AVRCP_AVCTP(_avctp) CONTAINER_OF(_avctp, struct bt_avrcp, session)

static const struct bt_avrcp_cb *avrcp_cb;
static struct bt_avrcp avrcp_connection[CONFIG_BT_MAX_CONN];

static struct bt_avrcp *get_new_connection(struct bt_conn *conn)
{
	struct bt_avrcp *avrcp;

	if (!conn) {
		LOG_ERR("Invalid Input (err: %d)", -EINVAL);
		return NULL;
	}

	avrcp = &avrcp_connection[bt_conn_index(conn)];
	memset(avrcp, 0, sizeof(struct bt_avrcp));
	return avrcp;
}

/* The AVCTP L2CAP channel established */
static void avrcp_connected(struct bt_avctp *session)
{
	struct bt_avrcp *avrcp = AVRCP_AVCTP(session);

	if ((avrcp_cb != NULL) && (avrcp_cb->connected != NULL)) {
		avrcp_cb->connected(avrcp);
	}
}

/* The AVCTP L2CAP channel released */
static void avrcp_disconnected(struct bt_avctp *session)
{
	struct bt_avrcp *avrcp = AVRCP_AVCTP(session);

	if ((avrcp_cb != NULL) && (avrcp_cb->disconnected != NULL)) {
		avrcp_cb->disconnected(avrcp);
	}
}

static const struct bt_avctp_ops_cb avctp_ops = {
	.connected = avrcp_connected,
	.disconnected = avrcp_disconnected,
};

static int avrcp_accept(struct bt_conn *conn, struct bt_avctp **session)
{
	struct bt_avrcp *avrcp;

	avrcp = get_new_connection(conn);
	if (!avrcp) {
		return -ENOMEM;
	}

	*session = &(avrcp->session);
	avrcp->session.ops = &avctp_ops;

	LOG_DBG("session: %p", &(avrcp->session));

	return 0;
}

static struct bt_avctp_event_cb avctp_cb = {
	.accept = avrcp_accept,
};

int bt_avrcp_init(void)
{
	int err;

	/* Register event handlers with AVCTP */
	err = bt_avctp_register(&avctp_cb);
	if (err < 0) {
		LOG_ERR("AVRCP registration failed");
		return err;
	}

	LOG_DBG("AVRCP Initialized successfully.");
	return 0;
}

struct bt_avrcp *bt_avrcp_connect(struct bt_conn *conn)
{
	struct bt_avrcp *avrcp;
	int err;

	avrcp = get_new_connection(conn);
	if (!avrcp) {
		LOG_ERR("Cannot allocate memory");
		return NULL;
	}

	avrcp->session.ops = &avctp_ops;
	err = bt_avctp_connect(conn, &(avrcp->session));
	if (err < 0) {
		/* If error occurs, undo the saving and return the error */
		memset(avrcp, 0, sizeof(struct bt_avrcp));
		LOG_DBG("AVCTP Connect failed");
		return NULL;
	}

	LOG_DBG("Connection request sent");
	return avrcp;
}

int bt_avrcp_disconnect(struct bt_avrcp *avrcp)
{
	int err;

	err = bt_avctp_disconnect(&(avrcp->session));
	if (err < 0) {
		LOG_DBG("AVCTP Disconnect failed");
		return err;
	}

	return err;
}

int bt_avrcp_register_cb(const struct bt_avrcp_cb *cb)
{
	avrcp_cb = cb;
	return 0;
}
