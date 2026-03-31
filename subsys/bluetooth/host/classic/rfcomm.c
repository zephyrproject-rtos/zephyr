/* rfcomm.c - RFCOMM handling */

/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright 2024-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/crc.h>
#include <zephyr/debug/stack.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>

#include <zephyr/bluetooth/classic/rfcomm.h>

#include "host/hci_core.h"
#include "host/conn_internal.h"
#include "l2cap_br_internal.h"
#include "rfcomm_internal.h"

#define LOG_LEVEL CONFIG_BT_RFCOMM_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_rfcomm);

#define RFCOMM_CHANNEL_START	0x01
#define RFCOMM_CHANNEL_END	0x1e

#define RFCOMM_MIN_MTU		BT_RFCOMM_SIG_MIN_MTU
#define RFCOMM_DEFAULT_MTU	127

#define RFCOMM_MAX_CREDITS		(BT_BUF_ACL_RX_COUNT - 1)
#define RFCOMM_CREDITS_THRESHOLD	(RFCOMM_MAX_CREDITS / 2)
#define RFCOMM_DEFAULT_CREDIT		RFCOMM_MAX_CREDITS

#define RFCOMM_CONN_TIMEOUT     K_SECONDS(60)
#define RFCOMM_DISC_TIMEOUT     K_SECONDS(20)
#define RFCOMM_IDLE_TIMEOUT     K_SECONDS(2)

#define DLC_RTX(_w) CONTAINER_OF(k_work_delayable_from_work(_w), \
				 struct bt_rfcomm_dlc, rtx_work)
#define SESSION_RTX(_w) CONTAINER_OF(k_work_delayable_from_work(_w), \
				     struct bt_rfcomm_session, rtx_work)

static sys_slist_t servers = SYS_SLIST_STATIC_INIT(&servers);

#define RFCOMM_SESSION(_ch) CONTAINER_OF(_ch, \
					 struct bt_rfcomm_session, br_chan.chan)

static struct bt_rfcomm_session bt_rfcomm_pool[CONFIG_BT_MAX_CONN];

#define RFCOMM_CRC_POLY CRC8_REFLECT_POLY
#define RFCOMM_CRC_INIT 0xff
#define RFCOMM_CRC_REVD true

static uint8_t rfcomm_calc_fcs(uint16_t len, const uint8_t *data)
{
	uint8_t fcs;

	fcs = crc8(data, len, RFCOMM_CRC_POLY, RFCOMM_CRC_INIT, RFCOMM_CRC_REVD);

	/* Ones compliment */
	fcs = (0xff - fcs);

	return fcs;
}

static bool rfcomm_check_fcs(uint16_t len, const uint8_t *data, uint8_t recvd_fcs)
{
	uint8_t fcs;

	fcs = rfcomm_calc_fcs(len, data);

	return (fcs == recvd_fcs);
}

static struct bt_rfcomm_dlc *rfcomm_dlcs_lookup_dlci(struct bt_rfcomm_session *session,
						     uint8_t dlci)
{
	struct bt_rfcomm_dlc *dlc, *tmp;

	if (session == NULL) {
		return NULL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&session->dlcs, dlc, tmp, _node) {
		if (dlc->dlci == dlci) {
			return dlc;
		}
	}

	return NULL;
}

static struct bt_rfcomm_dlc *rfcomm_dlcs_remove_dlci(struct bt_rfcomm_session *session,
						     uint8_t dlci)
{
	struct bt_rfcomm_dlc *tmp;
	bool found;

	tmp = rfcomm_dlcs_lookup_dlci(session, dlci);
	if (tmp == NULL) {
		return NULL;
	}

	found = sys_slist_find_and_remove(&session->dlcs, &tmp->_node);
	if (!found) {
		LOG_WRN("DLC %p has been removed", tmp);
	}
	return tmp;
}

static struct bt_rfcomm_server *rfcomm_server_lookup_channel(uint8_t channel)
{
	struct bt_rfcomm_server *server;
	struct bt_rfcomm_server *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&servers, server, next, node) {
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
	if (server->channel > RFCOMM_CHANNEL_END || !server->accept) {
		return -EINVAL;
	}

	if (!server->channel) {
		uint8_t chan = (uint8_t)BT_RFCOMM_CHAN_DYNAMIC_START;

		for (; chan <= RFCOMM_CHANNEL_END; chan++) {
			/* Check if given channel is already in use */
			if (!rfcomm_server_lookup_channel(chan)) {
				server->channel = chan;
				LOG_DBG("Allocated channel 0x%02x for new server", chan);
				break;
			}
		}

		if (!server->channel) {
			LOG_WRN("No free dynamic rfcomm channels available");
			return -EADDRNOTAVAIL;
		}
	} else {
		/* Check if given channel is already in use */
		if (rfcomm_server_lookup_channel(server->channel)) {
			LOG_WRN("Channel already registered");
			return -EADDRINUSE;
		}
	}

	LOG_DBG("Channel 0x%02x", server->channel);

	sys_slist_prepend(&servers, &server->node);

	return 0;
}

int bt_rfcomm_server_unregister(struct bt_rfcomm_server *server)
{
	if (!sys_slist_find_and_remove(&servers, &server->node)) {
		return -ENOENT;
	}

	return 0;
}

static void rfcomm_dlc_tx_trigger(struct bt_rfcomm_dlc *dlc)
{
	int err;

	if ((dlc == NULL) || (dlc->tx_work.handler == NULL)) {
		return;
	}

	LOG_DBG("DLC %p TX trigger", dlc);

	err = k_work_submit(&dlc->tx_work);
	if (err < 0) {
		LOG_ERR("Failed to submit tx work: %d", err);
	}
}

static void rfcomm_dlcs_tx_trigger(struct bt_rfcomm_session *session)
{
	struct bt_rfcomm_dlc *dlc, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&session->dlcs, dlc, tmp, _node) {
		rfcomm_dlc_tx_trigger(dlc);
	}
}

static void rfcomm_dlc_tx_give_credits(struct bt_rfcomm_dlc *dlc,
				       uint8_t credits)
{
	LOG_DBG("dlc %p credits %u", dlc, credits);

	while (credits--) {
		k_sem_give(&dlc->tx_credits);
		rfcomm_dlc_tx_trigger(dlc);
	}

	LOG_DBG("dlc %p updated credits %u", dlc, k_sem_count_get(&dlc->tx_credits));
}

static void rfcomm_dlc_destroy(struct bt_rfcomm_dlc *dlc)
{
	LOG_DBG("dlc %p", dlc);

	k_work_cancel_delayable(&dlc->rtx_work);
	k_work_cancel(&dlc->tx_work);

	dlc->state = BT_RFCOMM_STATE_IDLE;
	dlc->session = NULL;

	if (dlc->ops && dlc->ops->disconnected) {
		dlc->ops->disconnected(dlc);
	}
}

static void rfcomm_dlc_disconnect(struct bt_rfcomm_dlc *dlc)
{
	uint8_t old_state = dlc->state;

	LOG_DBG("dlc %p", dlc);

	if (dlc->state == BT_RFCOMM_STATE_DISCONNECTED) {
		return;
	}

	dlc->state = BT_RFCOMM_STATE_DISCONNECTED;

	switch (old_state) {
	case BT_RFCOMM_STATE_CONNECTED:
		rfcomm_dlc_tx_trigger(dlc);
		break;
	default:
		rfcomm_dlc_destroy(dlc);
		break;
	}
}

static void rfcomm_session_disconnected(struct bt_rfcomm_session *session)
{
	struct bt_rfcomm_dlc *dlc, *tmp;

	LOG_DBG("Session %p", session);

	if (session->state == BT_RFCOMM_STATE_DISCONNECTED) {
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&session->dlcs, dlc, tmp, _node) {
		rfcomm_dlc_disconnect(dlc);
	}

	sys_slist_init(&session->dlcs);
	session->state = BT_RFCOMM_STATE_DISCONNECTED;
}

