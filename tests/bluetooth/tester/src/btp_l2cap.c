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

#define DATA_MTU_INITIAL 128
#define DATA_MTU 256
#define DATA_BUF_SIZE BT_L2CAP_SDU_BUF_SIZE(DATA_MTU)
#define CHANNELS 2
#define SERVERS 1

NET_BUF_POOL_FIXED_DEFINE(data_pool, CHANNELS, DATA_BUF_SIZE, 8, NULL);

static bool authorize_flag;
static uint8_t req_keysize;

static struct channel {
	uint8_t chan_id; /* Internal number that identifies L2CAP channel. */
	struct bt_l2cap_le_chan le;
	bool in_use;
	bool hold_credit;
	struct net_buf *pending_credit;
} channels[CHANNELS];

#if defined(CONFIG_BT_CLASSIC)
static struct br_channel {
	uint8_t chan_id; /* Internal number that identifies L2CAP channel. */
	struct bt_l2cap_br_chan br;
	bool in_use;
	bool hold_credit;
	struct net_buf *pending_credit[CHANNELS];
} br_channels[CHANNELS];

struct bt_l2cap_br_server {
	struct bt_l2cap_server server;
	uint8_t options;
} br_servers[SERVERS];
#endif /* defined(CONFIG_BT_CLASSIC) */

/* TODO Extend to support multiple servers */
static struct bt_l2cap_server servers[SERVERS];

static struct net_buf *alloc_buf_cb(struct bt_l2cap_chan *chan)
{
	return net_buf_alloc(&data_pool, K_FOREVER);
}

static uint8_t recv_cb_buf[DATA_BUF_SIZE + sizeof(struct btp_l2cap_data_received_ev)];

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

	/* release netbuf on premature disconnection */
	if (chan->pending_credit) {
		net_buf_unref(chan->pending_credit);
		chan->pending_credit = NULL;
	}

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
	.alloc_buf	= alloc_buf_cb,
	.recv		= recv_cb,
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

#if defined(CONFIG_BT_CLASSIC)

static struct net_buf *alloc_buf_br_cb(struct bt_l2cap_chan *chan)
{
	return net_buf_alloc(&data_pool, K_NO_WAIT);
}

static struct br_channel *get_free_br_channel(void)
{
	uint8_t i;
	struct br_channel *chan = NULL;

	for (i = 0U; i < CHANNELS; i++) {
		if (br_channels[i].in_use) {
			continue;
		}

		chan = &br_channels[i];

		(void)memset(chan, 0, sizeof(*chan));
		chan->chan_id = i;

		br_channels[i].in_use = true;

		break;
	}

	return chan;
}

extern struct bt_conn *bt_conn_lookup_addr_br(const bt_addr_t *peer);

static int recv_br_cb(struct bt_l2cap_chan *l2cap_chan, struct net_buf *buf)
{
	struct btp_l2cap_data_received_ev *ev = (void *) recv_cb_buf;
	struct bt_l2cap_br_chan *l2cap_br_chan = CONTAINER_OF(
			l2cap_chan, struct bt_l2cap_br_chan, chan);
	struct br_channel *br_chan = CONTAINER_OF(l2cap_br_chan, struct br_channel, br);
	struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);

	ev->chan_id = br_chan->chan_id;
	ev->data_length = sys_cpu_to_le16(buf->len);
	memcpy(ev->data, buf->data, buf->len);

	tester_event(BTP_SERVICE_ID_L2CAP, BTP_L2CAP_EV_DATA_RECEIVED,
		     recv_cb_buf, sizeof(*ev) + buf->len);

	if (br_chan->hold_credit && (pool == &data_pool) &&
		(net_buf_id(buf) < (int)ARRAY_SIZE(br_chan->pending_credit)) &&
		!br_chan->pending_credit[net_buf_id(buf)]) {
		/* no need for extra ref, as when returning EINPROGRESS user
		 * becomes owner of the netbuf
		 */
		br_chan->pending_credit[net_buf_id(buf)] = buf;
		return -EINPROGRESS;
	}

	return 0;
}

