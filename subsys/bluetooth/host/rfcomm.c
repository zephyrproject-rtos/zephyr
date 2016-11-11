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

#include <zephyr.h>
#include <string.h>
#include <errno.h>
#include <atomic.h>
#include <misc/byteorder.h>
#include <misc/util.h>

#include <bluetooth/log.h>
#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/hci_driver.h>
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

#define RFCOMM_MAX_CREDITS		(CONFIG_BLUETOOTH_ACL_IN_COUNT - 1)
#define RFCOMM_CREDITS_THRESHOLD	(RFCOMM_MAX_CREDITS / 2)
#define RFCOMM_DEFAULT_CREDIT		RFCOMM_MAX_CREDITS

static struct bt_rfcomm_server *servers;

/* Pool for outgoing RFCOMM control packets, min MTU is 23 */
static struct k_fifo rfcomm_session;
static NET_BUF_POOL(rfcomm_session_pool, CONFIG_BLUETOOTH_MAX_CONN,
		    BT_RFCOMM_BUF_SIZE(RFCOMM_MIN_MTU), &rfcomm_session, NULL,
		    BT_BUF_USER_DATA_MIN);

/* Pool for dummy buffers to wake up the tx threads */
static struct k_fifo dummy;
static NET_BUF_POOL(dummy_pool, CONFIG_BLUETOOTH_MAX_CONN, 0, &dummy, NULL, 0);

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

static struct bt_rfcomm_dlc *rfcomm_dlcs_lookup_dlci(struct bt_rfcomm_dlc *dlcs,
						     uint8_t dlci)
{
	for (; dlcs; dlcs = dlcs->_next) {
		if (dlcs->dlci == dlci) {
			return dlcs;
		}
	}

	return NULL;
}