struct net_buf *bt_rfcomm_create_pdu(struct net_buf_pool *pool)
{
	/* Length in RFCOMM header can be 2 bytes depending on length of user
	 * data
	 */
	return bt_conn_create_pdu(pool,
				  sizeof(struct bt_l2cap_hdr) +
				  sizeof(struct bt_rfcomm_hdr) + 1);
}

static int rfcomm_send_cb(struct bt_rfcomm_session *session, struct net_buf *buf,
				 bt_conn_tx_cb_t cb, void *user_data)
{
	int err;

	err = bt_l2cap_br_chan_send_cb(&session->br_chan.chan, buf, cb, user_data);
	if (err < 0) {
		net_buf_unref(buf);
	}

	return err;
}

static int rfcomm_send(struct bt_rfcomm_session *session, struct net_buf *buf)
{
	return rfcomm_send_cb(session, buf, NULL, NULL);
}

static int rfcomm_send_sabm(struct bt_rfcomm_session *session, uint8_t dlci)
{
	struct bt_rfcomm_hdr *hdr;
	struct net_buf *buf;
	uint8_t cr, fcs;

	buf = bt_l2cap_create_pdu(NULL, 0);

	hdr = net_buf_add(buf, sizeof(*hdr));
	cr = BT_RFCOMM_CMD_CR(session->role);
	hdr->address = BT_RFCOMM_SET_ADDR(dlci, cr);
	hdr->control = BT_RFCOMM_SET_CTRL(BT_RFCOMM_SABM, BT_RFCOMM_PF_NON_UIH);
	hdr->length = BT_RFCOMM_SET_LEN_8(0);

	fcs = rfcomm_calc_fcs(BT_RFCOMM_FCS_LEN_NON_UIH, buf->data);
	net_buf_add_u8(buf, fcs);

	return rfcomm_send(session, buf);
}

static int rfcomm_send_disc(struct bt_rfcomm_session *session, uint8_t dlci)
{
	struct bt_rfcomm_hdr *hdr;
	struct net_buf *buf;
	uint8_t fcs, cr;

	LOG_DBG("dlci %d", dlci);

	buf = bt_l2cap_create_pdu(NULL, 0);

	hdr = net_buf_add(buf, sizeof(*hdr));
	cr = BT_RFCOMM_CMD_CR(session->role);
	hdr->address = BT_RFCOMM_SET_ADDR(dlci, cr);
	hdr->control = BT_RFCOMM_SET_CTRL(BT_RFCOMM_DISC, BT_RFCOMM_PF_NON_UIH);
	hdr->length = BT_RFCOMM_SET_LEN_8(0);
	fcs = rfcomm_calc_fcs(BT_RFCOMM_FCS_LEN_NON_UIH, buf->data);
	net_buf_add_u8(buf, fcs);

	return rfcomm_send(session, buf);
}

static void rfcomm_session_disconnect(struct bt_rfcomm_session *session)
{
	if (!sys_slist_is_empty(&session->dlcs)) {
		return;
	}

	session->state = BT_RFCOMM_STATE_DISCONNECTING;
	rfcomm_send_disc(session, 0);
	k_work_reschedule(&session->rtx_work, RFCOMM_DISC_TIMEOUT);
}

static struct net_buf *rfcomm_make_uih_msg(struct bt_rfcomm_session *session,
					   uint8_t cr, uint8_t type,
					   uint8_t len)
{
	struct bt_rfcomm_hdr *hdr;
	struct bt_rfcomm_msg_hdr *msg_hdr;
	struct net_buf *buf;
	uint8_t hdr_cr;

	buf = bt_l2cap_create_pdu(NULL, 0);

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr_cr = BT_RFCOMM_UIH_CR(session->role);
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

	LOG_DBG("Session %p", session);

	/* Need to include UIH header and FCS*/
	session->mtu = MIN(session->br_chan.rx.mtu,
			   session->br_chan.tx.mtu) -
			   BT_RFCOMM_HDR_SIZE - BT_RFCOMM_FCS_SIZE;

	if (session->state == BT_RFCOMM_STATE_CONNECTING) {
		rfcomm_send_sabm(session, 0);
	}
}

static void rfcomm_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_rfcomm_session *session = RFCOMM_SESSION(chan);

	LOG_DBG("Session %p", session);

	k_work_cancel_delayable(&session->rtx_work);
	rfcomm_session_disconnected(session);
	session->state = BT_RFCOMM_STATE_IDLE;
}

static void rfcomm_dlc_rtx_timeout(struct k_work *work)
{
	struct bt_rfcomm_dlc *dlc = DLC_RTX(work);
	struct bt_rfcomm_session *session = dlc->session;

	LOG_WRN("dlc %p state %d timeout", dlc, dlc->state);

	rfcomm_dlcs_remove_dlci(session, dlc->dlci);
	rfcomm_dlc_disconnect(dlc);
	rfcomm_session_disconnect(session);
}

static void rfcomm_dlc_init(struct bt_rfcomm_dlc *dlc,
			    struct bt_rfcomm_session *session,
			    uint8_t dlci,
			    bt_rfcomm_role_t role)
{
	struct k_work_sync sync;

	LOG_DBG("dlc %p", dlc);

	if (dlc->tx_work.handler != NULL) {
		/* Make sure the tx_work is cancelled before reinitializing */
		k_work_cancel_sync(&dlc->tx_work, &sync);
		/* Reset the work handler to NULL before the DLC connected */
		dlc->tx_work.handler = NULL;
	}

	dlc->dlci = dlci;
	dlc->session = session;
	dlc->rx_credit = RFCOMM_DEFAULT_CREDIT;
	dlc->state = BT_RFCOMM_STATE_INIT;
	dlc->role = role;
	k_work_init_delayable(&dlc->rtx_work, rfcomm_dlc_rtx_timeout);

	/* Start a conn timer which includes auth as well */
	k_work_schedule(&dlc->rtx_work, RFCOMM_CONN_TIMEOUT);

	sys_slist_prepend(&session->dlcs, &dlc->_node);
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
		LOG_ERR("Server Channel not registered");
		return NULL;
	}

	if (server->accept(session->br_chan.chan.conn, server, &dlc) < 0) {
		LOG_DBG("Incoming connection rejected");
		return NULL;
	}

	if (!BT_RFCOMM_CHECK_MTU(dlc->mtu)) {
		rfcomm_dlc_destroy(dlc);
		return NULL;
	}

	rfcomm_dlc_init(dlc, session, dlci, BT_RFCOMM_ROLE_ACCEPTOR);
	dlc->mtu = MIN(dlc->mtu, session->mtu);

	return dlc;
}

static int rfcomm_send_dm(struct bt_rfcomm_session *session, uint8_t dlci)
{
	struct bt_rfcomm_hdr *hdr;
	struct net_buf *buf;
	uint8_t fcs, cr;

	LOG_DBG("dlci %d", dlci);

	buf = bt_l2cap_create_pdu(NULL, 0);

	hdr = net_buf_add(buf, sizeof(*hdr));
	cr = BT_RFCOMM_RESP_CR(session->role);
	hdr->address = BT_RFCOMM_SET_ADDR(dlci, cr);
	/* For DM PF bit is not relevant, we set it 1 */
	hdr->control = BT_RFCOMM_SET_CTRL(BT_RFCOMM_DM, BT_RFCOMM_PF_NON_UIH);
	hdr->length = BT_RFCOMM_SET_LEN_8(0);
	fcs = rfcomm_calc_fcs(BT_RFCOMM_FCS_LEN_NON_UIH, buf->data);
	net_buf_add_u8(buf, fcs);

	return rfcomm_send(session, buf);
}

