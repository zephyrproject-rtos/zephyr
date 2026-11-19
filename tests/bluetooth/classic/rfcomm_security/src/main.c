/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/classic/l2cap_br.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "host/conn_internal.h"
#include "host/classic/l2cap_br_internal.h"
#include "host/classic/rfcomm_internal.h"

#define PEER_CID 0x0040U
#define DLC_CHANNEL 2U

static struct bt_conn *conn;
static struct bt_rfcomm_session *session;
static struct bt_rfcomm_dlc dlc;
static size_t disconnected_count;

NET_BUF_POOL_FIXED_DEFINE(rx_pool, 1, 32, 0, NULL);

static void disconnected(struct bt_rfcomm_dlc *channel)
{
	ARG_UNUSED(channel);

	disconnected_count++;
}

static struct bt_rfcomm_dlc_ops dlc_ops = {
	.disconnected = disconnected,
};

static void clear_pdus(struct bt_l2cap_br_chan *chan)
{
	sys_snode_t *node;

	node = sys_slist_get(&chan->_pdu_tx_queue);
	while (node != NULL) {
		net_buf_unref(CONTAINER_OF(node, struct net_buf, node));
		node = sys_slist_get(&chan->_pdu_tx_queue);
	}
}

static void create_session(void)
{
	struct bt_l2cap_chan *chan;
	struct net_buf *buf;
	uint8_t *header;

	buf = net_buf_alloc(&rx_pool, K_NO_WAIT);
	zassert_not_null(buf);

	net_buf_add_le16(buf, sizeof(struct bt_l2cap_sig_hdr) + sizeof(struct bt_l2cap_conn_req));
	net_buf_add_le16(buf, BT_L2CAP_CID_BR_SIG);
	net_buf_add_u8(buf, BT_L2CAP_CONN_REQ);
	net_buf_add_u8(buf, 1);
	net_buf_add_le16(buf, sizeof(struct bt_l2cap_conn_req));
	net_buf_add_le16(buf, BT_L2CAP_PSM_RFCOMM);
	net_buf_add_le16(buf, PEER_CID);
	bt_l2cap_br_recv(conn, buf);

	chan = bt_l2cap_br_lookup_tx_cid(conn, PEER_CID);
	zassert_not_null(chan);
	session = CONTAINER_OF(chan, struct bt_rfcomm_session, br_chan.chan);

	/* Model the established L2CAP bearer; exercise RFCOMM setup with SABM. */
	session->br_chan.state = BT_L2CAP_CONNECTED;
	session->br_chan.tx.mtu = session->br_chan.rx.mtu;
	chan->ops->connected(chan);

	buf = net_buf_alloc(&rx_pool, K_NO_WAIT);
	zassert_not_null(buf);
	header = net_buf_add(buf, sizeof(struct bt_rfcomm_hdr));
	header[0] = BT_RFCOMM_SET_ADDR(0, BT_RFCOMM_CMD_CR(BT_RFCOMM_ROLE_INITIATOR));
	header[1] = BT_RFCOMM_SET_CTRL(BT_RFCOMM_SABM, BT_RFCOMM_PF_NON_UIH);
	header[2] = BT_RFCOMM_SET_LEN_8(0);
	net_buf_add_u8(buf, UINT8_MAX - crc8(header, sizeof(struct bt_rfcomm_hdr),
					  CRC8_REFLECT_POLY, UINT8_MAX, true));
	zassert_ok(chan->ops->recv(chan, buf));
	net_buf_unref(buf);
	zassert_equal(session->state, BT_RFCOMM_STATE_CONNECTED);
	clear_pdus(&session->br_chan);
}

static void *setup(void)
{
	static const bt_addr_t addr = { .val = { 1, 2, 3, 4, 5, 6 } };

	conn = bt_conn_add_br(&addr);
	zassert_not_null(conn);
	bt_rfcomm_init();

	return NULL;
}