static struct bt_rfcomm_dlc *rfcomm_dlcs_remove_dlci(struct bt_rfcomm_dlc *dlcs,
						     uint8_t dlci)
{
	struct bt_rfcomm_dlc *tmp;

	if (!dlcs) {
		return NULL;
	}

	/* If first node is the one to be removed */
	if (dlcs->dlci == dlci) {
		dlcs->session->dlcs = dlcs->_next;
		return dlcs;
	}

	for (tmp = dlcs, dlcs = dlcs->_next; dlcs; dlcs = dlcs->_next) {
		if (dlcs->dlci == dlci) {
			tmp->_next = dlcs->_next;
			return dlcs;
		}
		tmp = dlcs;
	}

	return NULL;
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

static struct bt_rfcomm_session *
rfcomm_sessions_lookup_bt_conn(struct bt_conn *conn)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(bt_rfcomm_pool); i++) {
		struct bt_rfcomm_session *session = &bt_rfcomm_pool[i];

		if (session->br_chan.chan.conn == conn) {
			return session;
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

static void rfcomm_dlc_tx_give_credits(struct bt_rfcomm_dlc *dlc,
				       uint8_t credits)
{
	BT_DBG("dlc %p credits %u", dlc, credits);

	while (credits--) {
		k_sem_give(&dlc->tx_credits);
	}

	BT_DBG("dlc %p updated credits %u", dlc,
	       k_sem_count_get(&dlc->tx_credits));
}

static void rfcomm_dlc_destroy(struct bt_rfcomm_dlc *dlc)
{
	BT_DBG("dlc %p", dlc);

	dlc->state = BT_RFCOMM_STATE_IDLE;
	dlc->session = NULL;
	if (dlc->ops && dlc->ops->disconnected) {
		dlc->ops->disconnected(dlc);
	}
}

static void rfcomm_dlc_disconnect(struct bt_rfcomm_dlc *dlc)
{
	uint8_t old_state = dlc->state;

	BT_DBG("dlc %p", dlc);

	if (dlc->state == BT_RFCOMM_STATE_DISCONNECTED) {
		return;
	}

	dlc->state = BT_RFCOMM_STATE_DISCONNECTED;

	switch (old_state) {
	case BT_RFCOMM_STATE_CONNECTED:
		/* Queue a dummy buffer to wake up and stop the
		 * tx thread for states where it was running.
		 */
		net_buf_put(&dlc->tx_queue, net_buf_get(&dummy, 0));

		/* There could be a writer waiting for credits so return a
		 * dummy credit to wake it up.
		 */
		rfcomm_dlc_tx_give_credits(dlc, 1);
		break;
	default:
		rfcomm_dlc_destroy(dlc);
		break;
	}
}

static void rfcomm_session_disconnected(struct bt_rfcomm_session *session)
{
	struct bt_rfcomm_dlc *dlc;

	BT_DBG("Session %p", session);

	if (session->state == BT_RFCOMM_STATE_DISCONNECTED) {
		return;
	}

	for (dlc = session->dlcs; dlc;) {
		struct bt_rfcomm_dlc *next;

		/* prefetch since disconnected callback may cleanup */
		next = dlc->_next;
		dlc->_next = NULL;

		rfcomm_dlc_disconnect(dlc);

		dlc = next;
	}

	session->state = BT_RFCOMM_STATE_DISCONNECTED;
	session->dlcs = NULL;
}

struct net_buf *bt_rfcomm_create_pdu(struct k_fifo *fifo)
{
	/* Length in RFCOMM header can be 2 bytes depending on length of user
	 * data
	 */
	return bt_conn_create_pdu(fifo,
				  sizeof(struct bt_l2cap_hdr) +
				  sizeof(struct bt_rfcomm_hdr) + 1);
}

static int rfcomm_send_sabm(struct bt_rfcomm_session *session, uint8_t dlci)
{
	struct bt_rfcomm_hdr *hdr;
	struct net_buf *buf;
	uint8_t cr, fcs;

	buf = bt_l2cap_create_pdu(&rfcomm_session, 0);
	if (!buf) {
		BT_ERR("No buffers");
		return -ENOMEM;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));
	cr = BT_RFCOMM_CMD_CR(session->role);
	hdr->address = BT_RFCOMM_SET_ADDR(dlci, cr);
	hdr->control = BT_RFCOMM_SET_CTRL(BT_RFCOMM_SABM, BT_RFCOMM_PF_NON_UIH);
	hdr->length = BT_RFCOMM_SET_LEN_8(0);

	fcs = rfcomm_calc_fcs(BT_RFCOMM_FCS_LEN_NON_UIH, buf->data);
	net_buf_add_u8(buf, fcs);

	return bt_l2cap_chan_send(&session->br_chan.chan, buf);
}

static struct net_buf *rfcomm_make_uih_msg(struct bt_rfcomm_dlc *dlc,
					   uint8_t cr, uint8_t type,
					   uint8_t len)
{
	struct bt_rfcomm_hdr *hdr;
	struct bt_rfcomm_msg_hdr *msg_hdr;
	struct net_buf *buf;
	uint8_t hdr_cr;

	buf = bt_l2cap_create_pdu(&rfcomm_session, 0);
	if (!buf) {
		BT_ERR("No buffers");
		return NULL;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr_cr = BT_RFCOMM_UIH_CR(dlc->session->role);
	hdr->address = BT_RFCOMM_SET_ADDR(0, hdr_cr);
	hdr->control = BT_RFCOMM_SET_CTRL(BT_RFCOMM_UIH, BT_RFCOMM_PF_UIH);
	hdr->length = BT_RFCOMM_SET_LEN_8(sizeof(*msg_hdr) + len);

	msg_hdr = net_buf_add(buf, sizeof(*msg_hdr));
	msg_hdr->type = BT_RFCOMM_SET_MSG_TYPE(type, cr);
	msg_hdr->len = BT_RFCOMM_SET_LEN_8(len);

	return buf;
}

static void rfcomm_connected(struct bt_l2cap_chan *chan)
{
	struct bt_rfcomm_session *session = RFCOMM_SESSION(chan);

	BT_DBG("Session %p", session);

	/* Need to include UIH header and FCS*/
	session->mtu = min(session->br_chan.rx.mtu,
			   session->br_chan.tx.mtu) -
			   BT_RFCOMM_HDR_SIZE + BT_RFCOMM_FCS_SIZE;

	if (session->state == BT_RFCOMM_STATE_CONNECTING) {
		rfcomm_send_sabm(session, 0);
	}
}

static void rfcomm_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_rfcomm_session *session = RFCOMM_SESSION(chan);

	BT_DBG("Session %p", session);

	rfcomm_session_disconnected(session);
	session->state = BT_RFCOMM_STATE_IDLE;
}

static void rfcomm_dlc_init(struct bt_rfcomm_dlc *dlc,
			    struct bt_rfcomm_session *session,
			    uint8_t dlci,
			    bt_rfcomm_role_t role)
{
	BT_DBG("dlc %p", dlc);

	dlc->dlci = dlci;
	dlc->session = session;
	dlc->rx_credit = RFCOMM_DEFAULT_CREDIT;
	dlc->state = BT_RFCOMM_STATE_INIT;
	dlc->role = role;
	k_sem_init(&dlc->tx_credits, 0, UINT32_MAX);

	dlc->_next = session->dlcs;
	session->dlcs = dlc;
}

static struct bt_rfcomm_dlc *rfcomm_dlc_accept(struct bt_rfcomm_session *session,
					       uint8_t dlci)
{
	struct bt_rfcomm_server *server;
	struct bt_rfcomm_dlc *dlc;
	uint8_t channel;

	channel = BT_RFCOMM_GET_CHANNEL(dlci);
	server = rfcomm_server_lookup_channel(channel);
	if (!server) {
		BT_ERR("Server Channel not registered");
		return NULL;
	}

	if (server->accept(session->br_chan.chan.conn, &dlc) < 0) {
		BT_DBG("Incoming connection rejected");
		return NULL;
	}

	if (!BT_RFCOMM_CHECK_MTU(dlc->mtu)) {
		rfcomm_dlc_destroy(dlc);
		return NULL;
	}

	rfcomm_dlc_init(dlc, session, dlci, BT_RFCOMM_ROLE_ACCEPTOR);
	dlc->mtu = min(dlc->mtu, session->mtu);

	return dlc;
}

static int rfcomm_send_dm(struct bt_rfcomm_session *session, uint8_t dlci)
{
	struct bt_rfcomm_hdr *hdr;
	struct net_buf *buf;
	uint8_t fcs, cr;

	BT_DBG("dlci %d", dlci);

	buf = bt_l2cap_create_pdu(&rfcomm_session, 0);
	if (!buf) {
		BT_ERR("No buffers");
		return -ENOMEM;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));
	cr = BT_RFCOMM_RESP_CR(session->role);
	hdr->address = BT_RFCOMM_SET_ADDR(dlci, cr);
	/* For DM PF bit is not relevant, we set it 1 */
	hdr->control = BT_RFCOMM_SET_CTRL(BT_RFCOMM_DM, BT_RFCOMM_PF_NON_UIH);
	hdr->length = BT_RFCOMM_SET_LEN_8(0);
	fcs = rfcomm_calc_fcs(BT_RFCOMM_FCS_LEN_NON_UIH, buf->data);
	net_buf_add_u8(buf, fcs);

	return bt_l2cap_chan_send(&session->br_chan.chan, buf);
}