static bool rfcomm_check_fc(struct bt_rfcomm_dlc *dlc)
{
	int err;

	LOG_DBG("Wait for credits or MSC FC %p", dlc);
	/* Wait for credits or MSC FC */
	err = k_sem_take(&dlc->tx_credits, K_NO_WAIT);
	if (err != 0) {
		LOG_DBG("Credits not available");
		return false;
	}

	LOG_DBG("Credits available");
	if (dlc->session->cfc == BT_RFCOMM_CFC_SUPPORTED) {
		LOG_DBG("Credits available");
		return true;
	}

	err = k_sem_take(&dlc->session->fc, K_NO_WAIT);
	if (err != 0) {
		k_sem_give(&dlc->tx_credits);
		LOG_DBG("Flow control not available");
		return false;
	}

	/* Give the sems immediately so that sem will be available for all
	 * the bufs in the queue. It will be blocked only once all the bufs
	 * are sent (which will preempt this thread) and FCOFF / FC bit
	 * with 1, is received.
	 */
	k_sem_give(&dlc->session->fc);
	k_sem_give(&dlc->tx_credits);

	LOG_DBG("Flow control available");
	return true;
}

static void bt_rfcomm_tx_destroy(struct bt_rfcomm_dlc *dlc, struct net_buf *buf)
{
	LOG_DBG("dlc %p, buf %p", dlc, buf);

	if ((buf == NULL) || (buf->len == 0)) {
		return;
	}

	if (dlc && dlc->ops && dlc->ops->sent) {
		dlc->ops->sent(dlc, -ESHUTDOWN);
	}
}

static void rfcomm_sent(struct bt_conn *conn, void *user_data, int err)
{
	struct bt_rfcomm_dlc *dlc;

	LOG_DBG("conn %p", conn);

	if (user_data == NULL)	{
		return;
	}

	dlc = (struct bt_rfcomm_dlc *)user_data;

	rfcomm_dlc_tx_trigger(dlc);

	if (dlc && dlc->ops && dlc->ops->sent) {
		dlc->ops->sent(dlc, err);
	}
}

static void rfcomm_dlc_tx_worker(struct k_work *work)
{
	struct bt_rfcomm_dlc *dlc = CONTAINER_OF(work, struct bt_rfcomm_dlc, tx_work);
	struct net_buf *buf;

	LOG_DBG("Work for dlc %p state %u", dlc, dlc->state);

	if (dlc->state < BT_RFCOMM_STATE_CONNECTED) {
		return;
	}

	if (dlc->state == BT_RFCOMM_STATE_CONNECTED ||
	    dlc->state == BT_RFCOMM_STATE_USER_DISCONNECT) {
		if (k_fifo_is_empty(&dlc->tx_queue) == true) {
			goto user_disconnect;
		}

		if (rfcomm_check_fc(dlc) == false) {
			LOG_DBG("FC or credit not available");
			goto user_disconnect;
		}

		buf = k_fifo_get(&dlc->tx_queue, K_NO_WAIT);
		LOG_DBG("Tx buf %p", buf);
		if (rfcomm_send_cb(dlc->session, buf, rfcomm_sent, dlc) < 0) {
			/* This fails only if channel is disconnected */
			dlc->state = BT_RFCOMM_STATE_DISCONNECTED;
			bt_rfcomm_tx_destroy(dlc, buf);
			LOG_ERR("Failed to send buffer, disconnected");
			goto disconnect;
		}

		if (k_fifo_is_empty(&dlc->tx_queue) == false) {
			rfcomm_dlc_tx_trigger(dlc);
			return;
		}

user_disconnect:
		if (dlc->state == BT_RFCOMM_STATE_USER_DISCONNECT) {
			LOG_DBG("Process user disconnect");
			goto disconnect;
		}

		return;
	}

disconnect:
	LOG_DBG("dlc %p disconnected - cleaning up", dlc);

	/* Give back any allocated buffers */
	while ((buf = k_fifo_get(&dlc->tx_queue, K_NO_WAIT))) {
		bt_rfcomm_tx_destroy(dlc, buf);
		net_buf_unref(buf);
	}

	if (dlc->state == BT_RFCOMM_STATE_USER_DISCONNECT) {
		dlc->state = BT_RFCOMM_STATE_DISCONNECTING;
	}

	if (dlc->state == BT_RFCOMM_STATE_DISCONNECTING) {
		rfcomm_send_disc(dlc->session, dlc->dlci);
		k_work_reschedule(&dlc->rtx_work, RFCOMM_DISC_TIMEOUT);
	} else {
		rfcomm_dlc_destroy(dlc);
	}

	LOG_DBG("dlc %p exiting", dlc);
}

static int rfcomm_send_ua(struct bt_rfcomm_session *session, uint8_t dlci)
{
	struct bt_rfcomm_hdr *hdr;
	struct net_buf *buf;
	uint8_t cr, fcs;

	buf = bt_l2cap_create_pdu(NULL, 0);

	hdr = net_buf_add(buf, sizeof(*hdr));
	cr = BT_RFCOMM_RESP_CR(session->role);
	hdr->address = BT_RFCOMM_SET_ADDR(dlci, cr);
	hdr->control = BT_RFCOMM_SET_CTRL(BT_RFCOMM_UA, BT_RFCOMM_PF_NON_UIH);
	hdr->length = BT_RFCOMM_SET_LEN_8(0);

	fcs = rfcomm_calc_fcs(BT_RFCOMM_FCS_LEN_NON_UIH, buf->data);
	net_buf_add_u8(buf, fcs);

	return rfcomm_send(session, buf);
}

static int rfcomm_send_msc(struct bt_rfcomm_dlc *dlc, uint8_t cr,
			   uint8_t v24_signal)
{
	struct bt_rfcomm_msc *msc;
	struct net_buf *buf;
	uint8_t fcs;

	buf = rfcomm_make_uih_msg(dlc->session, cr, BT_RFCOMM_MSC,
				  sizeof(*msc));

	msc = net_buf_add(buf, sizeof(*msc));
	/* cr bit should be always 1 in MSC */
	msc->dlci = BT_RFCOMM_SET_ADDR(dlc->dlci, 1);
	msc->v24_signal = v24_signal;

	fcs = rfcomm_calc_fcs(BT_RFCOMM_FCS_LEN_UIH, buf->data);
	net_buf_add_u8(buf, fcs);

	return rfcomm_send(dlc->session, buf);
}

static int rfcomm_send_rls(struct bt_rfcomm_dlc *dlc, uint8_t cr,
			   uint8_t line_status)
{
	struct bt_rfcomm_rls *rls;
	struct net_buf *buf;
	uint8_t fcs;

	buf = rfcomm_make_uih_msg(dlc->session, cr, BT_RFCOMM_RLS,
				  sizeof(*rls));

	rls = net_buf_add(buf, sizeof(*rls));
	/* cr bit should be always 1 in RLS */
	rls->dlci = BT_RFCOMM_SET_ADDR(dlc->dlci, 1);
	rls->line_status = line_status;

	fcs = rfcomm_calc_fcs(BT_RFCOMM_FCS_LEN_UIH, buf->data);
	net_buf_add_u8(buf, fcs);

	return rfcomm_send(dlc->session, buf);
}

static int rfcomm_send_rpn(struct bt_rfcomm_session *session, uint8_t cr,
			   struct bt_rfcomm_rpn *rpn)
{
	struct net_buf *buf;
	uint8_t fcs;

	buf = rfcomm_make_uih_msg(session, cr, BT_RFCOMM_RPN, sizeof(*rpn));