static void connected_br_cb(struct bt_l2cap_chan *l2cap_chan)
{
	struct btp_l2cap_connected_ev ev;
	struct bt_conn_info info;
	struct bt_l2cap_br_chan *l2cap_br_chan = CONTAINER_OF(
			l2cap_chan, struct bt_l2cap_br_chan, chan);
	struct br_channel *br_chan = CONTAINER_OF(l2cap_br_chan, struct br_channel, br);

	/* TODO: ev.psm */
	if (!bt_conn_get_info(l2cap_chan->conn, &info)) {
		switch (info.type) {
		case BT_CONN_TYPE_BR:
			ev.chan_id = br_chan->chan_id;
			ev.mtu_remote = sys_cpu_to_le16(br_chan->br.tx.mtu);
			ev.mps_remote = 0;
			ev.mtu_local = sys_cpu_to_le16(br_chan->br.rx.mtu);
			ev.mps_local = 0;
			ev.address.type = BT_ADDR_LE_PUBLIC;
			bt_addr_copy(&ev.address.a, info.br.dst);
			break;
		default:
			/* TODO figure out how (if) want to handle BR/EDR */
			return;
		}
	}

	tester_event(BTP_SERVICE_ID_L2CAP, BTP_L2CAP_EV_CONNECTED, &ev, sizeof(ev));
}

static void disconnected_br_cb(struct bt_l2cap_chan *l2cap_chan)
{
	struct btp_l2cap_disconnected_ev ev;
	struct bt_conn_info info;
	struct bt_l2cap_br_chan *l2cap_br_chan = CONTAINER_OF(
			l2cap_chan, struct bt_l2cap_br_chan, chan);
	struct br_channel *br_chan = CONTAINER_OF(l2cap_br_chan, struct br_channel, br);

	/* release netbuf on premature disconnection */
	for (size_t i = 0; i < ARRAY_SIZE(br_chan->pending_credit); i++) {
		if (br_chan->pending_credit[i]) {
			net_buf_unref(br_chan->pending_credit[i]);
		}
		br_chan->pending_credit[i] = NULL;
	}

	(void)memset(&ev, 0, sizeof(struct btp_l2cap_disconnected_ev));

	/* TODO: ev.psm */
	if (!bt_conn_get_info(l2cap_chan->conn, &info)) {
		switch (info.type) {
		case BT_CONN_TYPE_BR:
			ev.chan_id = br_chan->chan_id;
			br_chan->in_use = false;
			ev.address.type = BT_ADDR_LE_PUBLIC;
			bt_addr_copy(&ev.address.a, info.br.dst);
			break;
		default:
			/* TODO figure out how (if) want to handle BR/EDR */
			return;
		}
	}

	tester_event(BTP_SERVICE_ID_L2CAP, BTP_L2CAP_EV_DISCONNECTED, &ev, sizeof(ev));
}

#if defined(CONFIG_BT_L2CAP_ECRED)
static void reconfigured_br_cb(struct bt_l2cap_chan *l2cap_chan)
{
	struct btp_l2cap_reconfigured_ev ev;
	struct bt_l2cap_br_chan *l2cap_br_chan = CONTAINER_OF(
			l2cap_chan, struct bt_l2cap_br_chan, chan);
	struct br_channel *br_chan = CONTAINER_OF(l2cap_br_chan, struct br_channel, br);

	(void)memset(&ev, 0, sizeof(ev));

	ev.chan_id = br_chan->chan_id;
	ev.mtu_remote = sys_cpu_to_le16(br_chan->br.tx.mtu);
	ev.mps_remote = sys_cpu_to_le16(br_chan->br.tx.mtu);
	ev.mtu_local = sys_cpu_to_le16(br_chan->br.rx.mtu);
	ev.mps_local = sys_cpu_to_le16(br_chan->br.rx.mtu);

	tester_event(BTP_SERVICE_ID_L2CAP, BTP_L2CAP_EV_RECONFIGURED, &ev, sizeof(ev));
}
#endif

static const struct bt_l2cap_chan_ops l2cap_br_ops = {
	.alloc_buf	= alloc_buf_br_cb,
	.recv		= recv_br_cb,
	.connected	= connected_br_cb,
	.disconnected	= disconnected_br_cb,
#if defined(CONFIG_BT_L2CAP_ECRED)
	.reconfigured	= reconfigured_br_cb,
#endif
};

