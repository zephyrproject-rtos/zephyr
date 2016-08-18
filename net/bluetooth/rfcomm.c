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
#include <bluetooth/rfcomm.h>

#include "hci_core.h"
#include "conn_internal.h"
#include "l2cap_internal.h"
#include "rfcomm_internal.h"

#if !defined(CONFIG_BLUETOOTH_DEBUG_RFCOMM)
#undef BT_DBG
#define BT_DBG(fmt, ...)
#endif

#define RFCOMM_CHANNEL_START	0x01
#define RFCOMM_CHANNEL_END	0x1e

#define RFCOMM_MIN_MTU		BT_RFCOMM_SIG_MIN_MTU
#define RFCOMM_DEFAULT_MTU	127

static struct bt_rfcomm_server *servers;

/* Pool for outgoing RFCOMM control packets, min MTU is 23 */
static struct nano_fifo rfcomm_session;
static NET_BUF_POOL(rfcomm_session_pool, CONFIG_BLUETOOTH_MAX_CONN,
		    BT_RFCOMM_BUF_SIZE(RFCOMM_MIN_MTU), &rfcomm_session, NULL,
		    BT_BUF_USER_DATA_MIN);

#define RFCOMM_SESSION(_ch) CONTAINER_OF(_ch, \
					 struct bt_rfcomm_session, br_chan.chan)

static struct bt_rfcomm_session bt_rfcomm_pool[CONFIG_BLUETOOTH_MAX_CONN];

/* reversed, 8-bit, poly=0x07 */
static const uint8_t rfcomm_crc_table[256] = {
	0x00, 0x91, 0xe3, 0x72, 0x07, 0x96, 0xe4, 0x75,
	0x0e, 0x9f, 0xed, 0x7c, 0x09, 0x98, 0xea, 0x7b,
	0x1c, 0x8d, 0xff, 0x6e, 0x1b, 0x8a, 0xf8, 0x69,
	0x12, 0x83, 0xf1, 0x60, 0x15, 0x84, 0xf6, 0x67,

	0x38, 0xa9, 0xdb, 0x4a, 0x3f, 0xae, 0xdc, 0x4d,
	0x36, 0xa7, 0xd5, 0x44, 0x31, 0xa0, 0xd2, 0x43,
	0x24, 0xb5, 0xc7, 0x56, 0x23, 0xb2, 0xc0, 0x51,
	0x2a, 0xbb, 0xc9, 0x58, 0x2d, 0xbc, 0xce, 0x5f,

	0x70, 0xe1, 0x93, 0x02, 0x77, 0xe6, 0x94, 0x05,
	0x7e, 0xef, 0x9d, 0x0c, 0x79, 0xe8, 0x9a, 0x0b,
	0x6c, 0xfd, 0x8f, 0x1e, 0x6b, 0xfa, 0x88, 0x19,
	0x62, 0xf3, 0x81, 0x10, 0x65, 0xf4, 0x86, 0x17,

	0x48, 0xd9, 0xab, 0x3a, 0x4f, 0xde, 0xac, 0x3d,
	0x46, 0xd7, 0xa5, 0x34, 0x41, 0xd0, 0xa2, 0x33,
	0x54, 0xc5, 0xb7, 0x26, 0x53, 0xc2, 0xb0, 0x21,
	0x5a, 0xcb, 0xb9, 0x28, 0x5d, 0xcc, 0xbe, 0x2f,

	0xe0, 0x71, 0x03, 0x92, 0xe7, 0x76, 0x04, 0x95,
	0xee, 0x7f, 0x0d, 0x9c, 0xe9, 0x78, 0x0a, 0x9b,
	0xfc, 0x6d, 0x1f, 0x8e, 0xfb, 0x6a, 0x18, 0x89,
	0xf2, 0x63, 0x11, 0x80, 0xf5, 0x64, 0x16, 0x87,

	0xd8, 0x49, 0x3b, 0xaa, 0xdf, 0x4e, 0x3c, 0xad,
	0xd6, 0x47, 0x35, 0xa4, 0xd1, 0x40, 0x32, 0xa3,
	0xc4, 0x55, 0x27, 0xb6, 0xc3, 0x52, 0x20, 0xb1,
	0xca, 0x5b, 0x29, 0xb8, 0xcd, 0x5c, 0x2e, 0xbf,

	0x90, 0x01, 0x73, 0xe2, 0x97, 0x06, 0x74, 0xe5,
	0x9e, 0x0f, 0x7d, 0xec, 0x99, 0x08, 0x7a, 0xeb,
	0x8c, 0x1d, 0x6f, 0xfe, 0x8b, 0x1a, 0x68, 0xf9,
	0x82, 0x13, 0x61, 0xf0, 0x85, 0x14, 0x66, 0xf7,

	0xa8, 0x39, 0x4b, 0xda, 0xaf, 0x3e, 0x4c, 0xdd,
	0xa6, 0x37, 0x45, 0xd4, 0xa1, 0x30, 0x42, 0xd3,
	0xb4, 0x25, 0x57, 0xc6, 0xb3, 0x22, 0x50, 0xc1,
	0xba, 0x2b, 0x59, 0xc8, 0xbd, 0x2c, 0x5e, 0xcf
};