	net_buf_add_mem(buf, rpn, sizeof(*rpn));

	fcs = rfcomm_calc_fcs(BT_RFCOMM_FCS_LEN_UIH, buf->data);
	net_buf_add_u8(buf, fcs);

	return rfcomm_send(session, buf);
}

int bt_rfcomm_send_rpn_cmd(struct bt_rfcomm_dlc *dlc, struct bt_rfcomm_rpn *rpn)
{
	struct bt_rfcomm_session *session;

	if (!dlc || !rpn) {
		return -EINVAL;
	}

	/* Validate baud rate */
	if (rpn->baud_rate > BT_RFCOMM_RPN_BAUD_RATE_230400) {
		LOG_ERR("Invalid baud rate: %u", rpn->baud_rate);
		return -EINVAL;
	}

	session = dlc->session;
	if (!session) {
		return -ENOTCONN;
	}

	if (dlc->state != BT_RFCOMM_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	LOG_DBG("dlc %p", dlc);

	/* Set DLCI in the RPN command */
	rpn->dlci = BT_RFCOMM_SET_ADDR(dlc->dlci, 1);

	/* Send the RPN command */
	return rfcomm_send_rpn(session, BT_RFCOMM_MSG_CMD_CR, rpn);
}

static int rfcomm_send_test(struct bt_rfcomm_session *session, uint8_t cr,
			    uint8_t *pattern, uint8_t len)
{
	struct net_buf *buf;
	uint8_t fcs;

	buf = rfcomm_make_uih_msg(session, cr, BT_RFCOMM_TEST, len);

	net_buf_add_mem(buf, pattern, len);

	fcs = rfcomm_calc_fcs(BT_RFCOMM_FCS_LEN_UIH, buf->data);
	net_buf_add_u8(buf, fcs);

	return rfcomm_send(session, buf);
}

static int rfcomm_send_nsc(struct bt_rfcomm_session *session, uint8_t cmd_type)
{
	struct net_buf *buf;
	uint8_t fcs;

	buf = rfcomm_make_uih_msg(session, BT_RFCOMM_MSG_RESP_CR,
				  BT_RFCOMM_NSC, sizeof(cmd_type));

	net_buf_add_u8(buf, cmd_type);

	fcs = rfcomm_calc_fcs(BT_RFCOMM_FCS_LEN_UIH, buf->data);
	net_buf_add_u8(buf, fcs);

	return rfcomm_send(session, buf);
}

static int rfcomm_send_fcon(struct bt_rfcomm_session *session, uint8_t cr)
{
	struct net_buf *buf;
	uint8_t fcs;

	buf = rfcomm_make_uih_msg(session, cr, BT_RFCOMM_FCON, 0);

	fcs = rfcomm_calc_fcs(BT_RFCOMM_FCS_LEN_UIH, buf->data);
	net_buf_add_u8(buf, fcs);

	return rfcomm_send(session, buf);
}

static int rfcomm_send_fcoff(struct bt_rfcomm_session *session, uint8_t cr)
{
	struct net_buf *buf;
	uint8_t fcs;

	buf = rfcomm_make_uih_msg(session, cr, BT_RFCOMM_FCOFF, 0);

	fcs = rfcomm_calc_fcs(BT_RFCOMM_FCS_LEN_UIH, buf->data);
	net_buf_add_u8(buf, fcs);

	return rfcomm_send(session, buf);
}

/* The size of credits field. */
#define BT_RFCOMM_CREDITS_SIZE 0x01

static void rfcomm_dlc_connected(struct bt_rfcomm_dlc *dlc)
{
	dlc->state = BT_RFCOMM_STATE_CONNECTED;

	rfcomm_send_msc(dlc, BT_RFCOMM_MSG_CMD_CR, BT_RFCOMM_DEFAULT_V24_SIG);

	if (dlc->session->cfc == BT_RFCOMM_CFC_UNKNOWN) {
		/* This means PN negotiation is not done for this session and
		 * can happen only for 1.0b device.
		 */
		dlc->session->cfc = BT_RFCOMM_CFC_NOT_SUPPORTED;
	}

	if (dlc->session->cfc == BT_RFCOMM_CFC_NOT_SUPPORTED) {
		LOG_DBG("CFC not supported %p", dlc);
		rfcomm_send_fcon(dlc->session, BT_RFCOMM_MSG_CMD_CR);
		/* Use tx_credits as binary sem for MSC FC */
		k_sem_init(&dlc->tx_credits, 0, 1);
	}

	/* Cancel conn timer */
	k_work_cancel_delayable(&dlc->rtx_work);

	k_fifo_init(&dlc->tx_queue);
	k_work_init(&dlc->tx_work, rfcomm_dlc_tx_worker);

	/* For CFC supported cases, the space of credits should be discounted from the maximum
	 * frame size. It is used to avoid the SDU length exceeding the maximum frame size if the
	 * credits field is included.
	 */
	if (dlc->session->cfc == BT_RFCOMM_CFC_SUPPORTED) {
		dlc->mtu -= BT_RFCOMM_CREDITS_SIZE;
	}

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

	LOG_DBG("dlc %p", dlc);

	/* If current security level is greater than or equal to required
	 * security level  then return SUCCESS.
	 * For SSP devices the current security will be at least MEDIUM
	 * since L2CAP is enforcing it
	 */
	if (conn->sec_level >= dlc->required_sec_level) {
		return RFCOMM_SECURITY_PASSED;
	}

	if (!bt_conn_set_security(conn, dlc->required_sec_level)) {
		/*
		 * General Bonding refers to the process of performing bonding
		 * during connection setup or channel establishment procedures
		 * as a precursor to accessing a service.
		 * For current case, it is dedicated bonding.
		 */
		atomic_set_bit(conn->flags, BT_CONN_BR_GENERAL_BONDING);
		/* If Security elevation is initiated or in progress */
		return RFCOMM_SECURITY_PENDING;
	}

	/* Security request failed */
	return RFCOMM_SECURITY_REJECT;
}

static void rfcomm_dlc_drop(struct bt_rfcomm_dlc *dlc)
{
	LOG_DBG("dlc %p", dlc);

	(void)rfcomm_dlcs_remove_dlci(dlc->session, dlc->dlci);
	rfcomm_dlc_destroy(dlc);
}

static int rfcomm_dlc_close(struct bt_rfcomm_dlc *dlc)
{
	LOG_DBG("dlc %p", dlc);

	switch (dlc->state) {
	case BT_RFCOMM_STATE_SECURITY_PENDING:
		if (dlc->role == BT_RFCOMM_ROLE_ACCEPTOR) {
			rfcomm_send_dm(dlc->session, dlc->dlci);
		}
		__fallthrough;
	case BT_RFCOMM_STATE_INIT:
		rfcomm_dlc_drop(dlc);
		break;
	case BT_RFCOMM_STATE_CONNECTING:
	case BT_RFCOMM_STATE_CONFIG:
		dlc->state = BT_RFCOMM_STATE_DISCONNECTING;
		rfcomm_send_disc(dlc->session, dlc->dlci);
		k_work_reschedule(&dlc->rtx_work, RFCOMM_DISC_TIMEOUT);
		break;
	case BT_RFCOMM_STATE_CONNECTED:
		dlc->state = BT_RFCOMM_STATE_DISCONNECTING;
		rfcomm_dlc_tx_trigger(dlc);
		break;
	case BT_RFCOMM_STATE_DISCONNECTING:
	case BT_RFCOMM_STATE_DISCONNECTED:
		break;
	case BT_RFCOMM_STATE_IDLE:
	default:
		return -EINVAL;
	}

	return 0;
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

		dlc = rfcomm_dlcs_lookup_dlci(session, dlci);
		if (dlc == NULL) {
			dlc = rfcomm_dlc_accept(session, dlci);
			if (dlc == NULL) {
				rfcomm_send_dm(session, dlci);
				return;
			}
		}

		result = rfcomm_dlc_security(dlc);
		switch (result) {
		case RFCOMM_SECURITY_PENDING:
			dlc->state = BT_RFCOMM_STATE_SECURITY_PENDING;
			return;
		case RFCOMM_SECURITY_PASSED:
			break;
		case RFCOMM_SECURITY_REJECT:
		default:
			rfcomm_send_dm(session, dlci);
			rfcomm_dlc_drop(dlc);
			return;
		}

		if (rfcomm_send_ua(session, dlci) < 0) {
			return;
		}

		/* Cancel idle timer if any */
		k_work_cancel_delayable(&session->rtx_work);

		rfcomm_dlc_connected(dlc);
	}
}