static void rfcomm_dlc_tx_thread(void *p1, void *p2, void *p3)
{
	struct bt_rfcomm_dlc *dlc = p1;
	struct net_buf *buf;

	BT_DBG("Started for dlc %p", dlc);

	while (dlc->state == BT_RFCOMM_STATE_CONNECTED) {
		/* Get next packet for dlc */
		BT_DBG("Wait for buf %p", dlc);
		buf = net_buf_get_timeout(&dlc->tx_queue, 0, K_FOREVER);
		if (dlc->state != BT_RFCOMM_STATE_CONNECTED) {
			net_buf_unref(buf);
			break;
		}

		BT_DBG("Wait for credits %p", dlc);
		/* Wait for credits */
		k_sem_take(&dlc->tx_credits, K_FOREVER);
		if (dlc->state != BT_RFCOMM_STATE_CONNECTED) {
			net_buf_unref(buf);
			break;
		}

		if (bt_l2cap_chan_send(&dlc->session->br_chan.chan, buf) < 0) {
			/* This fails only if channel is disconnected */
			net_buf_unref(buf);
			break;
		}
	}

	BT_DBG("dlc %p disconnected - cleaning up", dlc);

	/* Give back any allocated buffers */
	while ((buf = net_buf_get_timeout(&dlc->tx_queue, 0, K_NO_WAIT))) {
		net_buf_unref(buf);
	}

	rfcomm_dlc_destroy(dlc);

	BT_DBG("dlc %p exiting", dlc);
}

