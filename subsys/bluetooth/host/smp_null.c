/**
 * @file smp_null.c
 * Security Manager Protocol stub
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <errno.h>
#include <sys/atomic.h>
#include <sys/util.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/buf.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_CORE)
#define LOG_MODULE_NAME bt_smp
#include "common/log.h"

#include "hci_core.h"
#include "conn_internal.h"
#include "l2cap_internal.h"
#include "smp.h"

int bt_smp_sign_verify(struct bt_conn *conn, struct net_buf *buf)
{
	return -ENOTSUP;
}

int bt_smp_sign(struct bt_conn *conn, struct net_buf *buf)
{
	return -ENOTSUP;
}

static void bt_smp_recv(struct bt_conn *conn, struct net_buf *req_buf)
{
	struct bt_smp_pairing_fail *rsp;
	struct bt_smp_hdr *hdr;
	struct net_buf *buf;

	ARG_UNUSED(req_buf);

	/* If a device does not support pairing then it shall respond with
	 * a Pairing Failed command with the reason set to "Pairing Not
	 * Supported" when any command is received.
	 * Core Specification Vol. 3, Part H, 3.3
	 */

	buf = bt_l2cap_create_pdu(NULL, 0);
	/* NULL is not a possible return due to K_FOREVER */

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_SMP_CMD_PAIRING_FAIL;

	rsp = net_buf_add(buf, sizeof(*rsp));
	rsp->reason = BT_SMP_ERR_PAIRING_NOTSUPP;

	if (bt_l2cap_send(conn, BT_L2CAP_CID_SMP, buf)) {
		net_buf_unref(buf);
	}
}

BT_L2CAP_CHANNEL_MONITOR(smp) = {
	.cid = BT_L2CAP_CID_SMP,
	.recv = bt_smp_recv,
};

int bt_smp_init(void)
{
	return 0;
}