static int rfcomm_send_pn(struct bt_rfcomm_dlc *dlc, uint8_t cr)
{
	struct bt_rfcomm_pn *pn;
	struct net_buf *buf;
	uint8_t fcs;

	buf = rfcomm_make_uih_msg(dlc->session, cr, BT_RFCOMM_PN, sizeof(*pn));

	LOG_DBG("mtu %x", dlc->mtu);

	pn = net_buf_add(buf, sizeof(*pn));
	pn->dlci = dlc->dlci;
	pn->mtu = sys_cpu_to_le16(dlc->mtu);
	if (dlc->state == BT_RFCOMM_STATE_CONFIG &&
	    (dlc->session->cfc == BT_RFCOMM_CFC_UNKNOWN ||
	     dlc->session->cfc == BT_RFCOMM_CFC_SUPPORTED)) {
		pn->credits = dlc->rx_credit;
		if (cr) {
			pn->flow_ctrl = BT_RFCOMM_PN_CFC_CMD;
		} else {
			pn->flow_ctrl = BT_RFCOMM_PN_CFC_RESP;
		}
	} else {
		/* If PN comes in already opened dlc or cfc not supported
		 * these should be 0
		 */
		pn->credits = 0U;
		pn->flow_ctrl = 0U;
	}
	pn->max_retrans = 0U;
	pn->ack_timer = 0U;
	pn->priority = 0U;

	fcs = rfcomm_calc_fcs(BT_RFCOMM_FCS_LEN_UIH, buf->data);
	net_buf_add_u8(buf, fcs);

	return rfcomm_send(dlc->session, buf);
}

static int rfcomm_send_credit(struct bt_rfcomm_dlc *dlc, uint8_t credits)
{
	struct bt_rfcomm_hdr *hdr;
	struct net_buf *buf;
	uint8_t fcs, cr;

	LOG_DBG("Dlc %p credits %d", dlc, credits);

	buf = bt_l2cap_create_pdu(NULL, 0);

	hdr = net_buf_add(buf, sizeof(*hdr));
	cr = BT_RFCOMM_UIH_CR(dlc->session->role);
	hdr->address = BT_RFCOMM_SET_ADDR(dlc->dlci, cr);
	hdr->control = BT_RFCOMM_SET_CTRL(BT_RFCOMM_UIH,
					  BT_RFCOMM_PF_UIH_CREDIT);
	hdr->length = BT_RFCOMM_SET_LEN_8(0);
	net_buf_add_u8(buf, credits);
	fcs = rfcomm_calc_fcs(BT_RFCOMM_FCS_LEN_UIH, buf->data);
	net_buf_add_u8(buf, fcs);

	return rfcomm_send(dlc->session, buf);
}

static int rfcomm_dlc_start(struct bt_rfcomm_dlc *dlc)
{
	enum security_result result;

	LOG_DBG("dlc %p", dlc);

	result = rfcomm_dlc_security(dlc);
	switch (result) {
	case RFCOMM_SECURITY_PASSED:
		dlc->mtu = MIN(dlc->mtu, dlc->session->mtu);
		dlc->state = BT_RFCOMM_STATE_CONFIG;
		rfcomm_send_pn(dlc, BT_RFCOMM_MSG_CMD_CR);
		break;
	case RFCOMM_SECURITY_PENDING:
		dlc->state = BT_RFCOMM_STATE_SECURITY_PENDING;
		break;
	case RFCOMM_SECURITY_REJECT:
	default:
		return -EIO;
	}

	return 0;
}

static void rfcomm_handle_ua(struct bt_rfcomm_session *session, uint8_t dlci)
{
	struct bt_rfcomm_dlc *dlc, *tmp;
	int err;

	if (!dlci) {
		switch (session->state) {
		case BT_RFCOMM_STATE_CONNECTING:
			session->state = BT_RFCOMM_STATE_CONNECTED;
			SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&session->dlcs, dlc, tmp, _node) {
				if (dlc->role == BT_RFCOMM_ROLE_INITIATOR &&
				    dlc->state == BT_RFCOMM_STATE_INIT) {
					if (rfcomm_dlc_start(dlc) < 0) {
						rfcomm_dlc_drop(dlc);
					}
				}
			}
			/* Disconnect session if there is no dlcs left */
			rfcomm_session_disconnect(session);
			break;
		case BT_RFCOMM_STATE_DISCONNECTING:
			session->state = BT_RFCOMM_STATE_DISCONNECTED;
			/* Cancel disc timer */
			k_work_cancel_delayable(&session->rtx_work);
			err = bt_l2cap_chan_disconnect(&session->br_chan.chan);
			if (err < 0) {
				session->state = BT_RFCOMM_STATE_IDLE;
			}
			break;
		default:
			break;
		}
	} else {
		dlc = rfcomm_dlcs_lookup_dlci(session, dlci);
		if (dlc == NULL) {
			return;
		}

		switch (dlc->state) {
		case BT_RFCOMM_STATE_CONNECTING:
			rfcomm_dlc_connected(dlc);
			break;
		case BT_RFCOMM_STATE_DISCONNECTING:
			rfcomm_dlc_drop(dlc);
			rfcomm_session_disconnect(session);
			break;
		default:
			break;
		}
	}
}

static void rfcomm_handle_dm(struct bt_rfcomm_session *session, uint8_t dlci)
{
	struct bt_rfcomm_dlc *dlc;

	LOG_DBG("dlci %d", dlci);

	dlc = rfcomm_dlcs_remove_dlci(session, dlci);
	if (dlc == NULL) {
		return;
	}

	rfcomm_dlc_disconnect(dlc);
	rfcomm_session_disconnect(session);
}

static void rfcomm_handle_msc(struct bt_rfcomm_session *session,
			      struct net_buf *buf, uint8_t cr)
{
	struct bt_rfcomm_msc *msc = (void *)buf->data;
	struct bt_rfcomm_dlc *dlc;
	uint8_t dlci = BT_RFCOMM_GET_DLCI(msc->dlci);

	LOG_DBG("dlci %d", dlci);

	dlc = rfcomm_dlcs_lookup_dlci(session, dlci);
	if (dlc == NULL) {
		return;
	}

	if (cr == BT_RFCOMM_MSG_RESP_CR) {
		return;
	}

	if (dlc->session->cfc == BT_RFCOMM_CFC_NOT_SUPPORTED) {
		/* Only FC bit affects the flow on RFCOMM level */
		if (BT_RFCOMM_GET_FC(msc->v24_signal)) {
			/* If FC bit is 1 the device is unable to accept frames.
			 * Take the semaphore with timeout K_NO_WAIT so that
			 * dlc thread will be blocked when it tries sem_take
			 * before sending the data. K_NO_WAIT timeout will make
			 * sure that RX thread will not be blocked while taking
			 * the semaphore.
			 */
			k_sem_take(&dlc->tx_credits, K_NO_WAIT);
		} else {
			/* Give the sem so that it will unblock the waiting dlc
			 * thread in sem_take().
			 */
			k_sem_give(&dlc->tx_credits);
			rfcomm_dlc_tx_trigger(dlc);
		}
	}