static int rfcomm_send_ua(struct bt_rfcomm_session *session, uint8_t dlci)
{
	struct bt_rfcomm_hdr *hdr;
	struct net_buf *buf;
	uint8_t cr, fcs;

	buf = bt_l2cap_create_pdu(&rfcomm_session, 0);
	if (!buf) {
		BT_ERR("No buffers");
		return -ENOMEM;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));
	cr = BT_RFCOMM_RESP_CR(session->role);
	hdr->address = BT_RFCOMM_SET_ADDR(dlci, cr);
	hdr->control = BT_RFCOMM_SET_CTRL(BT_RFCOMM_UA, BT_RFCOMM_PF_NON_UIH);
	hdr->length = BT_RFCOMM_SET_LEN_8(0);

	fcs = rfcomm_calc_fcs(BT_RFCOMM_FCS_LEN_NON_UIH, buf->data);
	net_buf_add_u8(buf, fcs);

	return bt_l2cap_chan_send(&session->br_chan.chan, buf);
}

static int rfcomm_send_msc(struct bt_rfcomm_dlc *dlc, uint8_t cr)
{
	struct bt_rfcomm_msc *msc;
	struct net_buf *buf;
	uint8_t fcs;

	buf = rfcomm_make_uih_msg(dlc, cr, BT_RFCOMM_MSC, sizeof(*msc));
	if (!buf) {
		return -ENOMEM;
	}

	msc = net_buf_add(buf, sizeof(*msc));
	/* cr bit should be always 1 in MSC */
	msc->dlci = BT_RFCOMM_SET_ADDR(dlc->dlci, 1);
	msc->v24_signal = BT_RFCOMM_DEFAULT_V24_SIG;

	fcs = rfcomm_calc_fcs(BT_RFCOMM_FCS_LEN_UIH, buf->data);
	net_buf_add_u8(buf, fcs);

	return bt_l2cap_chan_send(&dlc->session->br_chan.chan, buf);
}

