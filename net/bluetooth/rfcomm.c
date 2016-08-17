/* rfcomm.c - RFCOMM handling */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <nanokernel.h>
#include <arch/cpu.h>
#include <toolchain.h>
#include <string.h>
#include <errno.h>
#include <atomic.h>
#include <misc/byteorder.h>
#include <misc/util.h>

#include <bluetooth/log.h>
#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/driver.h>
#include <bluetooth/l2cap.h>

#include "hci_core.h"
#include "conn_internal.h"
#include "l2cap_internal.h"
#include "rfcomm_internal.h"

#if !defined(CONFIG_BLUETOOTH_DEBUG_RFCOMM)
#undef BT_DBG
#define BT_DBG(fmt, ...)
#endif

#define RFCOMM_DEFAULT_MTU	127

#define RFCOMM_SESSION(_ch) CONTAINER_OF(_ch, \
					 struct bt_rfcomm_session, br_chan.chan)

static struct bt_rfcomm_session bt_rfcomm_pool[CONFIG_BLUETOOTH_MAX_CONN];

static void rfcomm_connected(struct bt_l2cap_chan *chan)
{
	BT_DBG("Session %p", RFCOMM_SESSION(chan));
}

static void rfcomm_disconnected(struct bt_l2cap_chan *chan)
{
	BT_DBG("Session %p", RFCOMM_SESSION(chan));
}

static void rfcomm_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
}

static int rfcomm_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	int i;
	static struct bt_l2cap_chan_ops ops = {
		.connected = rfcomm_connected,
		.disconnected = rfcomm_disconnected,
		.recv = rfcomm_recv,
	};

	BT_DBG("conn %p handle %u", conn, conn->handle);

	for (i = 0; i < ARRAY_SIZE(bt_rfcomm_pool); i++) {
		struct bt_rfcomm_session *session = &bt_rfcomm_pool[i];

		if (session->br_chan.chan.conn) {
			continue;
		}

		BT_DBG("session %p initialized", session);

		session->br_chan.chan.ops = &ops;
		session->br_chan.rx.mtu	= RFCOMM_DEFAULT_MTU;
		session->state = BT_RFCOMM_STATE_INIT;

		*chan = &session->br_chan.chan;
		return 0;
	}

	BT_ERR("No available RFCOMM context for conn %p", conn);

	return -ENOMEM;
}

void bt_rfcomm_init(void)
{
	static struct bt_l2cap_server server = {
		.psm	= BT_L2CAP_PSM_RFCOMM,
		.accept = rfcomm_accept,
	};

	bt_l2cap_br_server_register(&server);
}