#endif /* defined(CONFIG_BT_CLASSIC) */

static uint8_t _connect(const void *cmd, uint16_t cmd_len, void *rsp,
		       uint16_t *rsp_len, bool sec, bt_security_t sec_level)
{
	const struct btp_l2cap_connect_cmd *cp = cmd;
	struct btp_l2cap_connect_rp *rp = rsp;
	struct bt_conn *conn;
	struct channel *chan = NULL;
#if defined(CONFIG_BT_CLASSIC)
	struct br_channel *br_chan = NULL;
#endif /* defined(CONFIG_BT_CLASSIC) */
	struct bt_l2cap_chan *allocated_channels[5] = {};
	uint16_t mtu = sys_le16_to_cpu(cp->mtu);
	uint16_t psm = sys_le16_to_cpu(cp->psm);
	uint8_t i = 0;
	bool ecfc = cp->options & BTP_L2CAP_CONNECT_OPT_ECFC;
	int err = -EINVAL;
#if defined(CONFIG_BT_CLASSIC)
	bool bredr = false;
#endif /* defined(CONFIG_BT_CLASSIC) */

	if (cp->num == 0 || cp->num > CHANNELS || mtu > DATA_MTU_INITIAL) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
#if defined(CONFIG_BT_CLASSIC)
	if ((conn == NULL) && (cp->address.type == BT_ADDR_LE_PUBLIC)) {
		conn = bt_conn_lookup_addr_br(&cp->address.a);
		bredr = true;
	}
#endif /* defined(CONFIG_BT_CLASSIC) */
	if (!conn) {
		return BTP_STATUS_FAILED;
	}

	bt_conn_unref(conn);

	for (i = 0U; i < cp->num; i++) {
#if defined(CONFIG_BT_CLASSIC)
		if (bredr) {
			br_chan = get_free_br_channel();
			if (!br_chan) {
				goto fail;
			}
			br_chan->br.chan.ops = &l2cap_br_ops;
			br_chan->br.rx.mtu = mtu;
			if (sec) {
				br_chan->br.required_sec_level = sec_level;
			}

			if (cp->options & BTP_L2CAP_CONNECT_OPT_RET) {
#if defined(CONFIG_BT_L2CAP_RET)
				br_chan->br.rx.mode = BT_L2CAP_BR_LINK_MODE_RET;
				br_chan->br.rx.transmit = 3;
				br_chan->br.rx.window = 3;
#endif /* CONFIG_BT_L2CAP_RET */
			} else if (cp->options & BTP_L2CAP_CONNECT_OPT_FC) {
#if defined(CONFIG_BT_L2CAP_FC)
				br_chan->br.rx.mode = BT_L2CAP_BR_LINK_MODE_FC;
				br_chan->br.rx.transmit = 3;
				br_chan->br.rx.window = 2;
#endif /* CONFIG_BT_L2CAP_FC */
			} else if (cp->options & BTP_L2CAP_CONNECT_OPT_ERET) {
#if defined(CONFIG_BT_L2CAP_ENH_RET)
				br_chan->br.rx.mode = BT_L2CAP_BR_LINK_MODE_ERET;
				br_chan->br.rx.transmit = 3;
				br_chan->br.rx.window = 3;
#endif /* CONFIG_BT_L2CAP_ENH_RET */
			} else if (cp->options & BTP_L2CAP_CONNECT_OPT_STREAM) {
#if defined(CONFIG_BT_L2CAP_STREAM)
				br_chan->br.rx.mode = BT_L2CAP_BR_LINK_MODE_STREAM;
				br_chan->br.rx.transmit = 1;
				br_chan->br.rx.window = 3;
#endif /* CONFIG_BT_L2CAP_STREAM */
			}

			rp->chan_id[i] = br_chan->chan_id;
			allocated_channels[i] = &br_chan->br.chan;
			br_chan->hold_credit = cp->options & BTP_L2CAP_CONNECT_OPT_HOLD_CREDIT;
			continue;
		}
#endif /* defined(CONFIG_BT_CLASSIC) */
		chan = get_free_channel();
		if (!chan) {
			goto fail;
		}
		chan->le.chan.ops = &l2cap_ops;
		chan->le.rx.mtu = mtu;
#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
		if (sec) {
			chan->le.required_sec_level = sec_level;
		}
#endif /* defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL) */
		rp->chan_id[i] = chan->chan_id;
		allocated_channels[i] = &chan->le.chan;

		chan->hold_credit = cp->options & BTP_L2CAP_CONNECT_OPT_HOLD_CREDIT;
	}

	if (cp->num == 1 && !ecfc) {
		struct bt_l2cap_chan *l2ca_chan;
#if defined(CONFIG_BT_CLASSIC)
		if (bredr) {
			l2ca_chan = &br_chan->br.chan;
		} else {
#endif /* defined(CONFIG_BT_CLASSIC) */
			l2ca_chan = &chan->le.chan;
#if defined(CONFIG_BT_CLASSIC)
		}
#endif /* defined(CONFIG_BT_CLASSIC) */
		err = bt_l2cap_chan_connect(conn, l2ca_chan, psm);
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
#if defined(CONFIG_BT_CLASSIC)
			if (bredr) {
				CONTAINER_OF(CONTAINER_OF(allocated_channels[i],
							  struct bt_l2cap_br_chan, chan),
					     struct br_channel, br)
					->in_use = false;
				continue;
			}
#endif /* defined(CONFIG_BT_CLASSIC) */
			channels[BT_L2CAP_LE_CHAN(allocated_channels[i])->ident].in_use = false;
		}
	}