	rfcomm_send_msc(dlc, BT_RFCOMM_MSG_RESP_CR, msc->v24_signal);
}

static void rfcomm_handle_rls(struct bt_rfcomm_session *session,
			      struct net_buf *buf, uint8_t cr)
{
	struct bt_rfcomm_rls *rls = (void *)buf->data;
	uint8_t dlci = BT_RFCOMM_GET_DLCI(rls->dlci);
	struct bt_rfcomm_dlc *dlc;

	LOG_DBG("dlci %d", dlci);

	if (!cr) {
		/* Ignore if its a response */
		return;
	}

	dlc = rfcomm_dlcs_lookup_dlci(session, dlci);
	if (dlc == NULL) {
		return;
	}

	/* As per the ETSI same line status has to returned in the response */
	rfcomm_send_rls(dlc, BT_RFCOMM_MSG_RESP_CR, rls->line_status);
}

static void rfcomm_handle_rpn(struct bt_rfcomm_session *session,
			      struct net_buf *buf, uint8_t cr)
{
	struct bt_rfcomm_rpn default_rpn, *rpn = (void *)buf->data;
	uint8_t dlci = BT_RFCOMM_GET_DLCI(rpn->dlci);
	uint8_t data_bits, stop_bits, parity_bits;
	/* Exclude fcs to get number of value bytes */
	uint8_t value_len = buf->len - 1;

	LOG_DBG("dlci %d", dlci);

	if (!cr) {
		/* Ignore if its a response */
		return;
	}

	if (value_len == sizeof(*rpn)) {
		/* Accept all the values proposed by the sender */
		rpn->param_mask = sys_cpu_to_le16(BT_RFCOMM_RPN_PARAM_MASK_ALL);
		rfcomm_send_rpn(session, BT_RFCOMM_MSG_RESP_CR, rpn);
		return;
	}

	if (value_len != 1U) {
		return;
	}

	/* If only one value byte then current port settings has to be returned
	 * We will send default values
	 */
	default_rpn.dlci = BT_RFCOMM_SET_ADDR(dlci, 1);
	default_rpn.baud_rate = BT_RFCOMM_RPN_BAUD_RATE_9600;
	default_rpn.flow_control = BT_RFCOMM_RPN_FLOW_NONE;
	default_rpn.xoff_char = BT_RFCOMM_RPN_XOFF_CHAR;
	default_rpn.xon_char = BT_RFCOMM_RPN_XON_CHAR;
	data_bits = BT_RFCOMM_RPN_DATA_BITS_8;
	stop_bits = BT_RFCOMM_RPN_STOP_BITS_1;
	parity_bits = BT_RFCOMM_RPN_PARITY_NONE;
	default_rpn.line_settings = BT_RFCOMM_SET_LINE_SETTINGS(data_bits,
								stop_bits,
								parity_bits);
	default_rpn.param_mask = sys_cpu_to_le16(BT_RFCOMM_RPN_PARAM_MASK_ALL);

	rfcomm_send_rpn(session, BT_RFCOMM_MSG_RESP_CR, &default_rpn);
}

static void rfcomm_handle_pn(struct bt_rfcomm_session *session,
			     struct net_buf *buf, uint8_t cr)
{
	struct bt_rfcomm_pn *pn = (void *)buf->data;
	struct bt_rfcomm_dlc *dlc;

	dlc = rfcomm_dlcs_lookup_dlci(session, pn->dlci);
	if (dlc == NULL) {
		/*  Ignore if it is a response */
		if (!cr) {
			return;
		}

		if (!BT_RFCOMM_CHECK_MTU(pn->mtu)) {
			LOG_ERR("Invalid mtu %d", pn->mtu);
			rfcomm_send_dm(session, pn->dlci);
			return;
		}

		dlc = rfcomm_dlc_accept(session, pn->dlci);
		if (!dlc) {
			rfcomm_send_dm(session, pn->dlci);
			return;
		}

		LOG_DBG("Incoming connection accepted dlc %p", dlc);

		dlc->mtu = MIN(dlc->mtu, sys_le16_to_cpu(pn->mtu));

		if (pn->flow_ctrl == BT_RFCOMM_PN_CFC_CMD) {
			if (session->cfc == BT_RFCOMM_CFC_UNKNOWN) {
				session->cfc = BT_RFCOMM_CFC_SUPPORTED;
			}
			k_sem_init(&dlc->tx_credits, 0, K_SEM_MAX_LIMIT);
			rfcomm_dlc_tx_give_credits(dlc, pn->credits);
		} else {
			session->cfc = BT_RFCOMM_CFC_NOT_SUPPORTED;
		}

		dlc->state = BT_RFCOMM_STATE_CONFIG;
		rfcomm_send_pn(dlc, BT_RFCOMM_MSG_RESP_CR);
		/* Cancel idle timer if any */
		k_work_cancel_delayable(&session->rtx_work);
	} else {
		/* If its a command */
		if (cr) {
			if (!BT_RFCOMM_CHECK_MTU(pn->mtu)) {
				LOG_ERR("Invalid mtu %d", pn->mtu);
				rfcomm_dlc_close(dlc);
				return;
			}
			dlc->mtu = MIN(dlc->mtu, sys_le16_to_cpu(pn->mtu));
			rfcomm_send_pn(dlc, BT_RFCOMM_MSG_RESP_CR);
		} else {
			if (dlc->state != BT_RFCOMM_STATE_CONFIG) {
				return;
			}

			dlc->mtu = MIN(dlc->mtu, sys_le16_to_cpu(pn->mtu));
			if (pn->flow_ctrl == BT_RFCOMM_PN_CFC_RESP) {
				if (session->cfc == BT_RFCOMM_CFC_UNKNOWN) {
					session->cfc = BT_RFCOMM_CFC_SUPPORTED;
				}
				k_sem_init(&dlc->tx_credits, 0, K_SEM_MAX_LIMIT);
				rfcomm_dlc_tx_give_credits(dlc, pn->credits);
			} else {
				session->cfc = BT_RFCOMM_CFC_NOT_SUPPORTED;
			}

			dlc->state = BT_RFCOMM_STATE_CONNECTING;
			rfcomm_send_sabm(session, dlc->dlci);
		}
	}
}

static void rfcomm_handle_disc(struct bt_rfcomm_session *session, uint8_t dlci)
{
	struct bt_rfcomm_dlc *dlc;

	LOG_DBG("Dlci %d", dlci);

	if (dlci) {
		dlc = rfcomm_dlcs_remove_dlci(session, dlci);
		if (dlc == NULL) {
			rfcomm_send_dm(session, dlci);
			return;
		}

		rfcomm_send_ua(session, dlci);
		rfcomm_dlc_disconnect(dlc);

		if (sys_slist_is_empty(&session->dlcs)) {
			/* Start a session idle timer */
			k_work_reschedule(&session->rtx_work, RFCOMM_IDLE_TIMEOUT);
		}
	} else {
		/* Cancel idle timer */
		k_work_cancel_delayable(&session->rtx_work);
		rfcomm_send_ua(session, 0);
		rfcomm_session_disconnected(session);
	}
}

static void rfcomm_handle_msg(struct bt_rfcomm_session *session,
			      struct net_buf *buf)
{
	struct bt_rfcomm_msg_hdr *hdr;
	uint8_t msg_type, len, cr;

