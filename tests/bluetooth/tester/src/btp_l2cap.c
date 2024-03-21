/* l2cap.c - Bluetooth L2CAP Tester */

/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>

#include <errno.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/sys/byteorder.h>
#include "../../../../subsys/bluetooth/host/conn_internal.h"
#include "../../../../subsys/bluetooth/host/smp.h"

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME bttester_l2cap
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

#include "btp/btp.h"

#define DATA_MTU_INITIAL 128
#define DATA_MTU 256
#define DATA_BUF_SIZE BT_L2CAP_SDU_BUF_SIZE(DATA_MTU)
#define CHANNELS 2
#define SERVERS  2

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

static struct br_channel {
	uint8_t chan_id; /* Internal number that identifies L2CAP channel. */
	struct bt_l2cap_br_chan br;
	bool in_use;
	bool hold_credit;
	struct net_buf *pending_credit;
} br_channels[CHANNELS];

/* TODO Extend to support multiple servers */
static struct bt_l2cap_server servers[SERVERS];

#if defined(CONFIG_BT_CLASSIC)
extern struct bt_conn *bt_conn_lookup_addr_br(const bt_addr_t *peer);
#endif

#if defined(CONFIG_BT_CLASSIC)
static struct bt_l2cap_server *get_server(uint16_t psm);
#endif

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
#if defined(CONFIG_BT_CLASSIC)
	struct bt_l2cap_br_chan *l2cap_br_chan = CONTAINER_OF(
			l2cap_chan, struct bt_l2cap_br_chan, chan);
	struct br_channel *br_chan = CONTAINER_OF(l2cap_br_chan, struct br_channel, br);