#if defined(CONFIG_BT_CLASSIC)
	if (err == -ENOTSUP) {
		return BTP_STATUS_NOT_SUPPORT;
	}
#endif /* defined(CONFIG_BT_CLASSIC) */
	return BTP_STATUS_FAILED;
}

static uint8_t connect(const void *cmd, uint16_t cmd_len,
		       void *rsp, uint16_t *rsp_len)
{
	return _connect(cmd, cmd_len, rsp, rsp_len, false,
					(bt_security_t)BTP_L2CAP_CONNECT_SEC_LEVEL_0);
}

static int transform_sec_level(uint8_t sec_level, bt_security_t *level)
{
	int err = 0;

	switch (sec_level) {
	case BTP_L2CAP_CONNECT_SEC_LEVEL_0:
		*level = BT_SECURITY_L0;
		break;
	case BTP_L2CAP_CONNECT_SEC_LEVEL_1:
		*level = BT_SECURITY_L1;
		break;
	case BTP_L2CAP_CONNECT_SEC_LEVEL_2:
		*level = BT_SECURITY_L2;
		break;
	case BTP_L2CAP_CONNECT_SEC_LEVEL_3:
		*level = BT_SECURITY_L3;
		break;
	case BTP_L2CAP_CONNECT_SEC_LEVEL_4:
		*level = BT_SECURITY_L4;
		break;
	default:
		err = -EINVAL;
		break;
	}

	return err;
}

static uint8_t connect_with_sec_level(const void *cmd, uint16_t cmd_len,
		       void *rsp, uint16_t *rsp_len)
{
	const struct btp_l2cap_connect_with_sec_level_cmd *cp = cmd;
	int err;
	bt_security_t level;

	err = transform_sec_level(cp->sec_level, &level);
	if (err < 0) {
		LOG_ERR("Unsupported sec level %d", cp->sec_level);
		return BTP_STATUS_FAILED;
	}

	return _connect(&cp->cmd, cmd_len - sizeof(cp->sec_level), rsp, rsp_len, true,
					level);
}