	if (buf->len < sizeof(*hdr)) {
		LOG_ERR("Too small RFCOMM message");
		return;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	msg_type = BT_RFCOMM_GET_MSG_TYPE(hdr->type);
	cr = BT_RFCOMM_GET_MSG_CR(hdr->type);
	len = BT_RFCOMM_GET_LEN(hdr->len);

	LOG_DBG("msg type %x cr %x", msg_type, cr);

	switch (msg_type) {
	case BT_RFCOMM_PN:
		rfcomm_handle_pn(session, buf, cr);
		break;
	case BT_RFCOMM_MSC:
		rfcomm_handle_msc(session, buf, cr);
		break;
	case BT_RFCOMM_RLS:
		rfcomm_handle_rls(session, buf, cr);
		break;
	case BT_RFCOMM_RPN:
		rfcomm_handle_rpn(session, buf, cr);
		break;
	case BT_RFCOMM_TEST:
		if (!cr) {
			break;
		}
		rfcomm_send_test(session, BT_RFCOMM_MSG_RESP_CR, buf->data,
				 buf->len - 1);
		break;
	case BT_RFCOMM_FCON:
		if (session->cfc == BT_RFCOMM_CFC_SUPPORTED) {
			LOG_ERR("FCON received when CFC is supported ");
			return;
		}

		if (!cr) {
			break;
		}

		/* Give the sem so that it will unblock the waiting dlc threads
		 * of this session in sem_take().
		 */
		k_sem_give(&session->fc);
		rfcomm_send_fcon(session, BT_RFCOMM_MSG_RESP_CR);
		rfcomm_dlcs_tx_trigger(session);
		break;
	case BT_RFCOMM_FCOFF:
		if (session->cfc == BT_RFCOMM_CFC_SUPPORTED) {
			LOG_ERR("FCOFF received when CFC is supported ");
			return;
		}

		if (!cr) {
			break;
		}

		/* Take the semaphore with timeout K_NO_WAIT so that all the
		 * dlc threads in this session will be blocked when it tries
		 * sem_take before sending the data. K_NO_WAIT timeout will
		 * make sure that RX thread will not be blocked while taking
		 * the semaphore.
		 */
		k_sem_take(&session->fc, K_NO_WAIT);
		rfcomm_send_fcoff(session, BT_RFCOMM_MSG_RESP_CR);
		break;
	default:
		LOG_WRN("Unknown/Unsupported RFCOMM Msg type 0x%02x", msg_type);
		rfcomm_send_nsc(session, hdr->type);
		break;
	}
}

static void rfcomm_dlc_update_credits(struct bt_rfcomm_dlc *dlc)
{
	uint8_t credits;

	if (dlc->session->cfc == BT_RFCOMM_CFC_NOT_SUPPORTED) {
		return;
	}

	LOG_DBG("dlc %p credits %u", dlc, dlc->rx_credit);

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

	LOG_DBG("dlci %d, pf %d", dlci, pf);

	dlc = rfcomm_dlcs_lookup_dlci(session, dlci);
	if (!dlc) {
		LOG_ERR("Data recvd in non existing DLC");
		rfcomm_send_dm(session, dlci);
		return;
	}

	LOG_DBG("dlc %p rx credit %d", dlc, dlc->rx_credit);

	if (dlc->state != BT_RFCOMM_STATE_CONNECTED) {
		return;
	}

	if (pf == BT_RFCOMM_PF_UIH_CREDIT) {
		if (buf->len == 0) {
			LOG_WRN("Data recvd is invalid");
			return;
		}
		rfcomm_dlc_tx_give_credits(dlc, net_buf_pull_u8(buf));
	}

	if (buf->len > BT_RFCOMM_FCS_SIZE) {
		if (dlc->session->cfc == BT_RFCOMM_CFC_SUPPORTED &&
		    !dlc->rx_credit) {
			LOG_ERR("Data recvd when rx credit is 0");
			rfcomm_dlc_close(dlc);
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
	uint8_t fcs, cr;

	if (!buf) {
		return -EINVAL;
	}

	LOG_DBG("dlc %p tx credit %d", dlc, k_sem_count_get(&dlc->tx_credits));

	if (dlc->state != BT_RFCOMM_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	if (buf->len > dlc->mtu) {
		return -EMSGSIZE;
	}

	/* length */
	if (buf->len > BT_RFCOMM_MAX_LEN_8) {
		/* Length is 2 byte */
		net_buf_push_le16(buf, BT_RFCOMM_SET_LEN_16(buf->len));
	} else {
		net_buf_push_u8(buf, BT_RFCOMM_SET_LEN_8(buf->len));
	}

	/* control */
	net_buf_push_u8(buf, BT_RFCOMM_SET_CTRL(BT_RFCOMM_UIH,
					BT_RFCOMM_PF_UIH_NO_CREDIT));
	/* address */
	cr = BT_RFCOMM_UIH_CR(dlc->session->role);
	net_buf_push_u8(buf, BT_RFCOMM_SET_ADDR(dlc->dlci, cr));

	fcs = rfcomm_calc_fcs(BT_RFCOMM_FCS_LEN_UIH, buf->data);
	net_buf_add_u8(buf, fcs);

	k_fifo_put(&dlc->tx_queue, buf);
	rfcomm_dlc_tx_trigger(dlc);

	return buf->len;
}

static int rfcomm_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_rfcomm_session *session = RFCOMM_SESSION(chan);
	struct bt_rfcomm_hdr *hdr = (void *)buf->data;
	struct bt_rfcomm_hdr_ext *hdr_ext = (void *)buf->data;
	uint8_t dlci, frame_type, fcs, fcs_len;
	uint16_t msg_len;
	uint16_t hdr_len;

	/* Need to consider FCS also*/
	if (buf->len < (sizeof(*hdr) + sizeof(fcs))) {
		LOG_ERR("Too small RFCOMM Frame");
		return 0;
	}

	dlci = BT_RFCOMM_GET_DLCI(hdr->address);
	frame_type = BT_RFCOMM_GET_FRAME_TYPE(hdr->control);

	LOG_DBG("session %p dlci %x type %x", session, dlci, frame_type);

	if (BT_RFCOMM_LEN_EXTENDED(hdr->length)) {
		msg_len = BT_RFCOMM_GET_LEN_EXTENDED(hdr_ext->hdr.length, hdr_ext->second_length);
		hdr_len = sizeof(*hdr_ext);
	} else {
		msg_len = BT_RFCOMM_GET_LEN(hdr->length);
		hdr_len = sizeof(*hdr);
	}

	if (buf->len < (hdr_len + msg_len + sizeof(fcs))) {
		LOG_ERR("Too small RFCOMM information (%u < %zu)", buf->len,
			hdr_len + msg_len + sizeof(fcs));
		return 0;
	}

	fcs_len = (frame_type == BT_RFCOMM_UIH) ? BT_RFCOMM_FCS_LEN_UIH : hdr_len;
	fcs = *(net_buf_tail(buf) - sizeof(fcs));
	if (!rfcomm_check_fcs(fcs_len, buf->data, fcs)) {
		LOG_ERR("FCS check failed");
		return 0;
	}

	net_buf_pull(buf, hdr_len);

	switch (frame_type) {
	case BT_RFCOMM_SABM:
		rfcomm_handle_sabm(session, dlci);
		break;
	case BT_RFCOMM_UIH:
		if (!dlci) {
			rfcomm_handle_msg(session, buf);
		} else {
			rfcomm_handle_data(session, buf, dlci, BT_RFCOMM_GET_PF(hdr->control));
		}
		break;
	case BT_RFCOMM_DISC:
		rfcomm_handle_disc(session, dlci);
		break;
	case BT_RFCOMM_UA:
		rfcomm_handle_ua(session, dlci);
		break;
	case BT_RFCOMM_DM:
		rfcomm_handle_dm(session, dlci);
		break;
	default:
		LOG_WRN("Unknown/Unsupported RFCOMM Frame type 0x%02x", frame_type);
		break;
	}

	return 0;
}

static void rfcomm_encrypt_change(struct bt_l2cap_chan *chan,
				  uint8_t hci_status)
{
	struct bt_rfcomm_session *session = RFCOMM_SESSION(chan);
	struct bt_conn *conn = chan->conn;
	struct bt_rfcomm_dlc *dlc, *tmp;

	LOG_DBG("session %p status 0x%02x encr 0x%02x", session, hci_status, conn->encrypt);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&session->dlcs, dlc, tmp, _node) {
		if (dlc->state != BT_RFCOMM_STATE_SECURITY_PENDING) {
			continue;
		}

		if (hci_status || !conn->encrypt ||
		    conn->sec_level < dlc->required_sec_level) {
			rfcomm_dlc_close(dlc);
			continue;
		}

		if (dlc->role == BT_RFCOMM_ROLE_ACCEPTOR) {
			rfcomm_send_ua(session, dlc->dlci);
			rfcomm_dlc_connected(dlc);
		} else {
			dlc->mtu = MIN(dlc->mtu, session->mtu);
			dlc->state = BT_RFCOMM_STATE_CONFIG;
			rfcomm_send_pn(dlc, BT_RFCOMM_MSG_CMD_CR);
		}
	}
}

static void rfcomm_session_rtx_timeout(struct k_work *work)
{
	struct bt_rfcomm_session *session = SESSION_RTX(work);

	LOG_WRN("session %p state %d timeout", session, session->state);

	switch (session->state) {
	case BT_RFCOMM_STATE_CONNECTED:
		rfcomm_session_disconnect(session);
		break;
	case BT_RFCOMM_STATE_DISCONNECTING:
		session->state = BT_RFCOMM_STATE_DISCONNECTED;
		if (bt_l2cap_chan_disconnect(&session->br_chan.chan) < 0) {
			session->state = BT_RFCOMM_STATE_IDLE;
		}
		break;
	}
}

static struct bt_rfcomm_session *rfcomm_session_new(bt_rfcomm_role_t role)
{
	static const struct bt_l2cap_chan_ops ops = {
		.connected = rfcomm_connected,
		.disconnected = rfcomm_disconnected,
		.recv = rfcomm_recv,
		.encrypt_change = rfcomm_encrypt_change,
	};

	ARRAY_FOR_EACH(bt_rfcomm_pool, i) {
		struct bt_rfcomm_session *session = &bt_rfcomm_pool[i];

		if (session->br_chan.chan.conn != NULL) {
			continue;
		}

		LOG_DBG("session %p initialized", session);

		session->br_chan.chan.ops = &ops;
		session->br_chan.rx.mtu	= CONFIG_BT_RFCOMM_L2CAP_MTU;
		session->state = BT_RFCOMM_STATE_INIT;
		session->role = role;
		session->cfc = BT_RFCOMM_CFC_UNKNOWN;
		sys_slist_init(&session->dlcs);
		k_work_init_delayable(&session->rtx_work,
				      rfcomm_session_rtx_timeout);
		k_sem_init(&session->fc, 0, 1);

		return session;
	}

	return NULL;
}

int bt_rfcomm_dlc_connect(struct bt_conn *conn, struct bt_rfcomm_dlc *dlc,
			  uint8_t channel)
{
	struct bt_rfcomm_session *session;
	struct bt_l2cap_chan *chan;
	uint8_t dlci;
	int ret;

	LOG_DBG("conn %p dlc %p channel %d", conn, dlc, channel);

	if (!dlc) {
		return -EINVAL;
	}

	if (!conn || conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	if (channel < RFCOMM_CHANNEL_START || channel > RFCOMM_CHANNEL_END) {
		return -EINVAL;
	}

	if (!BT_RFCOMM_CHECK_MTU(dlc->mtu)) {
		return -EINVAL;
	}

	session = rfcomm_sessions_lookup_bt_conn(conn);
	if (!session) {
		session = rfcomm_session_new(BT_RFCOMM_ROLE_INITIATOR);
		if (!session) {
			return -ENOMEM;
		}
	}

	dlci = BT_RFCOMM_DLCI(session->role, channel);

	if (rfcomm_dlcs_lookup_dlci(session, dlci) != NULL) {
		return -EBUSY;
	}

	rfcomm_dlc_init(dlc, session, dlci, BT_RFCOMM_ROLE_INITIATOR);

	switch (session->state) {
	case BT_RFCOMM_STATE_INIT:
		if (session->role == BT_RFCOMM_ROLE_ACCEPTOR) {
			/* There is an ongoing incoming conn */
			break;
		}
		chan = &session->br_chan.chan;
		BR_CHAN(chan)->required_sec_level = dlc->required_sec_level;
		ret = bt_l2cap_chan_connect(conn, chan, BT_L2CAP_PSM_RFCOMM);
		if (ret < 0) {
			session->state = BT_RFCOMM_STATE_IDLE;
			goto fail;
		}
		session->state = BT_RFCOMM_STATE_CONNECTING;
		break;
	case BT_RFCOMM_STATE_CONNECTING:
		break;
	case BT_RFCOMM_STATE_CONNECTED:
		ret = rfcomm_dlc_start(dlc);
		if (ret < 0) {
			goto fail;
		}
		/* Cancel idle timer if any */
		k_work_cancel_delayable(&session->rtx_work);
		break;
	default:
		LOG_ERR("Invalid session state %d", session->state);
		ret = -EINVAL;
		goto fail;
	}

	return 0;

fail:
	(void)rfcomm_dlcs_remove_dlci(session, dlc->dlci);
	dlc->state = BT_RFCOMM_STATE_IDLE;
	dlc->session = NULL;
	return ret;
}

int bt_rfcomm_dlc_disconnect(struct bt_rfcomm_dlc *dlc)
{
	LOG_DBG("dlc %p", dlc);

	if (!dlc) {
		return -EINVAL;
	}

	if (dlc->state == BT_RFCOMM_STATE_CONNECTED) {
		/* This is to handle user initiated disconnect to send pending
		 * bufs in the queue before disconnecting
		 * Queue a dummy buffer (in case if queue is empty) to wake up
		 * and stop the tx thread.
		 */
		dlc->state = BT_RFCOMM_STATE_USER_DISCONNECT;
		rfcomm_dlc_tx_trigger(dlc);
		k_work_reschedule(&dlc->rtx_work, RFCOMM_DISC_TIMEOUT);

		return 0;
	}

	return rfcomm_dlc_close(dlc);
}

static int rfcomm_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
			 struct bt_l2cap_chan **chan)
{
	struct bt_rfcomm_session *session;

	LOG_DBG("conn %p", conn);

	session = rfcomm_session_new(BT_RFCOMM_ROLE_ACCEPTOR);
	if (session) {
		*chan = &session->br_chan.chan;
		return 0;
	}

	LOG_ERR("No available RFCOMM context for conn %p", conn);

	return -ENOMEM;
}

void bt_rfcomm_init(void)
{
	__maybe_unused int err;

	static bool initialized;
	static struct bt_l2cap_server server = {
		.psm       = BT_L2CAP_PSM_RFCOMM,
		.accept    = rfcomm_accept,
		.sec_level = BT_SECURITY_L1,
	};

	if (initialized) {
		return;
	}

	err = bt_l2cap_br_server_register(&server);
	if ((err != 0) && (err != -EEXIST)) {
		LOG_ERR("Failed to register L2CAP server for RFCOMM (err %d)", err);
		return;
	}

	initialized = true;
}