static uint8_t rfcomm_calc_fcs(uint16_t len, const uint8_t *data)
{
	uint8_t fcs = 0xff;

	while (len--) {
		fcs = rfcomm_crc_table[fcs ^ *data++];
	}

	/* Ones compliment */
	return (0xff - fcs);
}

static bool rfcomm_check_fcs(uint16_t len, const uint8_t *data,
			     uint8_t recvd_fcs)
{
	uint8_t fcs = 0xff;

	while (len--) {
		fcs = rfcomm_crc_table[fcs ^ *data++];
	}

	/* Ones compliment */
	fcs = rfcomm_crc_table[fcs ^ recvd_fcs];

	/*0xCF is the reversed order of 11110011.*/
	return (fcs == 0xcf);
}

static struct bt_rfcomm_server *rfcomm_server_lookup_channel(uint8_t channel)
{
	struct bt_rfcomm_server *server;

	for (server = servers; server; server = server->_next) {
		if (server->channel == channel) {
			return server;
		}
	}

	return NULL;
}

int bt_rfcomm_server_register(struct bt_rfcomm_server *server)
{
	if (server->channel < RFCOMM_CHANNEL_START ||
	    server->channel > RFCOMM_CHANNEL_END || !server->accept) {
		return -EINVAL;
	}

	/* Check if given channel is already in use */
	if (rfcomm_server_lookup_channel(server->channel)) {
		BT_DBG("Channel already registered");
		return -EADDRINUSE;
	}

	BT_DBG("Channel 0x%02x", server->channel);

	server->_next = servers;
	servers = server;

	return 0;
}

static void rfcomm_connected(struct bt_l2cap_chan *chan)
{
	struct bt_rfcomm_session *session = RFCOMM_SESSION(chan);

	BT_DBG("Session %p", session);

	/* Need to include UIH header and FCS*/
	session->mtu = min(session->br_chan.rx.mtu,
			   session->br_chan.tx.mtu) -
			   BT_RFCOMM_HDR_SIZE + BT_RFCOMM_FCS_SIZE;
}

static void rfcomm_disconnected(struct bt_l2cap_chan *chan)
{
	BT_DBG("Session %p", RFCOMM_SESSION(chan));
}

static int rfcomm_send_ua(struct bt_rfcomm_session *session, uint8_t dlci)
{
	struct bt_rfcomm_hdr *hdr;
	struct net_buf *buf;
	uint8_t cr, fcs;

	buf = bt_l2cap_create_pdu(&rfcomm_session);
	if (!buf) {
		BT_ERR("No buffers");
		return -ENOMEM;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));
	cr = session->initiator ? 0 : 1;
	hdr->address = BT_RFCOMM_SET_ADDR(dlci, cr);
	hdr->control = BT_RFCOMM_SET_CTRL(BT_RFCOMM_UA, 1);
	hdr->length = BT_RFCOMM_SET_LEN_8(0);

	fcs = rfcomm_calc_fcs(BT_RFCOMM_FCS_LEN_NON_UIH, buf->data);
	net_buf_add_u8(buf, fcs);

	return bt_l2cap_chan_send(&session->br_chan.chan, buf);
}

static void rfcomm_handle_sabm(struct bt_rfcomm_session *session, uint8_t dlci)
{
	if (!dlci) {
		session->initiator = false;

		if (rfcomm_send_ua(session, dlci) < 0) {
			return;
		}

		session->state = BT_RFCOMM_STATE_CONNECTED;
	}
}

static void rfcomm_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_rfcomm_session *session = RFCOMM_SESSION(chan);
	struct bt_rfcomm_hdr *hdr = (void *)buf->data;
	uint8_t dlci, frame_type, fcs, fcs_len;

	/* Need to consider FCS also*/
	if (buf->len < (sizeof(*hdr) + 1)) {
		BT_ERR("Too small RFCOMM Frame");
		return;
	}

	dlci = BT_RFCOMM_GET_DLCI(hdr->address);
	frame_type = BT_RFCOMM_GET_FRAME_TYPE(hdr->control);

	BT_DBG("session %p dlci %x type %x", session, dlci, frame_type);

	fcs_len = (frame_type == BT_RFCOMM_UIH) ? BT_RFCOMM_FCS_LEN_UIH :
		   BT_RFCOMM_FCS_LEN_NON_UIH;
	fcs = *(net_buf_tail(buf) - 1);
	if (!rfcomm_check_fcs(fcs_len, buf->data, fcs)) {
		BT_ERR("FCS check failed");
		return;
	}

	switch (frame_type) {
	case BT_RFCOMM_SABM:
		rfcomm_handle_sabm(session, dlci);
		break;
	default:
		BT_WARN("Unknown/Unsupported RFCOMM Frame type 0x%02x",
			frame_type);
		break;
	}
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

	net_buf_pool_init(rfcomm_session_pool);
	bt_l2cap_br_server_register(&server);
}