#if defined(CONFIG_BT_CLASSIC)
static uint8_t echo(const void *cmd, uint16_t cmd_len,
		       void *rsp, uint16_t *rsp_len)
{
	const struct btp_l2cap_echo_cmd *cp = cmd;
	struct bt_conn *conn = NULL;
	struct net_buf *buf;
	int err;

	if (cp->address.type == BT_ADDR_LE_PUBLIC) {
		conn = bt_conn_lookup_addr_br(&cp->address.a);
	}

	if (!conn) {
		return BTP_STATUS_FAILED;
	}

	bt_conn_unref(conn);

	buf = net_buf_alloc(&data_pool, K_MSEC(0));
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	net_buf_reserve(buf, BT_L2CAP_ECHO_RESERVE);
	if (net_buf_tailroom(buf) < cp->data_len) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	net_buf_add_mem(buf, cp->data, cp->data_len);

	err = bt_l2cap_br_echo(conn, buf);
	if (err) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static struct bt_l2cap_br_server *get_free_br_server(void)
{
	uint8_t i;

	for (i = 0U; i < SERVERS ; i++) {
		if (br_servers[i].server.psm) {
			continue;
		}

		return &br_servers[i];
	}

	return NULL;
}

static bool is_free_br_psm(uint16_t psm)
{
	uint8_t i;

	for (i = 0U; i < ARRAY_SIZE(br_servers); i++) {
		if (br_servers[i].server.psm == psm) {
			return false;
		}
	}

	return true;
}
#endif /* defined(CONFIG_BT_CLASSIC) */

#if defined(CONFIG_BT_CLASSIC) && defined(CONFIG_BT_L2CAP_CLS)
static struct bt_l2cap_cls cls;

static void cls_recv(struct bt_conn *conn, struct net_buf *buf)
{
}

static uint8_t cls_listen(const void *cmd, uint16_t cmd_len,
		       void *rsp, uint16_t *rsp_len)
{
	const struct btp_l2cap_cls_listen_cmd *cp = cmd;
	int err;

	cls.psm = cp->psm;
	cls.recv = cls_recv;
	cls.sec_level = BT_SECURITY_L1;

	err = bt_l2cap_cls_register(&cls);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif /* defined(CONFIG_BT_CLASSIC) && defined(CONFIG_BT_L2CAP_CLS) */

#if defined(CONFIG_BT_CLASSIC) && defined(CONFIG_BT_L2CAP_CLS)
static uint8_t cls_send(const void *cmd, uint16_t cmd_len,
		       void *rsp, uint16_t *rsp_len)
{
	const struct btp_l2cap_cls_send_cmd *cp = cmd;
	struct net_buf *buf;
	struct bt_conn *conn = NULL;
	int err;

	if (cp->address.type == BT_ADDR_LE_PUBLIC) {
		conn = bt_conn_lookup_addr_br(&cp->address.a);
	}

	if (!conn) {
		return BTP_STATUS_FAILED;
	}

	bt_conn_unref(conn);

	buf = net_buf_alloc(&data_pool, K_MSEC(0));
	if (!buf) {
		return BTP_STATUS_FAILED;
	}

	net_buf_reserve(buf, BT_L2CAP_CLS_SEND_RESERVE);

	if (net_buf_tailroom(buf) < cp->data_len) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	net_buf_add_mem(buf, cp->data, cp->data_len);

	err = bt_l2cap_cls_send(conn, cp->psm, buf);
	if (err) {
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif /* defined(CONFIG_BT_CLASSIC) && defined(CONFIG_BT_L2CAP_CLS) */

static struct bt_l2cap_chan *get_l2cap_chan_from_chan_id(uint8_t chan_id)
{
	struct channel *le_chan;
#if defined(CONFIG_BT_CLASSIC)
	struct br_channel *br_chan;
#endif /* defined(CONFIG_BT_CLASSIC) */

	if (chan_id < CHANNELS) {
		le_chan = &channels[chan_id];
		if (le_chan->in_use) {
			return &le_chan->le.chan;
		}

#if defined(CONFIG_BT_CLASSIC)
		br_chan = &br_channels[chan_id];
		if (br_chan->in_use) {
			return &br_chan->br.chan;
		}
#endif /* defined(CONFIG_BT_CLASSIC) */
	}

	return NULL;
}

static uint16_t get_l2cap_mtu_from_chan_id(uint8_t chan_id)
{
	struct channel *le_chan;
#if defined(CONFIG_BT_CLASSIC)
	struct br_channel *br_chan;
#endif /* defined(CONFIG_BT_CLASSIC) */

	if (chan_id < CHANNELS) {
		le_chan = &channels[chan_id];
		if (le_chan->in_use) {
			return le_chan->le.tx.mtu;
		}

#if defined(CONFIG_BT_CLASSIC)
		br_chan = &br_channels[chan_id];
		if (br_chan->in_use) {
			return br_chan->br.tx.mtu;
		}
#endif /* defined(CONFIG_BT_CLASSIC) */
	}

	return 0;
}

static uint8_t disconnect(const void *cmd, uint16_t cmd_len,
			  void *rsp, uint16_t *rsp_len)
{
	const struct btp_l2cap_disconnect_cmd *cp = cmd;
	struct bt_l2cap_chan *chan;
	int err;

	chan = get_l2cap_chan_from_chan_id(cp->chan_id);
	if (chan == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_l2cap_chan_disconnect(chan);
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
	struct bt_l2cap_chan *chan;
	uint16_t mtu;
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

	chan = get_l2cap_chan_from_chan_id(cp->chan_id);
	if (chan == NULL) {
		return BTP_STATUS_FAILED;
	}

	mtu = get_l2cap_mtu_from_chan_id(cp->chan_id);
	if (mtu == 0) {
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	/* FIXME: For now, fail if data length exceeds buffer length */
	if (data_len > DATA_MTU) {
		return BTP_STATUS_FAILED;
	}

	/* FIXME: For now, fail if data length exceeds remote's L2CAP SDU */
	if (data_len > mtu) {
		return BTP_STATUS_FAILED;
	}

	buf = net_buf_alloc(&data_pool, K_FOREVER);
	net_buf_reserve(buf, BT_L2CAP_SDU_CHAN_SEND_RESERVE);

	net_buf_add_mem(buf, cp->data, data_len);
	ret = bt_l2cap_chan_send(chan, buf);
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

	*l2cap_chan = &chan->le.chan;

	return 0;
}

#if defined(CONFIG_BT_CLASSIC)
static int accept_br(struct bt_conn *conn, struct bt_l2cap_server *server,
		  struct bt_l2cap_chan **l2cap_chan)
{
	struct br_channel *chan;
	struct bt_l2cap_br_server *br_server;

	br_server = CONTAINER_OF(server, struct bt_l2cap_br_server, server);

	if (bt_conn_enc_key_size(conn) < req_keysize) {
		return -EPERM;
	}

	if (authorize_flag) {
		return -EACCES;
	}

	chan = get_free_br_channel();
	if (!chan) {
		return -ENOMEM;
	}

	chan->br.chan.ops = &l2cap_br_ops;
	chan->br.rx.mtu = DATA_MTU_INITIAL;
	chan->hold_credit = br_server->options & BTP_L2CAP_LISTEN_OPT_HOLD_CREDIT;
#if defined(CONFIG_BT_L2CAP_RET) || defined(CONFIG_BT_L2CAP_FC) || \
	defined(CONFIG_BT_L2CAP_ENH_RET) || defined(CONFIG_BT_L2CAP_STREAM)
	chan->br.rx.optional = br_server->options & BTP_L2CAP_LISTEN_OPT_MODE_OPTIONAL;
#endif

	if (br_server->options & BTP_L2CAP_LISTEN_OPT_RET) {
#if defined(CONFIG_BT_L2CAP_RET)
		chan->br.rx.mode = BT_L2CAP_BR_LINK_MODE_RET;
		chan->br.rx.window = 3;
		chan->br.rx.transmit = 3;
#endif /* CONFIG_BT_L2CAP_RET */
	} else if (br_server->options & BTP_L2CAP_LISTEN_OPT_FC) {
#if defined(CONFIG_BT_L2CAP_FC)
		chan->br.rx.mode = BT_L2CAP_BR_LINK_MODE_FC;
		chan->br.rx.window = 3;
		chan->br.rx.transmit = 3;
#endif /* CONFIG_BT_L2CAP_FC */
	} else if (br_server->options & BTP_L2CAP_LISTEN_OPT_ERET) {
#if defined(CONFIG_BT_L2CAP_ENH_RET)
		chan->br.rx.mode = BT_L2CAP_BR_LINK_MODE_ERET;
		chan->br.rx.window = 3;
		chan->br.rx.transmit = 3;
#endif /* CONFIG_BT_L2CAP_ENH_RET */
	} else if (br_server->options & BTP_L2CAP_LISTEN_OPT_STREAM) {
#if defined(CONFIG_BT_L2CAP_STREAM)
		chan->br.rx.mode = BT_L2CAP_BR_LINK_MODE_STREAM;
		chan->br.rx.window = 3;
		chan->br.rx.transmit = 1;
#endif /* CONFIG_BT_L2CAP_STREAM */
	}

	*l2cap_chan = &chan->br.chan;

	return 0;
}
#endif /* CONFIG_BT_CLASSIC */

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

	if (cp->transport == BTP_L2CAP_TRANSPORT_BREDR) {
#if defined(CONFIG_BT_CLASSIC)
		server->accept = accept_br;
		if (bt_l2cap_br_server_register(server) < 0) {
			server->psm = 0U;
			LOG_ERR("Fail to register the BR/EDR server");
			return BTP_STATUS_FAILED;
		}
#else
		LOG_WRN("BR/EDR not supported");
		return BTP_STATUS_FAILED;
#endif /* defined(CONFIG_BT_CLASSIC) */
	} else {
		server->accept = accept;
		if (bt_l2cap_server_register(server) < 0) {
			server->psm = 0U;
			return BTP_STATUS_FAILED;
		}
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t credits(const void *cmd, uint16_t cmd_len,
		      void *rsp, uint16_t *rsp_len)
{
	const struct btp_l2cap_credits_cmd *cp = cmd;
	struct channel *chan;
#if defined(CONFIG_BT_CLASSIC)
	struct br_channel *br_chan = NULL;
#endif /* defined(CONFIG_BT_CLASSIC) */

	if (cp->chan_id >= CHANNELS) {
		return BTP_STATUS_FAILED;
	}

	chan = &channels[cp->chan_id];

	if (!chan->in_use) {
#if defined(CONFIG_BT_CLASSIC)
		br_chan = &br_channels[cp->chan_id];
		if (!br_chan->in_use) {
			return BTP_STATUS_FAILED;
		}
#else
		return BTP_STATUS_FAILED;
#endif /* defined(CONFIG_BT_CLASSIC) */
	}

#if defined(CONFIG_BT_CLASSIC)
	if (br_chan) {
		for (size_t i = 0; i < ARRAY_SIZE(br_chan->pending_credit); i++) {
			if (br_chan->pending_credit[i]) {
				bt_l2cap_chan_recv_complete(&br_chan->br.chan,
					br_chan->pending_credit[i]);
			}
			br_chan->pending_credit[i] = NULL;
		}
		return BTP_STATUS_SUCCESS;
	}
#endif /* defined(CONFIG_BT_CLASSIC) */

	if (chan->pending_credit) {
		if (bt_l2cap_chan_recv_complete(&chan->le.chan,
						chan->pending_credit) < 0) {
			return BTP_STATUS_FAILED;
		}

		chan->pending_credit = NULL;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t listen_with_mode(const void *cmd, uint16_t cmd_len,
		      void *rsp, uint16_t *rsp_len)
{
#ifndef CONFIG_BT_CLASSIC
	LOG_WRN("BR/EDR not supported");
	return BTP_STATUS_FAILED;
#else
	const struct btp_l2cap_listen_with_mode_cmd *cp = cmd;
	struct bt_l2cap_br_server *server;
	uint16_t psm = sys_le16_to_cpu(cp->psm);

	if (cp->transport != BTP_L2CAP_TRANSPORT_BREDR) {
		return BTP_STATUS_FAILED;
	}

	if (psm == 0 || !is_free_br_psm(psm)) {
		return BTP_STATUS_FAILED;
	}

	server = get_free_br_server();
	if (!server) {
		return BTP_STATUS_FAILED;
	}

	server->server.psm = psm;

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
		server->server.sec_level = BT_SECURITY_L3;
		break;
	case BTP_L2CAP_CONNECTION_RESPONSE_INSUFF_ENCRYPTION:
		server->server.sec_level = BT_SECURITY_L2;
		break;
	default:
		return BTP_STATUS_FAILED;
	}

	server->server.accept = accept_br;
	server->options = cp->options;

	if (bt_l2cap_br_server_register(&server->server) < 0) {
		server->server.psm = 0U;
		LOG_ERR("Fail to register the BR/EDR server");
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
#endif /* defined(CONFIG_BT_CLASSIC) */
}

static uint8_t set_opt(const void *cmd, uint16_t cmd_len,
		      void *rsp, uint16_t *rsp_len)
{
	const struct btp_l2cap_set_option_cmd *cp = cmd;
	struct channel *chan;
#if defined(CONFIG_BT_CLASSIC)
	struct br_channel *br_chan = NULL;
#endif /* defined(CONFIG_BT_CLASSIC) */

	if (cp->chan_id >= CHANNELS) {
		return BTP_STATUS_FAILED;
	}

	chan = &channels[cp->chan_id];

	if (!chan->in_use) {
#if defined(CONFIG_BT_CLASSIC)
		br_chan = &br_channels[cp->chan_id];
		if (!br_chan->in_use) {
			return BTP_STATUS_FAILED;
		}
#else
		return BTP_STATUS_FAILED;
#endif /* defined(CONFIG_BT_CLASSIC) */
	}

#if defined(CONFIG_BT_CLASSIC)
	if (br_chan) {
		if (cp->options & BTP_L2CAP_SET_OPT_HOLD_CREDIT) {
			br_chan->hold_credit = true;
		}
		return BTP_STATUS_SUCCESS;
	}
#endif /* defined(CONFIG_BT_CLASSIC) */

	if (cp->options & BTP_L2CAP_SET_OPT_HOLD_CREDIT) {
		chan->hold_credit = true;
	}
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
#if defined(CONFIG_BT_CLASSIC)
	tester_set_bit(rp->data, BTP_L2CAP_ECHO);
#endif /* defined(CONFIG_BT_CLASSIC) */
#if defined(CONFIG_BT_CLASSIC) && defined(CONFIG_BT_L2CAP_CLS)
	tester_set_bit(rp->data, BTP_L2CAP_CLS_LISTEN);
#endif /* defined(CONFIG_BT_CLASSIC) && defined(CONFIG_BT_L2CAP_CLS) */
#if defined(CONFIG_BT_CLASSIC) && defined(CONFIG_BT_L2CAP_CLS)
	tester_set_bit(rp->data, BTP_L2CAP_CLS_SEND);
#endif /* defined(CONFIG_BT_CLASSIC) && defined(CONFIG_BT_L2CAP_CLS) */

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
	{
		.opcode = BTP_L2CAP_CONNECT_WITH_SEC_LEVEL,
		.expect_len = sizeof(struct btp_l2cap_connect_with_sec_level_cmd),
		.func = connect_with_sec_level,
	},
#if defined(CONFIG_BT_CLASSIC)
	{
		.opcode = BTP_L2CAP_ECHO,
		.expect_len = sizeof(struct btp_l2cap_echo_cmd),
		.func = echo,
	},
#endif /* defined(CONFIG_BT_CLASSIC) */
#if defined(CONFIG_BT_CLASSIC) && defined(CONFIG_BT_L2CAP_CLS)
	{
		.opcode = BTP_L2CAP_CLS_LISTEN,
		.expect_len = sizeof(struct btp_l2cap_cls_listen_cmd),
		.func = cls_listen,
	},
#endif /* defined(CONFIG_BT_CLASSIC) && defined(CONFIG_BT_L2CAP_CLS) */
#if defined(CONFIG_BT_CLASSIC) && defined(CONFIG_BT_L2CAP_CLS)
	{
		.opcode = BTP_L2CAP_CLS_SEND,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = cls_send,
	},
#endif /* defined(CONFIG_BT_CLASSIC) && defined(CONFIG_BT_L2CAP_CLS) */
	{
		.opcode = BTP_L2CAP_LISTEN_WITH_MODE,
		.expect_len = sizeof(struct btp_l2cap_listen_with_mode_cmd),
		.func = listen_with_mode,
	},
	{
		.opcode = BTP_L2CAP_SET_OPT,
		.expect_len = sizeof(struct btp_l2cap_set_option_cmd),
		.func = set_opt,
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
