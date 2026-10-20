/* att_br.c - Attribute Protocol bearer over BR/EDR */

/*
 * Copyright 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/logging/log.h>

#include "host/att_internal.h"
#include "host/conn_internal.h"

#define LOG_LEVEL CONFIG_BT_ATT_LOG_LEVEL
LOG_MODULE_REGISTER(bt_att_br);

static int att_br_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
			 struct bt_l2cap_chan **chan)
{
	ARG_UNUSED(server);

	return bt_att_br_chan_accept(conn, chan);
}

/* Core v6.2 Vol 3 Part G 5.1.2: the channel shall be encrypted */
static struct bt_l2cap_server att_br_server = {
	.psm = BT_ATT_PSM,
	.sec_level = BT_SECURITY_L2,
	.accept = att_br_accept,
};

int bt_att_br_connect(struct bt_conn *conn)
{
	if (conn == NULL || !bt_conn_is_br(conn)) {
		return -EINVAL;
	}

	if (conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	return bt_att_br_chan_connect(conn);
}

int bt_att_br_disconnect(struct bt_conn *conn)
{
	if (conn == NULL || !bt_conn_is_br(conn)) {
		return -EINVAL;
	}

	return bt_att_br_chan_disconnect(conn);
}

void bt_att_br_init(void)
{
	int err;

	err = bt_l2cap_br_server_register(&att_br_server);
	if (err != 0) {
		LOG_ERR("ATT L2CAP server register failed (%d)", err);
	}
}