static void rfcomm_dlc_connected(struct bt_rfcomm_dlc *dlc)
{
	dlc->state = BT_RFCOMM_STATE_CONNECTED;

	rfcomm_send_msc(dlc, BT_RFCOMM_MSG_CMD_CR);

	k_fifo_init(&dlc->tx_queue);
	k_thread_spawn(dlc->stack, sizeof(dlc->stack), rfcomm_dlc_tx_thread,
		       dlc, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	if (dlc->ops && dlc->ops->connected) {
		dlc->ops->connected(dlc);
	}
}

enum security_result {
	RFCOMM_SECURITY_PASSED,
	RFCOMM_SECURITY_REJECT,
	RFCOMM_SECURITY_PENDING
};

static enum security_result rfcomm_dlc_security(struct bt_rfcomm_dlc *dlc)
{
	struct bt_conn *conn = dlc->session->br_chan.chan.conn;

	BT_DBG("dlc %p", dlc);

	/* If current security level is greater than or equal to required
	 * security level  then return SUCCESS.
	 * For SSP devices the current security will be atleast MEDIUM
	 * since L2CAP is enforcing it
	 */
	if (conn->sec_level >= dlc->required_sec_level) {
		return RFCOMM_SECURITY_PASSED;
	}

	if (!bt_conn_security(conn, dlc->required_sec_level)) {
		/* If Security elevation is initiated or in progress */
		return RFCOMM_SECURITY_PENDING;
	}

	/* Security request failed */
	return RFCOMM_SECURITY_REJECT;
}

static void rfcomm_dlc_drop(struct bt_rfcomm_dlc *dlc)
{
	BT_DBG("dlc %p", dlc);

	if (dlc->role == BT_RFCOMM_ROLE_ACCEPTOR) {
		rfcomm_send_dm(dlc->session, dlc->dlci);
	}
	rfcomm_dlcs_remove_dlci(dlc->session->dlcs, dlc->dlci);
	rfcomm_dlc_destroy(dlc);
}

static void rfcomm_handle_sabm(struct bt_rfcomm_session *session, uint8_t dlci)
{
	if (!dlci) {
		if (rfcomm_send_ua(session, dlci) < 0) {
			return;
		}

		session->state = BT_RFCOMM_STATE_CONNECTED;
	} else {
		struct bt_rfcomm_dlc *dlc;
		enum security_result result;

		dlc = rfcomm_dlcs_lookup_dlci(session->dlcs, dlci);
		if (!dlc) {
			dlc = rfcomm_dlc_accept(session, dlci);
			if (!dlc) {
				rfcomm_send_dm(session, dlci);
				return;
			}
		}

		result = rfcomm_dlc_security(dlc);
		switch (result) {
		case RFCOMM_SECURITY_PENDING:
			dlc->state = BT_RFCOMM_STATE_SECURITY_PENDING;
			/* TODO: Start an auth timer */
			return;
		case RFCOMM_SECURITY_PASSED:
			break;
		case RFCOMM_SECURITY_REJECT:
		default:
			rfcomm_dlc_drop(dlc);
			return;
		}

		if (rfcomm_send_ua(session, dlci) < 0) {
			return;
		}

		rfcomm_dlc_connected(dlc);
	}
}

static int rfcomm_send_pn(struct bt_rfcomm_dlc *dlc, uint8_t cr)
{
	struct bt_rfcomm_pn *pn;
	struct net_buf *buf;
	uint8_t fcs;

	buf = rfcomm_make_uih_msg(dlc, cr, BT_RFCOMM_PN, sizeof(*pn));
	if (!buf) {
		return -ENOMEM;
	}

	BT_DBG("mtu %x", dlc->mtu);

	pn = net_buf_add(buf, sizeof(*pn));
	pn->dlci = dlc->dlci;
	pn->mtu = sys_cpu_to_le16(dlc->mtu);
	if (dlc->state == BT_RFCOMM_STATE_CONFIG) {
		pn->credits = dlc->rx_credit;
		pn->flow_ctrl = cr ? 0xf0 : 0xe0;
	} else {
		/* If PN comes in already opened dlc these should be 0*/
		pn->credits = 0;
		pn->flow_ctrl = 0;
	}
	pn->max_retrans = 0;
	pn->ack_timer = 0;
	pn->priority = 0;

	fcs = rfcomm_calc_fcs(BT_RFCOMM_FCS_LEN_UIH, buf->data);
	net_buf_add_u8(buf, fcs);

	return bt_l2cap_chan_send(&dlc->session->br_chan.chan, buf);
}

static int rfcomm_send_credit(struct bt_rfcomm_dlc *dlc, uint8_t credits)
{
	struct bt_rfcomm_hdr *hdr;
	struct net_buf *buf;
	uint8_t fcs, cr;

	BT_DBG("Dlc %p credits %d", dlc, credits);

	buf = bt_l2cap_create_pdu(&rfcomm_session, 0);
	if (!buf) {
		BT_ERR("No buffers");
		return -ENOMEM;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));
	cr = BT_RFCOMM_UIH_CR(dlc->session->role);
	hdr->address = BT_RFCOMM_SET_ADDR(dlc->dlci, cr);
	hdr->control = BT_RFCOMM_SET_CTRL(BT_RFCOMM_UIH,
					  BT_RFCOMM_PF_UIH_CREDIT);
	hdr->length = BT_RFCOMM_SET_LEN_8(0);
	net_buf_add_u8(buf, credits);
	fcs = rfcomm_calc_fcs(BT_RFCOMM_FCS_LEN_UIH, buf->data);
	net_buf_add_u8(buf, fcs);

	return bt_l2cap_chan_send(&dlc->session->br_chan.chan, buf);
}

static void rfcomm_handle_ua(struct bt_rfcomm_session *session, uint8_t dlci)
{
	if (!dlci) {
		if (session->state != BT_RFCOMM_STATE_CONNECTING) {
			return;
		}
		session->state = BT_RFCOMM_STATE_CONNECTED;
		/* TODO: STart dlc */
	}
}

