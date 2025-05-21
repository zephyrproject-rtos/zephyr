/* l2cap.c - Bluetooth L2CAP Tester */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/l2cap_br.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <sys/types.h>

#include "btp/btp.h"

#define LOG_MODULE_NAME bttester_l2cap
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

#define L2CAP_MPS        96
#define DATA_MTU         (3 * L2CAP_MPS)
#define DATA_MTU_INITIAL (2 * L2CAP_MPS)

/* CHANNELS cannot be greater than 0x7f. */
#define CHANNELS 2

#if defined(CONFIG_BT_CLASSIC)
#define SERVERS 2
#else
#define SERVERS 1
#endif /* CONFIG_BT_CLASSIC */

#if defined(CONFIG_BT_CLASSIC) && defined(CONFIG_BT_L2CAP_MAX_WINDOW_SIZE)
#define DATA_POOL_COUNT CONFIG_BT_L2CAP_MAX_WINDOW_SIZE
#else
#define DATA_POOL_COUNT CHANNELS
#endif /* CONFIG_BT_CLASSIC && CONFIG_BT_L2CAP_MAX_WINDOW_SIZE */
NET_BUF_POOL_FIXED_DEFINE(data_pool, DATA_POOL_COUNT, BT_L2CAP_SDU_BUF_SIZE(DATA_MTU),
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

#if defined(CONFIG_BT_CLASSIC)
static struct br_channel {
	uint8_t chan_id; /* Internal number that identifies L2CAP channel. */
	struct bt_l2cap_br_chan br;
	bool in_use;
	bool hold_credit;
	struct net_buf *pending_credit[DATA_POOL_COUNT];
} br_channels[CHANNELS];
#endif /* CONFIG_BT_CLASSIC */

/* TODO Extend to support multiple servers */
static struct bt_l2cap_server servers[SERVERS];
struct server_settings {
	uint8_t mode;
	uint32_t options;
};
static struct server_settings server_settings[SERVERS];

#if defined(CONFIG_BT_L2CAP_SEG_RECV)
static void seg_recv_cb(struct bt_l2cap_chan *l2cap_chan, size_t sdu_len, off_t seg_offset,
			struct net_buf_simple *seg)
{
	struct btp_l2cap_data_received_ev *ev;
	struct bt_l2cap_le_chan *l2cap_le_chan =
		CONTAINER_OF(l2cap_chan, struct bt_l2cap_le_chan, chan);
	struct channel *chan = CONTAINER_OF(l2cap_le_chan, struct channel, le);

	ev = (void *)chan->recv_cb_buf;
	memcpy(&ev->data[seg_offset], seg->data, seg->len);

	/* complete SDU received */
	if (seg_offset + seg->len == sdu_len) {
		ev->chan_id = chan->chan_id;
		ev->data_length = sys_cpu_to_le16(sdu_len);

		tester_event(BTP_SERVICE_ID_L2CAP, BTP_L2CAP_EV_DATA_RECEIVED, chan->recv_cb_buf,
			     sizeof(*ev) + sdu_len);
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
	.seg_recv = seg_recv_cb,
#else
	.alloc_buf = alloc_buf_cb,
	.recv = recv_cb,
#endif
	.connected = connected_cb,
	.disconnected = disconnected_cb,
#if defined(CONFIG_BT_L2CAP_ECRED)
	.reconfigured = reconfigured_cb,
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

typedef void (*btp_l2cap_chan_allocated_cb_t)(uint8_t chan_id, void *user_data);

#if defined(CONFIG_BT_CLASSIC)
static void br_connected_cb(struct bt_l2cap_chan *l2cap_chan)
{
	struct btp_l2cap_connected_ev ev;
	struct bt_l2cap_br_chan *l2cap_br_chan = CONTAINER_OF(
			l2cap_chan, struct bt_l2cap_br_chan, chan);
	struct br_channel *chan = CONTAINER_OF(l2cap_br_chan, struct br_channel, br);
	struct bt_conn_info info;

	ev.chan_id = chan->chan_id;

	/* TODO: ev.psm */
	if (bt_conn_get_info(l2cap_chan->conn, &info) != 0) {
		return;
	}

	switch (info.type) {
	case BT_CONN_TYPE_BR:
		ev.mtu_remote = sys_cpu_to_le16(chan->br.tx.mtu);
		ev.mps_remote = sys_cpu_to_le16(chan->br.tx.mtu);
		ev.mtu_local = sys_cpu_to_le16(chan->br.rx.mtu);
		ev.mps_local = sys_cpu_to_le16(chan->br.rx.mtu);
		ev.address.type = BTP_BR_ADDRESS_TYPE;
		bt_addr_copy(&ev.address.a, info.br.dst);
		break;
	default:
		/* Unsupported transport */
		return;
	}

	tester_event(BTP_SERVICE_ID_L2CAP, BTP_L2CAP_EV_CONNECTED, &ev, sizeof(ev));
}

static void br_disconnected_cb(struct bt_l2cap_chan *l2cap_chan)
{
	struct btp_l2cap_disconnected_ev ev;
	struct bt_l2cap_br_chan *l2cap_br_chan = CONTAINER_OF(
			l2cap_chan, struct bt_l2cap_br_chan, chan);
	struct br_channel *chan = CONTAINER_OF(l2cap_br_chan, struct br_channel, br);
	struct bt_conn_info info;

	/* release netbuf on premature disconnection */
	ARRAY_FOR_EACH(chan->pending_credit, i) {
		if (chan->pending_credit[i] != NULL) {
			net_buf_unref(chan->pending_credit[i]);
			chan->pending_credit[i] = NULL;
		}
	}

	(void)memset(&ev, 0, sizeof(struct btp_l2cap_disconnected_ev));

	/* TODO: ev.result */
	ev.chan_id = chan->chan_id;

	chan->in_use = false;

	/* TODO: ev.psm */
	if (bt_conn_get_info(l2cap_chan->conn, &info) != 0) {
		return;
	}

	switch (info.type) {
	case BT_CONN_TYPE_BR:
		ev.address.type = BTP_BR_ADDRESS_TYPE;
		bt_addr_copy(&ev.address.a, info.br.dst);
		break;
	default:
		/* Unsupported transport */
		return;
	}

	tester_event(BTP_SERVICE_ID_L2CAP, BTP_L2CAP_EV_DISCONNECTED, &ev, sizeof(ev));
}

static struct net_buf *br_alloc_buf_cb(struct bt_l2cap_chan *chan)
{
	return net_buf_alloc(&data_pool, K_NO_WAIT);
}

static uint8_t br_recv_cb_buf[DATA_MTU + sizeof(struct btp_l2cap_data_received_ev)];

static int br_recv_cb(struct bt_l2cap_chan *l2cap_chan, struct net_buf *buf)
{
	struct btp_l2cap_data_received_ev *ev = (void *)br_recv_cb_buf;
	struct bt_l2cap_br_chan *l2cap_br_chan = CONTAINER_OF(
			l2cap_chan, struct bt_l2cap_br_chan, chan);
	struct br_channel *chan = CONTAINER_OF(l2cap_br_chan, struct br_channel, br);

	ev->chan_id = chan->chan_id;
	ev->data_length = sys_cpu_to_le16(buf->len);
	memcpy(ev->data, buf->data, buf->len);

	tester_event(BTP_SERVICE_ID_L2CAP, BTP_L2CAP_EV_DATA_RECEIVED,
		     br_recv_cb_buf, sizeof(*ev) + buf->len);

	if (chan->hold_credit && (net_buf_id(buf) < (int)ARRAY_SIZE(chan->pending_credit))) {
		chan->pending_credit[net_buf_id(buf)] = buf;
		return -EINPROGRESS;
	}

	return 0;
}

static const struct bt_l2cap_chan_ops br_l2cap_ops = {
	.connected = br_connected_cb,
	.disconnected = br_disconnected_cb,
	.alloc_buf = br_alloc_buf_cb,
	.recv = br_recv_cb,
};

static struct br_channel *get_free_br_channel(void)
{
	uint8_t i;
	struct br_channel *chan;

	for (i = 0U; i < CHANNELS; i++) {
		if (br_channels[i].in_use) {
			continue;
		}

		chan = &br_channels[i];

		(void)memset(chan, 0, sizeof(*chan));
		chan->chan_id = i + ARRAY_SIZE(channels);

		br_channels[i].in_use = true;

		return chan;
	}

	return NULL;
}

static uint8_t br_connect(const struct btp_l2cap_connect_v2_cmd *cp,
			  btp_l2cap_chan_allocated_cb_t cb, void *user_data)
{
	struct bt_conn *conn;
	struct br_channel *br_chan = NULL;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	br_chan = get_free_br_channel();
	if (br_chan == NULL) {
		goto fail;
	}
	br_chan->br.chan.ops = &br_l2cap_ops;
	br_chan->br.rx.mtu = sys_le16_to_cpu(cp->mtu);
	cb(br_chan->chan_id, user_data);

#if defined(CONFIG_BT_L2CAP_RET_FC)
	switch (cp->mode) {
	case BTP_L2CAP_CONNECT_V2_MODE_RET:
		br_chan->br.rx.mode = BT_L2CAP_BR_LINK_MODE_RET;
		br_chan->br.rx.max_transmit = 3;
		break;
	case BTP_L2CAP_CONNECT_V2_MODE_FC:
		br_chan->br.rx.mode = BT_L2CAP_BR_LINK_MODE_FC;
		br_chan->br.rx.max_transmit = 3;
		break;
	case BTP_L2CAP_CONNECT_V2_MODE_ERET:
		br_chan->br.rx.mode = BT_L2CAP_BR_LINK_MODE_ERET;
		br_chan->br.rx.max_transmit = 3;
		break;
	case BTP_L2CAP_CONNECT_V2_MODE_STREAM:
		br_chan->br.rx.mode = BT_L2CAP_BR_LINK_MODE_STREAM;
		br_chan->br.rx.max_transmit = 0;
		break;
	case BTP_L2CAP_CONNECT_V2_MODE_BASIC:
		br_chan->br.rx.mode = BT_L2CAP_BR_LINK_MODE_BASIC;
		br_chan->br.rx.max_transmit = 0;
		break;
	default:
		goto fail;
	}

	if (cp->options & BTP_L2CAP_CONNECT_V2_OPT_EXT_WIN_SIZE) {
		br_chan->br.rx.extended_control = true;
	} else {
		br_chan->br.rx.extended_control = false;
	}

	if (cp->options & BTP_L2CAP_CONNECT_V2_OPT_MODE_OPTIONAL) {
		br_chan->br.rx.optional = true;
	} else {
		br_chan->br.rx.optional = false;
	}

	br_chan->br.rx.max_window = CONFIG_BT_L2CAP_MAX_WINDOW_SIZE;
	if (cp->options & BTP_L2CAP_CONNECT_V2_OPT_NO_FCS) {
		br_chan->br.rx.fcs = BT_L2CAP_BR_FCS_NO;
	} else {
		br_chan->br.rx.fcs = BT_L2CAP_BR_FCS_16BIT;
	}

	br_chan->hold_credit = (cp->options & BTP_L2CAP_CONNECT_V2_OPT_HOLD_CREDIT) != 0;
#endif /* CONFIG_BT_L2CAP_RET_FC */

	err = bt_l2cap_chan_connect(conn, &br_chan->br.chan, sys_le16_to_cpu(cp->psm));
	if (err < 0) {
		goto fail;
	}

	bt_conn_unref(conn);
	return BTP_STATUS_SUCCESS;

fail:
	if (br_chan != NULL) {
		br_chan->in_use = false;
	}
	bt_conn_unref(conn);
	return BTP_STATUS_FAILED;
}
#else
static uint8_t br_connect(const struct btp_l2cap_connect_v2_cmd *cp,
			  btp_l2cap_chan_allocated_cb_t cb, void *user_data)
{
	return BTP_STATUS_FAILED;
}
#endif /* CONFIG_BT_CLASSIC */

static uint8_t _connect(const struct btp_l2cap_connect_v2_cmd *cp,
			btp_l2cap_chan_allocated_cb_t cb, void *user_data)
{
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

	if (cp->address.type == BTP_BR_ADDRESS_TYPE) {
		return br_connect(cp, cb, user_data);
	}

	if (cp->mode != BTP_L2CAP_CONNECT_V2_MODE_NONE) {
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
		cb(chan->chan_id, user_data);
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

	return BTP_STATUS_SUCCESS;

fail:
	for (i = 0U; i < ARRAY_SIZE(allocated_channels); i++) {
		if (allocated_channels[i]) {
			channels[BT_L2CAP_LE_CHAN(allocated_channels[i])->ident].in_use = false;
		}
	}
	return BTP_STATUS_FAILED;
}

struct btp_l2cap_connect_data {
	void *rsp;
	uint16_t *rsp_len;
	bool initialized;
};

static void btp_l2cap_chan_allocated_cb(uint8_t chan_id, void *user_data)
{
	struct btp_l2cap_connect_data *data = user_data;
	struct btp_l2cap_connect_rp *rp = data->rsp;

	if (data->initialized == false) {
		rp->num = 0;
		data->initialized = true;
	}

	rp->chan_id[rp->num] = chan_id;
	rp->num++;

	*data->rsp_len = sizeof(*rp) + (rp->num * sizeof(rp->chan_id[0]));
}

static uint8_t connect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_l2cap_connect_cmd *cp = cmd;
	struct btp_l2cap_connect_v2_cmd cp_v2;
	struct btp_l2cap_connect_data data;

	data.rsp = rsp;
	data.rsp_len = rsp_len;
	data.initialized = false;

	cp_v2.address = cp->address;
	cp_v2.psm = cp->psm;
	cp_v2.mtu = cp->mtu;
	cp_v2.num = cp->num;
	cp_v2.mode = BTP_L2CAP_CONNECT_V2_MODE_NONE;
	cp_v2.options = cp->options;

	return _connect(&cp_v2, btp_l2cap_chan_allocated_cb, &data);
}

static void btp_l2cap_chan_allocated_v2_cb(uint8_t chan_id, void *user_data)
{
	struct btp_l2cap_connect_data *data = user_data;
	struct btp_l2cap_connect_v2_rp *rp = data->rsp;

	if (data->initialized == false) {
		rp->num = 0;
		data->initialized = true;
	}

	rp->chan_id[rp->num] = chan_id;
	rp->num++;

	*data->rsp_len = sizeof(*rp) + (rp->num * sizeof(rp->chan_id[0]));
}

static uint8_t connect_v2(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_l2cap_connect_v2_cmd *cp = cmd;
	struct btp_l2cap_connect_data data;

	data.rsp = rsp;
	data.rsp_len = rsp_len;
	data.initialized = false;

	return _connect(cp, btp_l2cap_chan_allocated_v2_cb, &data);
}

#if defined(CONFIG_BT_CLASSIC)
static uint8_t br_disconnect(uint8_t chan_id)
{
	struct br_channel *br_chan;
	int err;

	if (chan_id >= CHANNELS) {
		return BTP_STATUS_FAILED;
	}

	br_chan = &br_channels[chan_id];

	err = bt_l2cap_chan_disconnect(&br_chan->br.chan);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#else
static uint8_t br_disconnect(uint8_t chan_id)
{
	return BTP_STATUS_FAILED;
}
#endif /* CONFIG_BT_CLASSIC */

static uint8_t disconnect(const void *cmd, uint16_t cmd_len,
			  void *rsp, uint16_t *rsp_len)
{
	const struct btp_l2cap_disconnect_cmd *cp = cmd;
	struct channel *chan;
	int err;

	if (IS_ENABLED(CONFIG_BT_CLASSIC)) {
		uint8_t chan_id;

		chan_id = cp->chan_id;
		if (chan_id >= ARRAY_SIZE(channels)) {
			chan_id = chan_id - ARRAY_SIZE(channels);
			return br_disconnect(chan_id);
		}
	}

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
	uint16_t mps;
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

	mps = MIN(L2CAP_MPS, BT_L2CAP_RX_MTU);
	err = bt_l2cap_ecred_chan_reconfigure_explicit(reconf_channels, cp->num, mtu, mps);
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

#if defined(CONFIG_BT_CLASSIC)
static uint8_t br_send_data(uint8_t chan_id, const struct btp_l2cap_send_data_cmd *cp,
			    void *rsp, uint16_t *rsp_len)
{
	struct br_channel *br_chan;
	struct net_buf *buf;
	uint16_t data_len;
	int err;

	if (chan_id >= CHANNELS) {
		return BTP_STATUS_FAILED;
	}

	br_chan = &br_channels[chan_id];
	data_len = sys_le16_to_cpu(cp->data_len);

	if (data_len > DATA_MTU) {
		return BTP_STATUS_FAILED;
	}

	if (data_len > br_chan->br.tx.mtu) {
		return BTP_STATUS_FAILED;
	}

	buf = net_buf_alloc(&data_pool, K_FOREVER);
	net_buf_reserve(buf, BT_L2CAP_SDU_CHAN_SEND_RESERVE);

	net_buf_add_mem(buf, cp->data, data_len);
	err = bt_l2cap_chan_send(&br_chan->br.chan, buf);
	if (err < 0) {
		LOG_ERR("Unable to send data: %d", -err);
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#else
static uint8_t br_send_data(uint8_t chan_id, const struct btp_l2cap_send_data_cmd *cp,
			    void *rsp, uint16_t *rsp_len)
{
	return BTP_STATUS_FAILED;
}
#endif /* CONFIG_BT_CLASSIC */

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

	if (IS_ENABLED(CONFIG_BT_CLASSIC)) {
		uint8_t chan_id;

		chan_id = cp->chan_id;
		if (chan_id >= ARRAY_SIZE(channels)) {
			chan_id = chan_id - ARRAY_SIZE(channels);
			return br_send_data(chan_id, cp, rsp, rsp_len);
		}
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

static uint8_t get_server_index(struct bt_l2cap_server *server)
{
	ptrdiff_t index = 0;

	index = server - servers;

	return (uint8_t)index;
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

#if defined(CONFIG_BT_CLASSIC)
static int br_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
		     struct bt_l2cap_chan **l2cap_chan)
{
	struct br_channel *chan;
	__maybe_unused uint16_t options = 0;
	__maybe_unused uint8_t mode = BTP_L2CAP_LISTEN_V2_MODE_NONE;
	__maybe_unused uint8_t index;

	if (bt_conn_enc_key_size(conn) < req_keysize) {
		return -EPERM;
	}

	if (authorize_flag) {
		return -EACCES;
	}

	chan = get_free_br_channel();
	if (chan == NULL) {
		return -ENOMEM;
	}

	chan->br.chan.ops = &br_l2cap_ops;
	chan->br.rx.mtu = DATA_MTU_INITIAL;

#if defined(CONFIG_BT_L2CAP_RET_FC)
	index = get_server_index(server);
	if (index < ARRAY_SIZE(server_settings)) {
		options = server_settings[index].options;
		mode = server_settings[index].mode;
	}

	switch (mode) {
	case BTP_L2CAP_LISTEN_V2_MODE_RET:
		chan->br.rx.mode = BT_L2CAP_BR_LINK_MODE_RET;
		chan->br.rx.max_transmit = 3;
		break;
	case BTP_L2CAP_LISTEN_V2_MODE_FC:
		chan->br.rx.mode = BT_L2CAP_BR_LINK_MODE_FC;
		chan->br.rx.max_transmit = 3;
		break;
	case BTP_L2CAP_LISTEN_V2_MODE_ERET:
		chan->br.rx.mode = BT_L2CAP_BR_LINK_MODE_ERET;
		chan->br.rx.max_transmit = 3;
		break;
	case BTP_L2CAP_LISTEN_V2_MODE_STREAM:
		chan->br.rx.mode = BT_L2CAP_BR_LINK_MODE_STREAM;
		chan->br.rx.max_transmit = 0;
		break;
	default:
		chan->br.rx.mode = BT_L2CAP_BR_LINK_MODE_BASIC;
		chan->br.rx.max_transmit = 0;
		break;
	}

	if (options & BTP_L2CAP_LISTEN_V2_OPT_EXT_WIN_SIZE) {
		chan->br.rx.extended_control = true;
	} else {
		chan->br.rx.extended_control = false;
	}

	if (options & BTP_L2CAP_LISTEN_V2_OPT_MODE_OPTIONAL) {
		chan->br.rx.optional = true;
	} else {
		chan->br.rx.optional = false;
	}

	chan->br.rx.max_window = CONFIG_BT_L2CAP_MAX_WINDOW_SIZE;
	if (options & BTP_L2CAP_LISTEN_V2_OPT_NO_FCS) {
		chan->br.rx.fcs = BT_L2CAP_BR_FCS_NO;
	} else {
		chan->br.rx.fcs = BT_L2CAP_BR_FCS_16BIT;
	}

	chan->hold_credit = (options & BTP_L2CAP_LISTEN_V2_OPT_HOLD_CREDIT) != 0;
#endif /* CONFIG_BT_L2CAP_RET_FC */

	*l2cap_chan = &chan->br.chan;

	return 0;
}
#else
static int br_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
		     struct bt_l2cap_chan **l2cap_chan)
{
	return -ENOTSUP;
}
#endif /* CONFIG_BT_CLASSIC */

static uint8_t _listen(uint16_t psm, uint8_t transport, uint16_t response, uint8_t mode,
		       uint32_t options)
{
	struct bt_l2cap_server *server;

	if (psm == 0 || !is_free_psm(psm)) {
		return BTP_STATUS_FAILED;
	}

	if (mode > BTP_L2CAP_LISTEN_V2_MODE_VALID) {
		return BTP_STATUS_FAILED;
	}

	server = get_free_server();
	if (!server) {
		return BTP_STATUS_FAILED;
	}

	server->psm = psm;
	server_settings[get_server_index(server)].mode = mode;
	server_settings[get_server_index(server)].options = options;

	switch (response) {
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
	case BTP_L2CAP_CONNECTION_RESPONSE_INSUFF_SEC_AUTHEN:
		server->sec_level = BT_SECURITY_L4;
		break;
	default:
		goto failed;
	}

	if (transport == BTP_L2CAP_TRANSPORT_LE) {
		if (mode != BTP_L2CAP_LISTEN_V2_MODE_NONE) {
			goto failed;
		}

		server->accept = accept;
		if (bt_l2cap_server_register(server) < 0) {
			goto failed;
		}
	} else if (IS_ENABLED(CONFIG_BT_CLASSIC) && (transport == BTP_L2CAP_TRANSPORT_BREDR)) {
		server->accept = br_accept;
		if (bt_l2cap_br_server_register(server) < 0) {
			goto failed;
		}
	} else {
		goto failed;
	}

	return BTP_STATUS_SUCCESS;

failed:
	server_settings[get_server_index(server)].mode = 0;
	server_settings[get_server_index(server)].options = 0;
	server->psm = 0U;
	return BTP_STATUS_FAILED;
}

static uint8_t listen(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_l2cap_listen_cmd *cp = cmd;
	uint16_t psm = sys_le16_to_cpu(cp->psm);
	uint16_t response = sys_le16_to_cpu(cp->response);

	return _listen(psm, cp->transport, response, BTP_L2CAP_LISTEN_V2_MODE_NONE, 0);
}

static uint8_t listen_v2(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_l2cap_listen_v2_cmd *cp = cmd;
	uint16_t psm = sys_le16_to_cpu(cp->psm);
	uint16_t response = sys_le16_to_cpu(cp->response);
	uint32_t options = sys_le32_to_cpu(cp->options);

	return _listen(psm, cp->transport, response, cp->mode, options);
}

#if defined(CONFIG_BT_CLASSIC)
static uint8_t br_credits(uint8_t chan_id, void *rsp, uint16_t *rsp_len)
{
	struct br_channel *chan;

	if (chan_id >= CHANNELS) {
		return BTP_STATUS_FAILED;
	}

	chan = &br_channels[chan_id];

	if (!chan->in_use) {
		return BTP_STATUS_FAILED;
	}

	ARRAY_FOR_EACH(chan->pending_credit, i) {
		if (chan->pending_credit[i] != NULL) {
			if (bt_l2cap_chan_recv_complete(&chan->br.chan,
							chan->pending_credit[i]) < 0) {
				return BTP_STATUS_FAILED;
			}

			chan->pending_credit[i] = NULL;

			return BTP_STATUS_SUCCESS;
		}
	}

	return BTP_STATUS_SUCCESS;
}
#else
static uint8_t br_credits(uint8_t chan_id, void *rsp, uint16_t *rsp_len)
{
	return BTP_STATUS_FAILED;
}
#endif /* CONFIG_BT_CLASSIC */

static uint8_t credits(const void *cmd, uint16_t cmd_len,
		      void *rsp, uint16_t *rsp_len)
{
	const struct btp_l2cap_credits_cmd *cp = cmd;
	struct channel *chan;

	if (IS_ENABLED(CONFIG_BT_CLASSIC)) {
		uint8_t chan_id;

		chan_id = cp->chan_id;
		if (chan_id >= ARRAY_SIZE(channels)) {
			chan_id = chan_id - ARRAY_SIZE(channels);
			return br_credits(chan_id, rsp, rsp_len);
		}
	}

	if (cp->chan_id >= CHANNELS) {
		return BTP_STATUS_FAILED;
	}

	chan = &channels[cp->chan_id];

	if (!chan->in_use) {
		return BTP_STATUS_FAILED;
	}
#if defined(CONFIG_BT_L2CAP_SEG_RECV)
	if (chan->pending_credits) {
		if (bt_l2cap_chan_give_credits(&chan->le.chan, chan->pending_credits) < 0) {
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

#if defined(CONFIG_BT_CLASSIC)
static uint8_t send_echo_req(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_l2cap_send_echo_req_cmd *cp = cmd;
	struct bt_conn *conn;
	struct net_buf *buf;
	uint16_t data_len;
	int err;

	if (cp->address.type != BTP_BR_ADDRESS_TYPE) {
		LOG_ERR("Only support Classic");
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_br(&cp->address.a);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_length);

	if (data_len > DATA_MTU) {
		return BTP_STATUS_FAILED;
	}

	buf = net_buf_alloc(&data_pool, K_FOREVER);
	net_buf_reserve(buf, BT_L2CAP_BR_ECHO_REQ_RESERVE);
	net_buf_add_mem(buf, cp->data, data_len);

	err = bt_l2cap_br_echo_req(conn, buf);
	bt_conn_unref(conn);
	if (err != 0) {
		LOG_ERR("Unable to ECHO REQ: %d", -err);
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif /* CONFIG_BT_CLASSIC */

#if defined(CONFIG_BT_L2CAP_CONNLESS)
static uint8_t connless_send(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_l2cap_connless_send_cmd *cp = cmd;
	struct bt_conn *conn;
	struct net_buf *buf;
	uint16_t psm;
	uint16_t data_len;
	int err;

	if (cp->address.type != BTP_BR_ADDRESS_TYPE) {
		LOG_ERR("Only support Classic");
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_br(&cp->address.a);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	psm = sys_le16_to_cpu(cp->psm);
	data_len = sys_le16_to_cpu(cp->data_length);

	if (data_len > DATA_MTU) {
		LOG_ERR("Data length exceeds MAX buffer len (%u > %u)", data_len, DATA_MTU);
		return BTP_STATUS_FAILED;
	}

	buf = net_buf_alloc(&data_pool, K_FOREVER);
	net_buf_reserve(buf, BT_L2CAP_CONNLESS_RESERVE);
	net_buf_add_mem(buf, cp->data, data_len);

	err = bt_l2cap_br_connless_send(conn, psm, buf);
	bt_conn_unref(conn);
	if (err < 0) {
		LOG_ERR("Unable to send CLS data: %d", -err);
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif /* CONFIG_BT_L2CAP_CONNLESS */

static uint8_t supported_commands(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	struct btp_l2cap_read_supported_commands_rp *rp = rsp;

	*rsp_len = tester_supported_commands(BTP_SERVICE_ID_L2CAP, rp->data);
	*rsp_len += sizeof(*rp);

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
		.opcode = BTP_L2CAP_CONNECT_V2,
		.expect_len = sizeof(struct btp_l2cap_connect_v2_cmd),
		.func = connect_v2,
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
		.opcode = BTP_L2CAP_LISTEN_V2,
		.expect_len = sizeof(struct btp_l2cap_listen_v2_cmd),
		.func = listen_v2,
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
#if defined(CONFIG_BT_EATT)
	{
		.opcode = BTP_L2CAP_DISCONNECT_EATT_CHANS,
		.expect_len = sizeof(struct btp_l2cap_disconnect_eatt_chans_cmd),
		.func = disconnect_eatt_chans,
	},
#endif
#if defined(CONFIG_BT_CLASSIC)
	{
		.opcode = BTP_L2CAP_SEND_ECHO_REQ,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = send_echo_req,
	},
#endif /* CONFIG_BT_CLASSIC */
#if defined(CONFIG_BT_L2CAP_CONNLESS)
	{
		.opcode = BTP_L2CAP_CONNLESS_SEND,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = connless_send,
	},
#endif /* CONFIG_BT_L2CAP_CONNLESS*/
};

#if defined(CONFIG_BT_CLASSIC)
static void l2cap_br_echo_req(struct bt_conn *conn, uint8_t identifier, struct net_buf *buf)
{
	struct net_buf *tx_buf;
	int err;

	tx_buf = net_buf_alloc(&data_pool, K_FOREVER);
	net_buf_reserve(tx_buf, BT_L2CAP_BR_ECHO_RSP_RESERVE);
	net_buf_add_mem(tx_buf, buf->data, buf->len);

	err = bt_l2cap_br_echo_rsp(conn, identifier, tx_buf);
	if (err != 0) {
		LOG_ERR("Unable to ECHO REQ: %d", -err);
		net_buf_unref(tx_buf);
	}
}

static void l2cap_br_echo_rsp(struct bt_conn *conn, struct net_buf *buf)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(buf);
}

static struct bt_l2cap_br_echo_cb echo_cb = {
	.req = l2cap_br_echo_req,
	.rsp = l2cap_br_echo_rsp,
};

static int l2cap_br_echo_reg(void)
{
	return bt_l2cap_br_echo_cb_register(&echo_cb);
}

static int l2cap_br_echo_unreg(void)
{
	return bt_l2cap_br_echo_cb_unregister(&echo_cb);
}
#else
static int l2cap_br_echo_reg(void)
{
	return -ENOTSUP;
}

static int l2cap_br_echo_unreg(void)
{
	return -ENOTSUP;
}
#endif /* CONFIG_BT_CLASSIC */

uint8_t tester_init_l2cap(void)
{
	tester_register_command_handlers(BTP_SERVICE_ID_L2CAP, handlers,
					 ARRAY_SIZE(handlers));

	if (IS_ENABLED(CONFIG_BT_CLASSIC)) {
		int err = l2cap_br_echo_reg();

		if (err < 0) {
			return BTP_STATUS_FAILED;
		}
	}
	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_l2cap(void)
{
	if (IS_ENABLED(CONFIG_BT_CLASSIC)) {
		int err = l2cap_br_echo_unreg();

		if (err < 0) {
			return BTP_STATUS_FAILED;
		}
	}

	return BTP_STATUS_SUCCESS;
}
