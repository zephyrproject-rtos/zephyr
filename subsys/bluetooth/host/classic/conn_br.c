/* conn_br.c - Bluetooth Classic connection handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/iso.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/slist.h>
#include <zephyr/debug/stack.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/direction.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci_vs.h>
#include <zephyr/bluetooth/att.h>

#include "common/assert.h"
#include "common/bt_str.h"

#include "host/conn_internal.h"
#include "host/l2cap_internal.h"
#include "host/keys.h"
#include "host/smp.h"
#include "ssp.h"
#include "sco_internal.h"

#define LOG_LEVEL CONFIG_BT_CONN_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_conn_br);

void bt_sco_cleanup(struct bt_conn *sco_conn)
{
	bt_sco_cleanup_acl(sco_conn);
	bt_conn_unref(sco_conn);
}

struct bt_conn *bt_conn_create_br(const bt_addr_t *peer,
				  const struct bt_br_conn_param *param)
{
	struct bt_hci_cp_connect *cp;
	struct bt_conn *conn;
	struct net_buf *buf;

	conn = bt_conn_lookup_addr_br(peer);
	if (conn) {
		switch (conn->state) {
		case BT_CONN_INITIATING:
		case BT_CONN_CONNECTED:
			return conn;
		default:
			bt_conn_unref(conn);
			return NULL;
		}
	}

	conn = bt_conn_add_br(peer);
	if (!conn) {
		return NULL;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_CONNECT, sizeof(*cp));
	if (!buf) {
		bt_conn_unref(conn);
		return NULL;
	}

	cp = net_buf_add(buf, sizeof(*cp));

	(void)memset(cp, 0, sizeof(*cp));

	memcpy(&cp->bdaddr, peer, sizeof(cp->bdaddr));
	cp->packet_type = sys_cpu_to_le16(0xcc18); /* DM1 DH1 DM3 DH5 DM5 DH5 */
	cp->pscan_rep_mode = 0x02; /* R2 */
	cp->allow_role_switch = param->allow_role_switch ? 0x01 : 0x00;
	cp->clock_offset = 0x0000; /* TODO used cached clock offset */

	if (bt_hci_cmd_send_sync(BT_HCI_OP_CONNECT, buf, NULL) < 0) {
		bt_conn_unref(conn);
		return NULL;
	}

	bt_conn_set_state(conn, BT_CONN_INITIATING);
	conn->role = BT_CONN_ROLE_CENTRAL;

	return conn;
}

int bt_hci_connect_br_cancel(struct bt_conn *conn)
{
	struct bt_hci_cp_connect_cancel *cp;
	struct bt_hci_rp_connect_cancel *rp;
	struct net_buf *buf, *rsp;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_CONNECT_CANCEL, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	memcpy(&cp->bdaddr, &conn->br.dst, sizeof(cp->bdaddr));

	err = bt_hci_cmd_send_sync(BT_HCI_OP_CONNECT_CANCEL, buf, &rsp);
	if (err) {
		return err;
	}

	rp = (void *)rsp->data;

	err = rp->status ? -EIO : 0;

	net_buf_unref(rsp);

	return err;
}

const bt_addr_t *bt_conn_get_dst_br(const struct bt_conn *conn)
{
	if (conn == NULL) {
		LOG_DBG("Invalid connect");
		return NULL;
	}

	if (!bt_conn_is_type(conn, BT_CONN_TYPE_BR)) {
		LOG_DBG("Invalid connection type: %u for %p", conn->type, conn);
		return NULL;
	}

	return &conn->br.dst;
}