#endif /* CONFIG_BT_CLASSIC */
	struct bt_conn_info info;
	(void)memset(&ev, 0, sizeof(ev));
	/* TODO: ev.psm */
	if (!bt_conn_get_info(l2cap_chan->conn, &info)) {
		switch (info.type) {
		case BT_CONN_TYPE_LE:
			ev.chan_id = chan->chan_id;
			ev.mtu_remote = sys_cpu_to_le16(chan->le.tx.mtu);
			ev.mps_remote = sys_cpu_to_le16(chan->le.tx.mps);
			ev.mtu_local = sys_cpu_to_le16(chan->le.rx.mtu);
			ev.mps_local = sys_cpu_to_le16(chan->le.rx.mps);
			bt_addr_le_copy(&ev.address, info.le.dst);
			break;
		case BT_CONN_TYPE_BR:
#if defined(CONFIG_BT_CLASSIC)
			ev.chan_id = br_chan->chan_id;
			ev.psm = br_chan->br.psm;
			ev.mtu_remote = sys_cpu_to_le16(br_chan->br.tx.mtu);
			ev.mps_remote = sys_cpu_to_le16(br_chan->br.tx.mps);
			ev.mtu_local = sys_cpu_to_le16(br_chan->br.rx.mtu);
			ev.mps_local = sys_cpu_to_le16(br_chan->br.rx.mps);
			bt_addr_copy(&ev.address.a, info.br.dst);
			break;
#endif /* CONFIG_BT_CLASSIC */
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
#if defined(CONFIG_BT_CLASSIC)
	struct bt_l2cap_br_chan *l2cap_br_chan = CONTAINER_OF(
			l2cap_chan, struct bt_l2cap_br_chan, chan);
	struct br_channel *br_chan = CONTAINER_OF(l2cap_br_chan, struct br_channel, br);
#endif /* CONFIG_BT_CLASSIC */
	struct bt_conn_info info;

	/* release netbuf on premature disconnection */
	if (chan->pending_credit) {
		net_buf_unref(chan->pending_credit);
		chan->pending_credit = NULL;
	}

	(void)memset(&ev, 0, sizeof(struct btp_l2cap_disconnected_ev));

	/* TODO: ev.result */
	/* TODO: ev.psm */
	if (!bt_conn_get_info(l2cap_chan->conn, &info)) {
		switch (info.type) {
		case BT_CONN_TYPE_LE:
			ev.chan_id = chan->chan_id;
			ev.psm = chan->le.psm;
			bt_addr_le_copy(&ev.address, info.le.dst);
			chan->in_use = false;
			break;
		case BT_CONN_TYPE_BR:
#if defined(CONFIG_BT_CLASSIC)
			ev.chan_id = br_chan->chan_id;
			ev.psm = br_chan->br.psm;
			br_chan->in_use = false;
			bt_addr_copy(&ev.address.a, info.br.dst);
			break;
#endif /* CONFIG_BT_CLASSIC */
		default:
			/* TODO figure out how (if) want to handle BR/EDR */
			return;
		}
	}

	tester_event(BTP_SERVICE_ID_L2CAP, BTP_L2CAP_EV_DISCONNECTED, &ev, sizeof(ev));
}

#if defined(CONFIG_BT_L2CAP_ECRED)
static void reconfigured_cb(struct bt_l2cap_chan *l2cap_chan)
{
	struct btp_l2cap_reconfigured_ev ev;
#if defined(CONFIG_BT_CLASSIC)
	struct bt_conn_info info;
#endif /* CONFIG_BT_CLASSIC */
	struct bt_l2cap_le_chan *l2cap_le_chan;
	struct channel *chan;
#if defined(CONFIG_BT_CLASSIC)
	struct bt_l2cap_br_chan *l2cap_br_chan;
	struct br_channel *br_chan;
#endif /* CONFIG_BT_CLASSIC */

	(void)memset(&ev, 0, sizeof(ev));

#if defined(CONFIG_BT_CLASSIC)
	(void)bt_conn_get_info(l2cap_chan->conn, &info);

	if (BT_CONN_TYPE_BR == info.type) {
		l2cap_br_chan = CONTAINER_OF(l2cap_chan, struct bt_l2cap_br_chan, chan);
		br_chan = CONTAINER_OF(l2cap_br_chan, struct br_channel, br);

		ev.chan_id = br_chan->chan_id;
		ev.mtu_remote = sys_cpu_to_le16(br_chan->br.tx.mtu);
		ev.mps_remote = sys_cpu_to_le16(br_chan->br.tx.mps);
		ev.mtu_local = sys_cpu_to_le16(br_chan->br.rx.mtu);
		ev.mps_local = sys_cpu_to_le16(br_chan->br.rx.mps);
	} else if (BT_CONN_TYPE_LE ==  info.type)
#endif /* CONFIG_BT_CLASSIC */
	{
		l2cap_le_chan = CONTAINER_OF(l2cap_chan, struct bt_l2cap_le_chan, chan);
		chan = CONTAINER_OF(l2cap_le_chan, struct channel, le);

		ev.chan_id = chan->chan_id;
		ev.mtu_remote = sys_cpu_to_le16(chan->le.tx.mtu);
		ev.mps_remote = sys_cpu_to_le16(chan->le.tx.mps);
		ev.mtu_local = sys_cpu_to_le16(chan->le.rx.mtu);
		ev.mps_local = sys_cpu_to_le16(chan->le.rx.mps);
	}
#if defined(CONFIG_BT_CLASSIC)
	else {
		/* misra rule */
	}
#endif /* CONFIG_BT_CLASSIC */

	tester_event(BTP_SERVICE_ID_L2CAP, BTP_L2CAP_EV_RECONFIGURED, &ev, sizeof(ev));
}
#endif

#if (defined(CONFIG_BT_L2CAP_IFRAME_SUPPORT) && (CONFIG_BT_L2CAP_IFRAME_SUPPORT > 0U))
#if (defined(CONFIG_BT_L2CAP_MODE_SELECT) && (CONFIG_BT_L2CAP_MODE_SELECT > 0U))
/* support L2CAP Enhanced Retransmission Mode */
static struct bt_l2cap_retrans_fc en_retrans_fc = {
	.mode = L2CAP_MODE_ERTM,
	.tx_window_size = 10U,
	.max_transmit = 20U,
	.retrans_time_out = 10000U,
	.monitor_time_out = 2000U,
	.max_pdu = 200U,
};

/* support L2CAP Streaming Mode */
static struct bt_l2cap_retrans_fc sm_fc = {
	.mode = L2CAP_MODE_SM,
	.tx_window_size = 7U,
	.max_transmit = 5U,
	.retrans_time_out = 10000U,
	.monitor_time_out = 2000U,
	.max_pdu = 100U,
};
#endif /* CONFIG_BT_L2CAP_MODE_SELECT */

#if (defined(CONFIG_BT_L2CAP_NO_FCS) && (CONFIG_BT_L2CAP_NO_FCS > 0U))
static uint8_t no_fcs_type = 0x00;
#endif /* CONFIG_BT_L2CAP_NO_FCS */

void get_configuration(struct bt_l2cap_chan *chan, struct bt_l2cap_cfg_options *cfg)
{
	if(NULL != cfg->retrans_fc) {
		switch(cfg->retrans_fc->mode) {
		case L2CAP_MODE_ERTM:
		  memcpy(cfg->retrans_fc, &en_retrans_fc, sizeof(struct bt_l2cap_retrans_fc));
		  break;

		case L2CAP_MODE_SM:
		  memcpy(cfg->retrans_fc, &sm_fc, sizeof(struct bt_l2cap_retrans_fc));
		  break;

		default:
		  break;
	  }
	} else {
		/* get channel's configuration from application */
#if (defined(CONFIG_BT_L2CAP_MODE_SELECT) && (CONFIG_BT_L2CAP_MODE_SELECT > 3U))
		cfg->retrans_fc = &sm_fc;
#elif (defined(CONFIG_BT_L2CAP_MODE_SELECT) && (CONFIG_BT_L2CAP_MODE_SELECT > 2U))
		cfg->retrans_fc = &en_retrans_fc;
#else
		cfg->retrans_fc = NULL;
#endif

#if (defined(CONFIG_BT_L2CAP_NO_FCS) && (CONFIG_BT_L2CAP_NO_FCS > 0U))
		cfg->fcs_type = &no_fcs_type;
#endif
	}
}

void configuration_request(struct bt_l2cap_chan *chan, struct bt_l2cap_cfg_options *cfg, struct bt_l2cap_cfg_options *rsp)
{
	/* configuration request is received from peer device */
	memcpy(rsp, cfg, sizeof(struct bt_l2cap_cfg_options));
}

void configuration_response(struct bt_l2cap_chan *chan, struct bt_l2cap_cfg_options *rsp)
{
	/* configuration response is received from peer device */
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
#if defined(CONFIG_BT_L2CAP_IFRAME_SUPPORT)
	.get_cfg = get_configuration,
	.cfg_req = configuration_request,
	.cfg_rsp = configuration_response,
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
static struct br_channel *get_free_br_channel()
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
#endif

static uint8_t connect(const void *cmd, uint16_t cmd_len,
			   void *rsp, uint16_t *rsp_len)
{
	const struct btp_l2cap_connect_cmd *cp = cmd;
	struct btp_l2cap_connect_rp *rp = rsp;
	struct bt_conn *conn;
	struct channel *chan = NULL;
#if defined(CONFIG_BT_CLASSIC)
	struct br_channel *br_chan = NULL;
	struct bt_l2cap_server *server = NULL;
#endif /* CONFIG_BT_CLASSIC */
	struct bt_l2cap_chan *allocated_channels[5] = {};
	uint16_t mtu = sys_le16_to_cpu(cp->mtu);
	uint16_t psm = sys_le16_to_cpu(cp->psm);
	uint8_t i = 0;
	bool ecfc = cp->options & BTP_L2CAP_CONNECT_OPT_ECFC;
	int err;
	struct bt_conn_info info = {0};

	if (cp->num == 0 || cp->num > CHANNELS || mtu > DATA_MTU_INITIAL) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
#if defined(CONFIG_BT_CLASSIC)
	if (NULL == conn)
	{
		conn = bt_conn_lookup_addr_br(&cp->address.a);
	}

	server = get_server(psm);
	if (NULL == server)
	{
		return BTP_STATUS_FAILED;
	}
#endif /* CONFIG_BT_CLASSIC */

	if (!conn) {
		return BTP_STATUS_FAILED;
	}

	(void)bt_conn_get_info(conn, &info);

	for (i = 0U; i < cp->num; i++) {
#if defined(CONFIG_BT_CLASSIC)
		if (BT_CONN_TYPE_BR == info.type)
		{
			br_chan = get_free_br_channel();
			if (!br_chan)
			{
				goto fail;
			}
			br_chan->br.chan.ops = &l2cap_ops;
			br_chan->br.rx.mtu = mtu;
			rp->chan_id[i] = br_chan->chan_id;
	#if (defined(CONFIG_BT_L2CAP_ECRED) && (CONFIG_BT_L2CAP_ECRED == 1)) || \
		(defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL) && (CONFIG_BT_L2CAP_DYNAMIC_CHANNEL > 0U))
			allocated_channels[i] = &br_chan->br.chan;
	#endif /* CONFIG_BT_L2CAP_ECRED || CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */
			br_chan->hold_credit = cp->options & BTP_L2CAP_CONNECT_OPT_HOLD_CREDIT;
		}
		else
#endif /* CONFIG_BT_CLASSIC */
		{
			chan = get_free_channel();
			if (!chan) {
				goto fail;
			}
			chan->le.chan.ops = &l2cap_ops;
			chan->le.rx.mtu = mtu;
			rp->chan_id[i] = chan->chan_id;
			allocated_channels[i] = &chan->le.chan;

			chan->hold_credit = cp->options & BTP_L2CAP_CONNECT_OPT_HOLD_CREDIT;
		}
	}

	if (cp->num == 1 && !ecfc) {
#if defined(CONFIG_BT_CLASSIC)
		if (BT_CONN_TYPE_BR == info.type)
		{
			err = bt_l2cap_chan_connect(conn, &br_chan->br.chan, psm);
			if (err < 0) {
				goto fail;
			}
		}
		else
#endif /* CONFIG_BT_CLASSIC */
		{
			err = bt_l2cap_chan_connect(conn, &chan->le.chan, psm);
			if (err < 0) {
				goto fail;
			}
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

	bt_conn_unref(conn);
	return BTP_STATUS_SUCCESS;

fail:
	for (i = 0U; i < ARRAY_SIZE(allocated_channels); i++) {
		if (allocated_channels[i]) {
#if defined(CONFIG_BT_CLASSIC)
			if (BT_CONN_TYPE_BR == info.type)
			{
				CONTAINER_OF(CONTAINER_OF(allocated_channels[i], struct bt_l2cap_br_chan, chan), struct br_channel, br)->in_use = false;
			}
			else
#endif /* CONFIG_BT_CLASSIC */
			{
				channels[BT_L2CAP_LE_CHAN(allocated_channels[i])->ident].in_use = false;
			}
		}
	}
	bt_conn_unref(conn);
	return BTP_STATUS_FAILED;
}

static uint8_t disconnect(const void *cmd, uint16_t cmd_len,
			  void *rsp, uint16_t *rsp_len)
{
	const struct btp_l2cap_disconnect_cmd *cp = cmd;
	struct channel *chan;
#if defined(CONFIG_BT_CLASSIC)
	struct br_channel *br_chan;
#endif /* CONFIG_BT_CLASSIC */
	int err;

	if (cp->chan_id >= CHANNELS) {
		return BTP_STATUS_FAILED;
	}
#if defined(CONFIG_BT_CLASSIC)
	if(BTP_L2CAP_TRANSPORT_BREDR == cp->transport) {
		/* BR/EDR transport */

		br_chan = &br_channels[cp->chan_id];

		err = bt_l2cap_chan_disconnect(&br_chan->br.chan);
	}
	else
#endif /* CONFIG_BT_CLASSIC */
	{
		/* LE transport */
		chan = &channels[cp->chan_id];

		err = bt_l2cap_chan_disconnect(&chan->le.chan);
		if (err) {
			return BTP_STATUS_FAILED;
		}
	}
	return BTP_STATUS_SUCCESS;
}

#if defined(CONFIG_BT_L2CAP_ECRED)
static uint8_t reconfigure(const void *cmd, uint16_t cmd_len,
							void *rsp, uint16_t *rsp_len)
{
	const struct btp_l2cap_reconfigure_cmd *cp = cmd;
	uint16_t mtu, mps;
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

	mps = sys_le16_to_cpu(cp->mps);

	for (int i = 0; i < cp->num; i++) {
		if (cp->chan_id[i] > CHANNELS) {
			return BTP_STATUS_FAILED;
		}

		if (BTP_L2CAP_TRANSPORT_BREDR == cp->transport) {
			/* bredr */
			reconf_channels[i] = &br_channels[cp->chan_id[i]].br.chan;
		} else if (BTP_L2CAP_TRANSPORT_LE == cp->transport) {
			/* le */
			reconf_channels[i] = &channels[cp->chan_id[i]].le.chan;
		} else {
			/* invalid transport type */
		}
	}

#if defined(CONFIG_BT_CLASSIC)
	if (BTP_L2CAP_TRANSPORT_BREDR == cp->transport) {
		/* bredr */
		conn = bt_conn_lookup_addr_br(&cp->address.a);
	} else 
#endif
	if (BTP_L2CAP_TRANSPORT_LE == cp->transport) {
		/* le */
		conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	} else {
		/* invalid transport type */
		conn = NULL;
	}
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

#if 0
	/* support CONFIG_BT_CLASSIC */
	err = bt_l2cap_ecred_chan_reconfigure(reconf_channels, mtu, mps);
#endif
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
#if defined(CONFIG_BT_CLASSIC)
	struct br_channel *br_chan;
#endif /* CONFIG_BT_CLASSIC */
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

#if defined(CONFIG_BT_CLASSIC)
	if(BTP_L2CAP_TRANSPORT_BREDR == cp->transport) {
		/* BR/EDR transport */
		br_chan = &br_channels[cp->chan_id];
		data_len = sys_le16_to_cpu(cp->data_len);

		/* FIXME: For now, fail if data length exceeds buffer length */
		if (data_len > DATA_MTU) {
			return BTP_STATUS_FAILED;
		}

		/* FIXME: For now, fail if data length exceeds remote's L2CAP SDU */
		if (data_len > br_chan->br.tx.mtu) {
			return BTP_STATUS_FAILED;
		}
	}
	else
#endif /* CONFIG_BT_CLASSIC */
	{
		/* LE transport */
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
	}

	buf = net_buf_alloc(&data_pool, K_FOREVER);
	net_buf_reserve(buf, BT_L2CAP_SDU_CHAN_SEND_RESERVE);

	net_buf_add_mem(buf, cp->data, data_len);

#if defined(CONFIG_BT_CLASSIC)
	if(BTP_L2CAP_TRANSPORT_BREDR == cp->transport) {
		/* BR/EDR transport */
		ret = bt_l2cap_chan_send(&br_chan->br.chan, buf);
	}
	else
#endif /* CONFIG_BT_CLASSIC */
	{
		/* LE transport */
		ret = bt_l2cap_chan_send(&chan->le.chan, buf);
	}

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

#if defined(CONFIG_BT_CLASSIC)
static struct bt_l2cap_server *get_server(uint16_t psm)
{
	uint8_t i;
	struct bt_l2cap_server *pFreeServer = NULL;

	for (i = 0U; i < SERVERS ; i++) {
		if (servers[i].psm == psm) {
			pFreeServer = &servers[i];
			break;
		}
	}

	return pFreeServer;
}
#endif

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
#if defined(CONFIG_BT_CLASSIC)
	struct br_channel *br_chan;
#endif /* CONFIG_BT_CLASSIC */
	struct bt_conn_info info;

	if (bt_conn_enc_key_size(conn) < req_keysize) {
		return -EPERM;
	}

	if (authorize_flag) {
		return -EACCES;
	}

	(void)bt_conn_get_info(conn, &info);

#if defined(CONFIG_BT_CLASSIC)
	if (BT_CONN_TYPE_BR == info.type) {
		br_chan = get_free_br_channel();
		if (!br_chan) {
			return -ENOMEM;
		}

		br_chan->br.chan.ops = &l2cap_ops;
		br_chan->br.rx.mtu = DATA_MTU_INITIAL;

		*l2cap_chan = &br_chan->br.chan;
	}
	else
#endif /* CONFIG_BT_CLASSIC */
	{
		chan = get_free_channel();
		if (!chan) {
			return -ENOMEM;
		}

		chan->le.chan.ops = &l2cap_ops;
		chan->le.rx.mtu = DATA_MTU_INITIAL;

		*l2cap_chan = &chan->le.chan;
	}

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
#if (defined(CONFIG_BT_L2CAP_IFRAME_SUPPORT) && (CONFIG_BT_L2CAP_IFRAME_SUPPORT > 0U))\
	/* support L2CAP Enhanced Retransmission Mode and Streaming Mode */
	server->feature_mask = 0x18; 
#endif

	switch (cp->response) {
	case BTP_L2CAP_CONNECTION_RESPONSE_SUCCESS:
#if defined(CONFIG_BT_CLASSIC)
		if (BTP_L2CAP_TRANSPORT_BREDR == cp->transport)
		{
			server->sec_level = BT_SECURITY_L2;
		}
#endif /* CONFIG_BT_CLASSIC */
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
	case BTP_L2CAP_CONNECTION_RESPONSE_MODE4_LEVEL4:
		server->sec_level = BT_SECURITY_L4;
		break;
	default:
		return BTP_STATUS_FAILED;
	}

#if defined(CONFIG_BT_CLASSIC)
	if (BTP_L2CAP_TRANSPORT_BREDR == cp->transport)
	{
		if (psm > 0x1000) {
			/* Dynamic BR/EDR PSM */
			if (bt_l2cap_br_server_register(server) < 0) {
				server->psm = 0U;
				return BTP_STATUS_FAILED;
			}
		} 
#if (defined(CONFIG_BT_L2CAP_ECRED) && (CONFIG_BT_L2CAP_ECRED > 0U))
		else if ((psm >= 0x80) && (psm <= 0xFF)) {
			/* Dynamic SPSM */
#if 0
			if (bt_l2cap_ecbfc_server_register(server) < 0) {
				server->psm = 0U;
				return BTP_STATUS_FAILED;
			}
#else
			return BTP_STATUS_FAILED;
#endif
		} 
#endif
		else {
			/* invalid PSM */
		}
	}
	else
#endif /* CONFIG_BT_CLASSIC */
	{
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
	struct br_channel *br_chan;
#endif

	if (cp->chan_id >= CHANNELS) {
		return BTP_STATUS_FAILED;
	}

#if defined(CONFIG_BT_CLASSIC)
	if (BTP_L2CAP_TRANSPORT_BREDR == cp->transport) {
		br_chan = &br_channels[cp->chan_id];

		if (!br_chan->in_use) {
			return BTP_STATUS_FAILED;
		}

		if (br_chan->pending_credit) {
			if (bt_l2cap_chan_recv_complete(&br_chan->br.chan,
							br_chan->pending_credit) < 0) {
				return BTP_STATUS_FAILED;
			}

			br_chan->pending_credit = NULL;
		}
	} else 
#endif
	{
		chan = &channels[cp->chan_id];

		if (!chan->in_use) {
			return BTP_STATUS_FAILED;
		}

		if (chan->pending_credit) {
			if (bt_l2cap_chan_recv_complete(&chan->le.chan,
							chan->pending_credit) < 0) {
				return BTP_STATUS_FAILED;
			}

			chan->pending_credit = NULL;
		}
	}

	return BTP_STATUS_SUCCESS;
}

#if defined(CONFIG_BT_CLASSIC)
static uint8_t echo(const void *cmd, uint16_t cmd_len,
			  void *rsp, uint16_t *rsp_len)
{
#if 0
	const struct btp_l2cap_echo_cmd *cp = cmd;

	if(0U != l2ca_ping_req((UCHAR *)((void *)cp->address.val), (UCHAR *)((void *)cp->data), cp->data_length)) {
		return BTP_STATUS_FAILED;
	}
#endif
	return BTP_STATUS_SUCCESS;
}

static uint8_t rx_flow_req(const void *cmd, uint16_t cmd_len,
			  void *rsp, uint16_t *rsp_len)
{
#if 0
	const struct btp_l2cap_rx_flow_req_cmd *cp = cmd;
	struct br_channel *br_chan;

	if (cp->chan_id >= CHANNELS) {
		return BTP_STATUS_FAILED;
	}

	br_chan = &br_channels[cp->chan_id];

	if(0 == cp->flow) {
		l2ca_rx_flow_req(br_chan->br.rx.cid, L2CAP_FEC_RX_FLOW_ON);
	} else {
		l2ca_rx_flow_req(br_chan->br.rx.cid, L2CAP_FEC_RX_FLOW_OFF);
	}
#endif
	return BTP_STATUS_SUCCESS;
}
#endif

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
#if defined(CONFIG_BT_CLASSIC)
	{
		.opcode = BTP_L2CAP_ECHO,
		.expect_len = sizeof(struct btp_l2cap_echo_cmd),
		.func = echo,
	},
	{
		.opcode = BTP_L2CAP_RX_FLOW_REQ,
		.expect_len = sizeof(struct btp_l2cap_rx_flow_req_cmd),
		.func = rx_flow_req,
	},
#endif
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