static void rfcomm_handle_msc(struct bt_rfcomm_session *session,
			      struct net_buf *buf, uint8_t cr)
{
	struct bt_rfcomm_msc *msc = (void *)buf->data;
	struct bt_rfcomm_dlc *dlc;
	uint8_t dlci = BT_RFCOMM_GET_DLCI(msc->dlci);

	BT_DBG("dlci %d", dlci);

	dlc = rfcomm_dlcs_lookup_dlci(session->dlcs, dlci);
	if (!dlc) {
		return;
	}

	if (cr == BT_RFCOMM_MSG_CMD_CR) {
		rfcomm_send_msc(dlc, BT_RFCOMM_MSG_RESP_CR);
	}
}

static void rfcomm_handle_pn(struct bt_rfcomm_session *session,
			     struct net_buf *buf, uint8_t cr)
{
	struct bt_rfcomm_pn *pn = (void *)buf->data;
	struct bt_rfcomm_dlc *dlc;

	if (!BT_RFCOMM_CHECK_MTU(pn->mtu)) {
		BT_ERR("Invalid mtu %d", pn->mtu);
		rfcomm_send_dm(session, pn->dlci);
		return;
	}

	dlc = rfcomm_dlcs_lookup_dlci(session->dlcs, pn->dlci);
	if (!dlc) {
		dlc = rfcomm_dlc_accept(session, pn->dlci);
		if (!dlc) {
			rfcomm_send_dm(session, pn->dlci);
			return;
		}

		BT_DBG("Incoming connection accepted dlc %p", dlc);

		dlc->mtu = min(dlc->mtu, sys_le16_to_cpu(pn->mtu));
		rfcomm_dlc_tx_give_credits(dlc, pn->credits);
		dlc->state = BT_RFCOMM_STATE_CONFIG;
	}

	rfcomm_send_pn(dlc, BT_RFCOMM_MSG_RESP_CR);
}

static void rfcomm_handle_disc(struct bt_rfcomm_session *session, uint8_t dlci)
{
	struct bt_rfcomm_dlc *dlc;

	BT_DBG("Dlci %d", dlci);

	if (dlci) {
		dlc = rfcomm_dlcs_remove_dlci(session->dlcs, dlci);
		if (!dlc) {
			rfcomm_send_dm(session, dlci);
			return;
		}

		rfcomm_send_ua(session, dlci);
		rfcomm_dlc_disconnect(dlc);
	} else {
		rfcomm_send_ua(session, 0);
		rfcomm_session_disconnected(session);
	}
}

static void rfcomm_handle_msg(struct bt_rfcomm_session *session,
			      struct net_buf *buf)
{
	struct bt_rfcomm_msg_hdr *hdr = (void *)buf->data;
	uint8_t msg_type, len, cr;

	msg_type = BT_RFCOMM_GET_MSG_TYPE(hdr->type);
	cr = BT_RFCOMM_GET_MSG_CR(hdr->type);
	len = BT_RFCOMM_GET_LEN(hdr->len);

	BT_DBG("msg type %x cr %x", msg_type, cr);

	net_buf_pull(buf, sizeof(*hdr));

	switch (msg_type) {
	case BT_RFCOMM_PN:
		rfcomm_handle_pn(session, buf, cr);
		break;
	case BT_RFCOMM_MSC:
		rfcomm_handle_msc(session, buf, cr);
		break;
	default:
		BT_WARN("Unknown/Unsupported RFCOMM Msg type 0x%02x", msg_type);
		break;
	}
}

static void rfcomm_dlc_update_credits(struct bt_rfcomm_dlc *dlc)
{
	uint8_t credits;

	BT_DBG("dlc %p credits %u", dlc, dlc->rx_credit);

	/* Only give more credits if it went below the defined threshold */
	if (dlc->rx_credit > RFCOMM_CREDITS_THRESHOLD) {
		return;
	}

	/* Restore credits */
	credits = RFCOMM_MAX_CREDITS - dlc->rx_credit;
	dlc->rx_credit += credits;

	rfcomm_send_credit(dlc, credits);
}

static void rfcomm_handle_data(struct bt_rfcomm_session *session,
			       struct net_buf *buf, uint8_t dlci, uint8_t pf)