static void before(void *fixture)
{
	struct bt_l2cap_chan *sig;

	ARG_UNUSED(fixture);

	conn->state = BT_CONN_CONNECTED;
	conn->sec_level = BT_SECURITY_L2;
	conn->required_sec_level = BT_SECURITY_L2;
	conn->encrypt = 1;
	conn->br.features[1][0] |= BIT(0);
	atomic_clear_bit(conn->flags, BT_CONN_BR_PAIRING);
	atomic_clear_bit(conn->flags, BT_CONN_BR_GENERAL_BONDING);
	bt_l2cap_br_connected(conn);
	create_session();

	sig = bt_l2cap_br_lookup_rx_cid(conn, BT_L2CAP_CID_BR_SIG);
	zassert_not_null(sig);
	clear_pdus(CONTAINER_OF(sig, struct bt_l2cap_br_chan, chan));

	memset(&dlc, 0, sizeof(dlc));
	dlc.ops = &dlc_ops;
	dlc.mtu = BT_RFCOMM_SIG_MIN_MTU;
	dlc.required_sec_level = BT_SECURITY_L3;
	disconnected_count = 0;
	atomic_set_bit(conn->flags, BT_CONN_BR_PAIRING);
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	bt_l2cap_br_disconnected(conn);
}

ZTEST_SUITE(rfcomm_security, NULL, setup, before, after, NULL);

static void connect_during_pairing(void)
{
	zassert_ok(bt_rfcomm_dlc_connect(conn, &dlc, DLC_CHANNEL));
	zassert_equal(dlc.state, BT_RFCOMM_STATE_SECURITY_PENDING);
	zassert_equal_ptr(dlc.session, session);
	zassert_true(sys_slist_is_empty(&session->br_chan._pdu_tx_queue));
	zassert_equal(conn->required_sec_level, BT_SECURITY_L2);
	zassert_false(atomic_test_bit(conn->flags, BT_CONN_BR_GENERAL_BONDING));
}

ZTEST(rfcomm_security, test_peer_pairing_success)
{
	struct net_buf *buf;
	const struct bt_rfcomm_hdr *header;
	const struct bt_rfcomm_msg_hdr *msg;
	const struct bt_rfcomm_pn *pn;

	connect_during_pairing();

	conn->sec_level = BT_SECURITY_L3;
	bt_conn_security_changed(conn, 0, BT_SECURITY_ERR_SUCCESS);

	zassert_equal(dlc.state, BT_RFCOMM_STATE_CONFIG);
	zassert_equal(disconnected_count, 0);
	zassert_equal(sys_slist_len(&session->br_chan._pdu_tx_queue), 1);
	buf = SYS_SLIST_PEEK_HEAD_CONTAINER(&session->br_chan._pdu_tx_queue, buf, node);
	zassert_not_null(buf);
	zassert_equal(buf->len, sizeof(struct bt_l2cap_hdr) + sizeof(*header) + sizeof(*msg) +
			       sizeof(*pn) + sizeof(uint8_t));
	header = (const void *)(buf->data + sizeof(struct bt_l2cap_hdr));
	msg = (const void *)(header + 1);
	pn = (const void *)(msg + 1);
	zassert_equal(BT_RFCOMM_GET_FRAME_TYPE(header->control), BT_RFCOMM_UIH);
	zassert_equal(BT_RFCOMM_GET_MSG_TYPE(msg->type), BT_RFCOMM_PN);
	zassert_equal(BT_RFCOMM_GET_MSG_CR(msg->type), BT_RFCOMM_MSG_CMD_CR);
	zassert_equal(pn->dlci, dlc.dlci);

	bt_conn_security_changed(conn, 0, BT_SECURITY_ERR_SUCCESS);
	zassert_equal(sys_slist_len(&session->br_chan._pdu_tx_queue), 1);
}

static void check_dlc_closed(void)
{
	zassert_is_null(dlc.session);
	zassert_equal(dlc.state, BT_RFCOMM_STATE_IDLE);
	zassert_equal(disconnected_count, 1);
	zassert_true(sys_slist_is_empty(&session->dlcs));
	zassert_true(sys_slist_is_empty(&session->br_chan._pdu_tx_queue));
	zassert_equal(session->state, BT_RFCOMM_STATE_CONNECTED);
}

ZTEST(rfcomm_security, test_peer_pairing_failure)
{
	connect_during_pairing();

	bt_conn_security_changed(conn, BT_HCI_ERR_AUTH_FAIL, BT_SECURITY_ERR_AUTH_FAIL);
	check_dlc_closed();

	bt_conn_security_changed(conn, BT_HCI_ERR_AUTH_FAIL, BT_SECURITY_ERR_AUTH_FAIL);
	zassert_equal(disconnected_count, 1);
}

ZTEST(rfcomm_security, test_peer_pairing_insufficient_security)
{
	connect_during_pairing();

	bt_conn_security_changed(conn, 0, BT_SECURITY_ERR_SUCCESS);
	check_dlc_closed();
}
