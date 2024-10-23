/* l2cap.c - Bluetooth L2CAP Tester */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>

#include <errno.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME bttester_l2cap
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

#include "btp/btp.h"

#define L2CAP_MPS 96
#define DATA_MTU (3 * L2CAP_MPS)
#define DATA_MTU_INITIAL (2 * L2CAP_MPS)

#define CHANNELS 2
#define SERVERS 1

NET_BUF_POOL_FIXED_DEFINE(data_pool, CHANNELS, BT_L2CAP_SDU_BUF_SIZE(DATA_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static bool authorize_flag;
static uint8_t req_keysize;

static struct channel {
	uint8_t chan_id; /* Internal number that identifies L2CAP channel. */
	struct bt_l2cap_le_chan le;
	bool in_use;
	bool hold_credit;
#if defined(CONFIG_BT_L2CAP_SEG_RECV)
	unsigned int pending_credits;
	uint8_t recv_cb_buf[DATA_MTU + sizeof(struct btp_l2cap_data_received_ev)];
#else
	struct net_buf *pending_credit;
#endif
} channels[CHANNELS];

/* TODO Extend to support multiple servers */
static struct bt_l2cap_server servers[SERVERS];

#if defined(CONFIG_BT_L2CAP_SEG_RECV)
static void seg_recv_cb(struct bt_l2cap_chan *l2cap_chan, size_t sdu_len,
			off_t seg_offset, struct net_buf_simple *seg)
{
	struct btp_l2cap_data_received_ev *ev;
	struct bt_l2cap_le_chan *l2cap_le_chan = CONTAINER_OF(
			l2cap_chan, struct bt_l2cap_le_chan, chan);
	struct channel *chan = CONTAINER_OF(l2cap_le_chan, struct channel, le);

	ev = (void *)chan->recv_cb_buf;
	memcpy(&ev->data[seg_offset], seg->data, seg->len);

	/* complete SDU received */
	if (seg_offset + seg->len == sdu_len) {
		ev->chan_id = chan->chan_id;
		ev->data_length = sys_cpu_to_le16(sdu_len);

		tester_event(BTP_SERVICE_ID_L2CAP, BTP_L2CAP_EV_DATA_RECEIVED,
			     chan->recv_cb_buf, sizeof(*ev) + sdu_len);
	}

	if (chan->hold_credit) {
		chan->pending_credits++;
	} else {
		bt_l2cap_chan_give_credits(l2cap_chan, 1);
	}
}
#else
static struct net_buf *alloc_buf_cb(struct bt_l2cap_chan *chan)
{
	return net_buf_alloc(&data_pool, K_FOREVER);
}

static uint8_t recv_cb_buf[DATA_MTU + sizeof(struct btp_l2cap_data_received_ev)];

static int recv_cb(struct bt_l2cap_chan *l2cap_chan, struct net_buf *buf)
{
	struct btp_l2cap_data_received_ev *ev = (void *) recv_cb_buf;
	struct bt_l2cap_le_chan *l2cap_le_chan = CONTAINER_OF(
			l2cap_chan, struct bt_l2cap_le_chan, chan);
	struct channel *chan = CONTAINER_OF(l2cap_le_chan, struct channel, le);

	ev->chan_id = chan->chan_id;
	ev->data_length = sys_cpu_to_le16(buf->len);
	memcpy(ev->data, buf->data, buf->len);

	tester_event(BTP_SERVICE_ID_L2CAP, BTP_L2CAP_EV_DATA_RECEIVED,
		     recv_cb_buf, sizeof(*ev) + buf->len);

	if (chan->hold_credit && !chan->pending_credit) {
		/* no need for extra ref, as when returning EINPROGRESS user
		 * becomes owner of the netbuf
		 */
		chan->pending_credit = buf;
		return -EINPROGRESS;
	}

	return 0;
}
#endif

static void connected_cb(struct bt_l2cap_chan *l2cap_chan)
{
	struct btp_l2cap_connected_ev ev;
	struct bt_l2cap_le_chan *l2cap_le_chan = CONTAINER_OF(
			l2cap_chan, struct bt_l2cap_le_chan, chan);
	struct channel *chan = CONTAINER_OF(l2cap_le_chan, struct channel, le);
	struct bt_conn_info info;

	ev.chan_id = chan->chan_id;
	/* TODO: ev.psm */
	if (!bt_conn_get_info(l2cap_chan->conn, &info)) {
		switch (info.type) {
		case BT_CONN_TYPE_LE:
			ev.mtu_remote = sys_cpu_to_le16(chan->le.tx.mtu);
			ev.mps_remote = sys_cpu_to_le16(chan->le.tx.mps);
			ev.mtu_local = sys_cpu_to_le16(chan->le.rx.mtu);
			ev.mps_local = sys_cpu_to_le16(chan->le.rx.mps);
			bt_addr_le_copy(&ev.address, info.le.dst);
			break;
		case BT_CONN_TYPE_BR:
		default:
			/* TODO figure out how (if) want to handle BR/EDR */
			return;
		}
	}

	tester_event(BTP_SERVICE_ID_L2CAP, BTP_L2CAP_EV_CONNECTED, &ev, sizeof(ev));
}

static void disconnected_cb(struct bt_l2cap_chan *l2cap_chan)
{
	struct btp_l2cap_disconnected_ev ev;
	struct bt_l2cap_le_chan *l2cap_le_chan = CONTAINER_OF(
			l2cap_chan, struct bt_l2cap_le_chan, chan);
	struct channel *chan = CONTAINER_OF(l2cap_le_chan, struct channel, le);
	struct bt_conn_info info;

#if !defined(CONFIG_BT_L2CAP_SEG_RECV)
	/* release netbuf on premature disconnection */
	if (chan->pending_credit) {
		net_buf_unref(chan->pending_credit);
		chan->pending_credit = NULL;
	}
#endif

	(void)memset(&ev, 0, sizeof(struct btp_l2cap_disconnected_ev));

	/* TODO: ev.result */
	ev.chan_id = chan->chan_id;
	/* TODO: ev.psm */
	if (!bt_conn_get_info(l2cap_chan->conn, &info)) {
		switch (info.type) {
		case BT_CONN_TYPE_LE:
			bt_addr_le_copy(&ev.address, info.le.dst);
			break;
		case BT_CONN_TYPE_BR:
		default:
			/* TODO figure out how (if) want to handle BR/EDR */
			return;
		}
	}

	chan->in_use = false;

	tester_event(BTP_SERVICE_ID_L2CAP, BTP_L2CAP_EV_DISCONNECTED, &ev, sizeof(ev));
}

#if defined(CONFIG_BT_L2CAP_ECRED)
static void reconfigured_cb(struct bt_l2cap_chan *l2cap_chan)
{
	struct btp_l2cap_reconfigured_ev ev;
	struct bt_l2cap_le_chan *l2cap_le_chan = CONTAINER_OF(
			l2cap_chan, struct bt_l2cap_le_chan, chan);
	struct channel *chan = CONTAINER_OF(l2cap_le_chan, struct channel, le);

	(void)memset(&ev, 0, sizeof(ev));

	ev.chan_id = chan->chan_id;
	ev.mtu_remote = sys_cpu_to_le16(chan->le.tx.mtu);
	ev.mps_remote = sys_cpu_to_le16(chan->le.tx.mps);
	ev.mtu_local = sys_cpu_to_le16(chan->le.rx.mtu);
	ev.mps_local = sys_cpu_to_le16(chan->le.rx.mps);

	tester_event(BTP_SERVICE_ID_L2CAP, BTP_L2CAP_EV_RECONFIGURED, &ev, sizeof(ev));
}
#endif

static const struct bt_l2cap_chan_ops l2cap_ops = {
#if defined(CONFIG_BT_L2CAP_SEG_RECV)
	.seg_recv	= seg_recv_cb,
#else
	.alloc_buf	= alloc_buf_cb,
	.recv		= recv_cb,
#endif
	.connected	= connected_cb,
	.disconnected	= disconnected_cb,
#if defined(CONFIG_BT_L2CAP_ECRED)
	.reconfigured	= reconfigured_cb,
#endif
};

static struct channel *get_free_channel()
{
	uint8_t i;
	struct channel *chan;

	for (i = 0U; i < CHANNELS; i++) {
		if (channels[i].in_use) {
			continue;
		}

		chan = &channels[i];

		(void)memset(chan, 0, sizeof(*chan));
		chan->chan_id = i;

		channels[i].in_use = true;

		return chan;
	}

	return NULL;
}

static uint8_t connect(const void *cmd, uint16_t cmd_len,
		       void *rsp, uint16_t *rsp_len)
{
	const struct btp_l2cap_connect_cmd *cp = cmd;
	struct btp_l2cap_connect_rp *rp = rsp;
	struct bt_conn *conn;
	struct channel *chan = NULL;
	struct bt_l2cap_chan *allocated_channels[5] = {};
	uint16_t mtu = sys_le16_to_cpu(cp->mtu);
	uint16_t psm = sys_le16_to_cpu(cp->psm);
	uint8_t i = 0;
	bool ecfc = cp->options & BTP_L2CAP_CONNECT_OPT_ECFC;
	int err;

	if (cp->num == 0 || cp->num > CHANNELS || mtu > DATA_MTU_INITIAL) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		return BTP_STATUS_FAILED;
	}

	for (i = 0U; i < cp->num; i++) {
		chan = get_free_channel();
		if (!chan) {
			goto fail;
		}
		chan->le.chan.ops = &l2cap_ops;
		chan->le.rx.mtu = mtu;
#if defined(CONFIG_BT_L2CAP_SEG_RECV)
		chan->le.rx.mps = L2CAP_MPS;
#endif
		rp->chan_id[i] = chan->chan_id;
		allocated_channels[i] = &chan->le.chan;

		chan->hold_credit = cp->options & BTP_L2CAP_CONNECT_OPT_HOLD_CREDIT;

		bt_l2cap_chan_give_credits(&chan->le.chan, 1);
	}

	if (cp->num == 1 && !ecfc) {
		err = bt_l2cap_chan_connect(conn, &chan->le.chan, psm);
		if (err < 0) {
			goto fail;
		}
	} else if (ecfc) {
#if defined(CONFIG_BT_L2CAP_ECRED)
		err = bt_l2cap_ecred_chan_connect(conn, allocated_channels,
							psm);
		if (err < 0) {
			goto fail;
		}
#else
		goto fail;
#endif
	} else {
		LOG_ERR("Invalid 'num' parameter value");
		goto fail;
	}

	rp->num = cp->num;
	*rsp_len = sizeof(*rp) + (rp->num * sizeof(rp->chan_id[0]));

	return BTP_STATUS_SUCCESS;

fail:
	for (i = 0U; i < ARRAY_SIZE(allocated_channels); i++) {
		if (allocated_channels[i]) {
			channels[BT_L2CAP_LE_CHAN(allocated_channels[i])->ident].in_use = false;
		}
	}
	return BTP_STATUS_FAILED;
}

static uint8_t disconnect(const void *cmd, uint16_t cmd_len,
			  void *rsp, uint16_t *rsp_len)
{
	const struct btp_l2cap_disconnect_cmd *cp = cmd;
	struct channel *chan;
	int err;

	if (cp->chan_id >= CHANNELS) {
		return BTP_STATUS_FAILED;
	}

	chan = &channels[cp->chan_id];

	err = bt_l2cap_chan_disconnect(&chan->le.chan);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

#if defined(CONFIG_BT_L2CAP_ECRED)
static uint8_t reconfigure(const void *cmd, uint16_t cmd_len,
			   void *rsp, uint16_t *rsp_len)
{
	const struct btp_l2cap_reconfigure_cmd *cp = cmd;
	uint16_t mtu;
	struct bt_conn *conn;
	int err;
	struct bt_l2cap_chan *reconf_channels[CHANNELS + 1] = {};

	if (cmd_len < sizeof(*cp) ||
	    cmd_len != sizeof(*cp) + cp->num) {
		return BTP_STATUS_FAILED;
	}

	if (cp->num > CHANNELS) {
		return BTP_STATUS_FAILED;
	}

	mtu = sys_le16_to_cpu(cp->mtu);
	if (mtu > DATA_MTU) {
		return BTP_STATUS_FAILED;
	}

	for (int i = 0; i < cp->num; i++) {
		if (cp->chan_id[i] > CHANNELS) {
			return BTP_STATUS_FAILED;
		}

		reconf_channels[i] = &channels[cp->chan_id[i]].le.chan;
	}

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_l2cap_ecred_chan_reconfigure(reconf_channels, mtu);
	if (err) {
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	bt_conn_unref(conn);
	return BTP_STATUS_SUCCESS;
}
#endif

#if defined(CONFIG_BT_EATT)
static uint8_t disconnect_eatt_chans(const void *cmd, uint16_t cmd_len,
				     void *rsp, uint16_t *rsp_len)
{
	const struct btp_l2cap_disconnect_eatt_chans_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	for (int i = 0; i < cp->count; i++) {
		err = bt_eatt_disconnect_one(conn);
		if (err) {
			bt_conn_unref(conn);
			return BTP_STATUS_FAILED;
		}
	}

	bt_conn_unref(conn);
	return BTP_STATUS_SUCCESS;
}
#endif


static uint8_t send_data(const void *cmd, uint16_t cmd_len,
			 void *rsp, uint16_t *rsp_len)
{
	const struct btp_l2cap_send_data_cmd *cp = cmd;
	struct channel *chan;
	struct net_buf *buf;
	uint16_t data_len;
	int ret;

	if (cmd_len < sizeof(*cp) ||
	    cmd_len != sizeof(*cp) + sys_le16_to_cpu(cp->data_len)) {
		return BTP_STATUS_FAILED;
	}

	if (cp->chan_id >= CHANNELS) {
		return BTP_STATUS_FAILED;
	}

	chan = &channels[cp->chan_id];
	data_len = sys_le16_to_cpu(cp->data_len);


	/* FIXME: For now, fail if data length exceeds buffer length */
	if (data_len > DATA_MTU) {
		return BTP_STATUS_FAILED;
	}

	/* FIXME: For now, fail if data length exceeds remote's L2CAP SDU */
	if (data_len > chan->le.tx.mtu) {
		return BTP_STATUS_FAILED;
	}

	buf = net_buf_alloc(&data_pool, K_FOREVER);
	net_buf_reserve(buf, BT_L2CAP_SDU_CHAN_SEND_RESERVE);

	net_buf_add_mem(buf, cp->data, data_len);
	ret = bt_l2cap_chan_send(&chan->le.chan, buf);
	if (ret < 0) {
		LOG_ERR("Unable to send data: %d", -ret);
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static struct bt_l2cap_server *get_free_server(void)
{
	uint8_t i;

	for (i = 0U; i < SERVERS ; i++) {
		if (servers[i].psm) {
			continue;
		}

		return &servers[i];
	}

	return NULL;
}

static bool is_free_psm(uint16_t psm)
{
	uint8_t i;

	for (i = 0U; i < ARRAY_SIZE(servers); i++) {
		if (servers[i].psm == psm) {
			return false;
		}
	}

	return true;
}

static int accept(struct bt_conn *conn, struct bt_l2cap_server *server,
		  struct bt_l2cap_chan **l2cap_chan)
{
	struct channel *chan;

	if (bt_conn_enc_key_size(conn) < req_keysize) {
		return -EPERM;
	}

	if (authorize_flag) {
		return -EACCES;
	}

	chan = get_free_channel();
	if (!chan) {
		return -ENOMEM;
	}

	chan->le.chan.ops = &l2cap_ops;
	chan->le.rx.mtu = DATA_MTU_INITIAL;
#if defined(CONFIG_BT_L2CAP_SEG_RECV)
	chan->le.rx.mps = L2CAP_MPS;
#endif

	*l2cap_chan = &chan->le.chan;

	bt_l2cap_chan_give_credits(&chan->le.chan, 1);

	return 0;
}

static uint8_t listen(const void *cmd, uint16_t cmd_len,
		      void *rsp, uint16_t *rsp_len)
{
	const struct btp_l2cap_listen_cmd *cp = cmd;
	struct bt_l2cap_server *server;
	uint16_t psm = sys_le16_to_cpu(cp->psm);

	/* TODO: Handle cmd->transport flag */

	if (psm == 0 || !is_free_psm(psm)) {
		return BTP_STATUS_FAILED;
	}

	server = get_free_server();
	if (!server) {
		return BTP_STATUS_FAILED;
	}

	server->accept = accept;
	server->psm = psm;

	switch (cp->response) {
	case BTP_L2CAP_CONNECTION_RESPONSE_SUCCESS:
		break;
	case BTP_L2CAP_CONNECTION_RESPONSE_INSUFF_ENC_KEY:
		/* TSPX_psm_encryption_key_size_required */
		req_keysize = 16;
		break;
	case BTP_L2CAP_CONNECTION_RESPONSE_INSUFF_AUTHOR:
		authorize_flag = true;
		break;
	case BTP_L2CAP_CONNECTION_RESPONSE_INSUFF_AUTHEN:
		server->sec_level = BT_SECURITY_L3;
		break;
	case BTP_L2CAP_CONNECTION_RESPONSE_INSUFF_ENCRYPTION:
		server->sec_level = BT_SECURITY_L2;
		break;
	default:
		return BTP_STATUS_FAILED;
	}

	if (bt_l2cap_server_register(server) < 0) {
		server->psm = 0U;
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t credits(const void *cmd, uint16_t cmd_len,
		      void *rsp, uint16_t *rsp_len)
{
	const struct btp_l2cap_credits_cmd *cp = cmd;
	struct channel *chan;

	if (cp->chan_id >= CHANNELS) {
		return BTP_STATUS_FAILED;
	}

	chan = &channels[cp->chan_id];

	if (!chan->in_use) {
		return BTP_STATUS_FAILED;
	}
#if defined(CONFIG_BT_L2CAP_SEG_RECV)
	if (chan->pending_credits) {
		if (bt_l2cap_chan_give_credits(&chan->le.chan,
					       chan->pending_credits) < 0) {
			return BTP_STATUS_FAILED;
		}

		chan->pending_credits = 0;
	}
#else
	if (chan->pending_credit) {
		if (bt_l2cap_chan_recv_complete(&chan->le.chan,
						chan->pending_credit) < 0) {
			return BTP_STATUS_FAILED;
		}

		chan->pending_credit = NULL;
	}
#endif

	return BTP_STATUS_SUCCESS;
}

static uint8_t supported_commands(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	struct btp_l2cap_read_supported_commands_rp *rp = rsp;

	/* octet 0 */
	tester_set_bit(rp->data, BTP_L2CAP_READ_SUPPORTED_COMMANDS);
	tester_set_bit(rp->data, BTP_L2CAP_CONNECT);
	tester_set_bit(rp->data, BTP_L2CAP_DISCONNECT);
	tester_set_bit(rp->data, BTP_L2CAP_SEND_DATA);
	tester_set_bit(rp->data, BTP_L2CAP_LISTEN);
#if defined(CONFIG_BT_L2CAP_ECRED)
	tester_set_bit(rp->data, BTP_L2CAP_RECONFIGURE);
#endif
	/* octet 1 */
	tester_set_bit(rp->data, BTP_L2CAP_CREDITS);
#if defined(CONFIG_BT_EATT)
	tester_set_bit(rp->data, BTP_L2CAP_DISCONNECT_EATT_CHANS);
#endif

	*rsp_len = sizeof(*rp) + 2;

	return BTP_STATUS_SUCCESS;
}

static const struct btp_handler handlers[] = {
	{
		.opcode = BTP_L2CAP_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = supported_commands,
	},
	{
		.opcode = BTP_L2CAP_CONNECT,
		.expect_len = sizeof(struct btp_l2cap_connect_cmd),
		.func = connect,
	},
	{
		.opcode = BTP_L2CAP_DISCONNECT,
		.expect_len = sizeof(struct btp_l2cap_disconnect_cmd),
		.func = disconnect,
	},
	{
		.opcode = BTP_L2CAP_SEND_DATA,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = send_data,
	},
	{
		.opcode = BTP_L2CAP_LISTEN,
		.expect_len = sizeof(struct btp_l2cap_listen_cmd),
		.func = listen,
	},
	{
		.opcode = BTP_L2CAP_RECONFIGURE,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = reconfigure,
	},
	{
		.opcode = BTP_L2CAP_CREDITS,
		.expect_len = sizeof(struct btp_l2cap_credits_cmd),
		.func = credits,
	},
	{
		.opcode = BTP_L2CAP_DISCONNECT_EATT_CHANS,
		.expect_len = sizeof(struct btp_l2cap_disconnect_eatt_chans_cmd),
		.func = disconnect_eatt_chans,
	},
};

uint8_t tester_init_l2cap(void)
{
	tester_register_command_handlers(BTP_SERVICE_ID_L2CAP, handlers,
					 ARRAY_SIZE(handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_l2cap(void)
{
	return BTP_STATUS_SUCCESS;
}