{
	struct bt_rfcomm_dlc *dlc;

	BT_DBG("dlci %d, pf %d", dlci, pf);

	dlc = rfcomm_dlcs_lookup_dlci(session->dlcs, dlci);
	if (!dlc) {
		BT_ERR("Data recvd in non existing DLC");
		rfcomm_send_dm(session, dlci);
		return;
	}

	BT_DBG("dlc %p rx credit %d", dlc, dlc->rx_credit);

	if (dlc->state != BT_RFCOMM_STATE_CONNECTED) {
		return;
	}

	if (pf == BT_RFCOMM_PF_UIH_CREDIT) {
		rfcomm_dlc_tx_give_credits(dlc, net_buf_pull_u8(buf));
	}

	if (buf->len > BT_RFCOMM_FCS_SIZE) {
		if (!dlc->rx_credit) {
			BT_ERR("Data recvd when rx credit is 0");
			/* Disconnect */
			return;
		}

		/* Remove FCS */
		buf->len -= BT_RFCOMM_FCS_SIZE;
		if (dlc->ops && dlc->ops->recv) {
			dlc->ops->recv(dlc, buf);
		}

		dlc->rx_credit--;
		rfcomm_dlc_update_credits(dlc);
	}
}

int bt_rfcomm_dlc_send(struct bt_rfcomm_dlc *dlc, struct net_buf *buf)
{
	struct bt_rfcomm_hdr *hdr;
	uint8_t fcs, cr;

	if (!buf) {
		return -EINVAL;
	}

	BT_DBG("dlc %p tx credit %d", dlc, k_sem_count_get(&dlc->tx_credits));

	if (dlc->state != BT_RFCOMM_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	if (buf->len > dlc->mtu) {
		return -EMSGSIZE;
	}

	if (buf->len > BT_RFCOMM_MAX_LEN_8) {
		uint16_t *len;

		/* Length is 2 byte */
		hdr = net_buf_push(buf, sizeof(*hdr) + 1);
		len = (uint16_t *)&hdr->length;
		*len = BT_RFCOMM_SET_LEN_16(sys_cpu_to_le16(buf->len -
							    sizeof(*hdr) + 1));
	} else {
		hdr = net_buf_push(buf, sizeof(*hdr));
		hdr->length = BT_RFCOMM_SET_LEN_8(buf->len - sizeof(*hdr));
	}

	cr = BT_RFCOMM_UIH_CR(dlc->session->role);
	hdr->address = BT_RFCOMM_SET_ADDR(dlc->dlci, cr);
	hdr->control = BT_RFCOMM_SET_CTRL(BT_RFCOMM_UIH,
					  BT_RFCOMM_PF_UIH_NO_CREDIT);

	fcs = rfcomm_calc_fcs(BT_RFCOMM_FCS_LEN_UIH, buf->data);
	net_buf_add_u8(buf, fcs);

	net_buf_put(&dlc->tx_queue, buf);

	return buf->len;
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

	if (BT_RFCOMM_LEN_EXTENDED(hdr->length)) {
		net_buf_pull(buf, sizeof(*hdr) + 1);
	} else {
		net_buf_pull(buf, sizeof(*hdr));
	}

	switch (frame_type) {
	case BT_RFCOMM_SABM:
		rfcomm_handle_sabm(session, dlci);
		break;
	case BT_RFCOMM_UIH:
		if (!dlci) {
			rfcomm_handle_msg(session, buf);
		} else {
			rfcomm_handle_data(session, buf, dlci,
					   BT_RFCOMM_GET_PF(hdr->control));
		}
		break;
	case BT_RFCOMM_DISC:
		rfcomm_handle_disc(session, dlci);
		break;
	case BT_RFCOMM_UA:
		rfcomm_handle_ua(session, dlci);
		break;
	default:
		BT_WARN("Unknown/Unsupported RFCOMM Frame type 0x%02x",
			frame_type);
		break;
	}
}

static void rfcomm_encrypt_change(struct bt_l2cap_chan *chan,
				  uint8_t hci_status)
{
	struct bt_rfcomm_session *session = RFCOMM_SESSION(chan);
	struct bt_conn *conn = chan->conn;
	struct bt_rfcomm_dlc *dlc;

	BT_DBG("session %p status 0x%02x encr 0x%02x", session, hci_status,
	       conn->encrypt);

	for (dlc = session->dlcs; dlc; dlc = dlc->_next) {
		if (dlc->state != BT_RFCOMM_STATE_SECURITY_PENDING) {
			continue;
		}

		if (hci_status || !conn->encrypt ||
		    conn->sec_level < dlc->required_sec_level) {
			rfcomm_dlc_drop(dlc);
			continue;
		}

		if (dlc->role == BT_RFCOMM_ROLE_ACCEPTOR) {
			rfcomm_send_ua(session, dlc->dlci);
			rfcomm_dlc_connected(dlc);
		}
	}
}

static struct bt_rfcomm_session *rfcomm_session_new(bt_rfcomm_role_t role)
{
	int i;
	static struct bt_l2cap_chan_ops ops = {
		.connected = rfcomm_connected,
		.disconnected = rfcomm_disconnected,
		.recv = rfcomm_recv,
		.encrypt_change = rfcomm_encrypt_change,
	};

	for (i = 0; i < ARRAY_SIZE(bt_rfcomm_pool); i++) {
		struct bt_rfcomm_session *session = &bt_rfcomm_pool[i];

		if (session->br_chan.chan.conn) {
			continue;
		}

		BT_DBG("session %p initialized", session);

		session->br_chan.chan.ops = &ops;
		session->br_chan.rx.mtu	= CONFIG_BLUETOOTH_RFCOMM_L2CAP_MTU;
		session->state = BT_RFCOMM_STATE_INIT;
		session->role = role;

		return session;
	}

	return NULL;
}

int bt_rfcomm_dlc_connect(struct bt_conn *conn, struct bt_rfcomm_dlc *dlc,
			  uint8_t channel)
{
	struct bt_rfcomm_session *session;
	struct bt_l2cap_chan *chan;
	int ret;

	BT_DBG("conn %p dlc %p channel %d", conn, dlc, channel);

	if (!dlc) {
		return -EINVAL;
	}

	if (!conn || conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	if (channel < RFCOMM_CHANNEL_START || channel > RFCOMM_CHANNEL_END) {
		return -EINVAL;
	}

	session = rfcomm_sessions_lookup_bt_conn(conn);
	if (!session) {
		session = rfcomm_session_new(BT_RFCOMM_ROLE_INITIATOR);
		if (!session) {
			return -ENOMEM;
		}
	}

	switch (session->state) {
	case BT_RFCOMM_STATE_INIT:
		if (session->role == BT_RFCOMM_ROLE_ACCEPTOR) {
			/* There is an ongoing incoming conn */
			break;
		}
		chan = &session->br_chan.chan;
		ret = bt_l2cap_chan_connect(conn, chan, BT_L2CAP_PSM_RFCOMM);
		if (ret < 0) {
			session->state = BT_RFCOMM_STATE_IDLE;
			return ret;
		}
		session->state = BT_RFCOMM_STATE_CONNECTING;
		break;
	case BT_RFCOMM_STATE_CONNECTING:
		break;
	case BT_RFCOMM_STATE_CONNECTED:
		/* TODO: Start dlc */
		break;
	default:
		BT_ERR("Invalid session state %d", session->state);
		return -EINVAL;
	}

	return 0;
}

static int rfcomm_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	struct bt_rfcomm_session *session;

	BT_DBG("conn %p", conn);

	session = rfcomm_session_new(BT_RFCOMM_ROLE_ACCEPTOR);
	if (session) {
		*chan = &session->br_chan.chan;
		return 0;
	}

	BT_ERR("No available RFCOMM context for conn %p", conn);

	return -ENOMEM;
}

void bt_rfcomm_init(void)
{
	static struct bt_l2cap_server server = {
		.psm       = BT_L2CAP_PSM_RFCOMM,
		.accept    = rfcomm_accept,
		.sec_level = BT_SECURITY_LOW,
	};

	net_buf_pool_init(rfcomm_session_pool);
	net_buf_pool_init(dummy_pool);
	bt_l2cap_br_server_register(&server);
}
