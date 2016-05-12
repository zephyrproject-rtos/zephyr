/* l2cap_br.c - L2CAP BREDR oriented handling */

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

#include "hci_core.h"
#include "conn_internal.h"
#include "l2cap_internal.h"

#if !defined(CONFIG_BLUETOOTH_DEBUG_L2CAP)
#undef BT_DBG
#define BT_DBG(fmt, ...)
#endif

#define BR_CHAN(_ch) CONTAINER_OF(_ch, struct bt_l2cap_br_chan, chan)

#define L2CAP_BR_PSM_START	0x0001
#define L2CAP_BR_PSM_END	0xffff

#define L2CAP_BR_DYN_CID_START	0x0040
#define L2CAP_BR_DYN_CID_END	0xffff

#define L2CAP_BR_MIN_MTU	48

#define L2CAP_BR_PSM_SDP	0x0001

/*
 * L2CAP extended feature mask:
 * BR/EDR fixed channel support enabled
 */
#define L2CAP_FEAT_FIXED_CHAN_MASK	0x00000080

static struct bt_l2cap_server *br_servers;
static struct bt_l2cap_fixed_chan *br_channels;

/* Pool for outgoing BR/EDR signaling packets, min MTU is 48 */
static struct nano_fifo br_sig;
static NET_BUF_POOL(br_sig_pool, CONFIG_BLUETOOTH_MAX_CONN,
		    BT_L2CAP_BUF_SIZE(L2CAP_BR_MIN_MTU), &br_sig, NULL,
		    BT_BUF_USER_DATA_MIN);

/* Set of flags applicable on "flags" member of bt_l2cap_br context */
enum {
	BT_L2CAP_FLAG_INFO_PENDING,	/* retrieving remote l2cap info */
	BT_L2CAP_FLAG_INFO_DONE,	/* remote l2cap info is done */
};

/* BR/EDR L2CAP signalling channel specific context */
struct bt_l2cap_br {
	/* The channel this context is associated with */
	struct bt_l2cap_br_chan	chan;
	atomic_t		flags[1];
	uint8_t			info_ident;
	uint8_t			info_fixed_chan;
	uint32_t		info_feat_mask;
};

static struct bt_l2cap_br bt_l2cap_br_pool[CONFIG_BLUETOOTH_MAX_CONN];

struct bt_l2cap_chan *bt_l2cap_br_lookup_rx_cid(struct bt_conn *conn,
						uint16_t cid)
{
	struct bt_l2cap_chan *chan;

	for (chan = conn->channels; chan; chan = chan->_next) {
		struct bt_l2cap_br_chan *ch = BR_CHAN(chan);

		if (ch->rx.cid == cid) {
			return chan;
		}
	}

	return NULL;
}

static struct bt_l2cap_chan *bt_l2cap_br_lookup_tx_cid(struct bt_conn *conn,
						       uint16_t cid)
{
	struct bt_l2cap_chan *chan;

	for (chan = conn->channels; chan; chan = chan->_next) {
		struct bt_l2cap_br_chan *ch = BR_CHAN(chan);

		if (ch->tx.cid == cid) {
			return chan;
		}
	}

	return NULL;
}

static struct bt_l2cap_br_chan*
l2cap_br_chan_alloc_cid(struct bt_conn *conn, struct bt_l2cap_chan *chan)
{
	struct bt_l2cap_br_chan *ch = BR_CHAN(chan);
	uint16_t cid;

	/*
	 * No action needed if there's already a CID allocated, e.g. in
	 * the case of a fixed channel.
	 */
	if (ch->rx.cid > 0) {
		return ch;
	}

	for (cid = L2CAP_BR_DYN_CID_START; cid <= L2CAP_BR_DYN_CID_END; cid++) {
		if (!bt_l2cap_br_lookup_rx_cid(conn, cid)) {
			ch->rx.cid = cid;
			return ch;
		}
	}

	return NULL;
}

static bool l2cap_br_chan_add(struct bt_conn *conn, struct bt_l2cap_chan *chan)
{
	struct bt_l2cap_br_chan *ch = l2cap_br_chan_alloc_cid(conn, chan);

	if (!ch) {
		BT_DBG("Unable to allocate L2CAP CID");
		return false;
	}

	/* Attach channel to the connection */
	chan->_next = conn->channels;
	conn->channels = chan;
	chan->conn = conn;

	BT_DBG("conn %p chan %p cid 0x%04x", conn, ch, ch->rx.cid);

	return true;
}

static uint8_t l2cap_br_get_ident(void)
{
	static uint8_t ident;

	ident++;
	/* handle integer overflow (0 is not valid) */
	if (!ident) {
		ident++;
	}

	return ident;
}

static void l2cap_br_get_info(struct bt_l2cap_br *l2cap, uint16_t info_type)
{
	struct bt_l2cap_info_req *info;
	struct net_buf *buf;
	struct bt_l2cap_sig_hdr *hdr;

	BT_DBG("info type %u", info_type);

	if (atomic_test_bit(l2cap->flags, BT_L2CAP_FLAG_INFO_PENDING)) {
		return;
	}

	switch (info_type) {
	case BT_L2CAP_INFO_FEAT_MASK:
	case BT_L2CAP_INFO_FIXED_CHAN:
		break;
	default:
		BT_WARN("Unsupported info type %u", info_type);
		return;
	}

	buf = bt_l2cap_create_pdu(&br_sig);
	if (!buf) {
		BT_ERR("No buffers");
		return;
	}

	atomic_set_bit(l2cap->flags, BT_L2CAP_FLAG_INFO_PENDING);
	l2cap->info_ident = l2cap_br_get_ident();

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_INFO_REQ;
	hdr->ident = l2cap->info_ident;
	hdr->len = sys_cpu_to_le16(sizeof(*info));

	info = net_buf_add(buf, sizeof(*info));
	info->type = sys_cpu_to_le16(info_type);

	/* TODO: add command timeout guard */
	bt_l2cap_send(l2cap->chan.chan.conn, BT_L2CAP_CID_BR_SIG, buf);
}

static int l2cap_br_info_rsp(struct bt_l2cap_br *l2cap, uint8_t ident,
			     struct net_buf *buf)
{
	struct bt_l2cap_info_rsp *rsp = (void *)buf->data;
	uint16_t type, result;
	int err = 0;

	if (atomic_test_bit(l2cap->flags, BT_L2CAP_FLAG_INFO_DONE)) {
		return 0;
	}

	atomic_clear_bit(l2cap->flags, BT_L2CAP_FLAG_INFO_PENDING);

	if (buf->len < sizeof(*rsp)) {
		BT_ERR("Too small info rsp packet size");
		err = -EINVAL;
		goto done;
	}

	if (ident != l2cap->info_ident) {
		BT_WARN("Idents mismatch");
		err = -EINVAL;
		goto done;
	}

	result = sys_le16_to_cpu(rsp->result);
	if (result != BT_L2CAP_INFO_SUCCESS) {
		BT_WARN("Result unsuccessful");
		err = -EINVAL;
		goto done;
	}

	type = sys_le16_to_cpu(rsp->type);
	net_buf_pull(buf, sizeof(*rsp));

	switch (type) {
	case BT_L2CAP_INFO_FEAT_MASK:
		l2cap->info_feat_mask = net_buf_pull_le32(buf);
		BT_DBG("remote info mask 0x%08x", l2cap->info_feat_mask);

		if (!(l2cap->info_feat_mask & L2CAP_FEAT_FIXED_CHAN_MASK)) {
			break;
		}

		l2cap_br_get_info(l2cap, BT_L2CAP_INFO_FIXED_CHAN);
		return 0;
	case BT_L2CAP_INFO_FIXED_CHAN:
		l2cap->info_fixed_chan = net_buf_pull_u8(buf);
		BT_DBG("remote fixed channel mask 0x%02x",
		       l2cap->info_fixed_chan);
		break;
	default:
		BT_WARN("type 0x%04x unsupported", type);
		err = -EINVAL;
		break;
	}
done:
	atomic_set_bit(l2cap->flags, BT_L2CAP_FLAG_INFO_DONE);
	l2cap->info_ident = 0;
	return err;
}

static int l2cap_br_info_req(struct bt_l2cap_br *l2cap, uint8_t ident,
			     struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_info_req *req = (void *)buf->data;
	struct bt_l2cap_info_rsp *rsp;
	struct net_buf *rsp_buf;
	struct bt_l2cap_sig_hdr *hdr_info;
	uint16_t type;

	if (buf->len < sizeof(*req)) {
		BT_ERR("Too small info req packet size");
		return -EINVAL;
	}

	rsp_buf = bt_l2cap_create_pdu(&br_sig);
	if (!rsp_buf) {
		BT_ERR("No buffers");
		return -ENOMEM;
	}

	type = sys_le16_to_cpu(req->type);
	BT_DBG("type 0x%04x", type);

	hdr_info = net_buf_add(rsp_buf, sizeof(*hdr_info));
	hdr_info->code = BT_L2CAP_INFO_RSP;
	hdr_info->ident = ident;

	rsp = net_buf_add(rsp_buf, sizeof(*rsp));

	switch (type) {
	case BT_L2CAP_INFO_FEAT_MASK:
		rsp->type = sys_cpu_to_le16(BT_L2CAP_INFO_FEAT_MASK);
		rsp->result = sys_cpu_to_le16(BT_L2CAP_INFO_SUCCESS);
		net_buf_add_le32(rsp_buf, L2CAP_FEAT_FIXED_CHAN_MASK);
		hdr_info->len = sys_cpu_to_le16(sizeof(*rsp) + sizeof(uint32_t));
		break;
	case BT_L2CAP_INFO_FIXED_CHAN:
		rsp->type = sys_cpu_to_le16(BT_L2CAP_INFO_FIXED_CHAN);
		rsp->result = sys_cpu_to_le16(BT_L2CAP_INFO_SUCCESS);
		/* fixed channel mask protocol data is 8 octets wide */
		memset(net_buf_add(rsp_buf, 8), 0, 8);
		/* signaling channel is mandatory on BR/EDR transport */
		rsp->data[0] = BT_L2CAP_MASK_BR_SIG;
		hdr_info->len = sys_cpu_to_le16(sizeof(*rsp) + 8);
		break;
	default:
		rsp->type = req->type;
		rsp->result = sys_cpu_to_le16(BT_L2CAP_INFO_NOTSUPP);
		hdr_info->len = sys_cpu_to_le16(sizeof(*rsp));
		break;
	}

	bt_l2cap_send(conn, BT_L2CAP_CID_BR_SIG, rsp_buf);

	return 0;
}

void bt_l2cap_br_connected(struct bt_conn *conn)
{
	struct bt_l2cap_fixed_chan *fchan;
	struct bt_l2cap_chan *chan;
	struct bt_l2cap_br *l2cap;

	fchan = br_channels;

	for (; fchan; fchan = fchan->_next) {
		struct bt_l2cap_br_chan *ch;

		if (!fchan->accept) {
			continue;
		}

		if (fchan->accept(conn, &chan) < 0) {
			continue;
		}

		ch = BR_CHAN(chan);

		ch->rx.cid = fchan->cid;
		ch->tx.cid = fchan->cid;

		if (!l2cap_br_chan_add(conn, chan)) {
			return;
		}

		if (chan->ops && chan->ops->connected) {
			chan->ops->connected(chan);
		}
	}

	l2cap = CONTAINER_OF(chan, struct bt_l2cap_br, chan.chan);
	l2cap_br_get_info(l2cap, BT_L2CAP_INFO_FEAT_MASK);
}

static struct bt_l2cap_server *l2cap_br_server_lookup_psm(uint16_t psm)
{
	struct bt_l2cap_server *server;

	for (server = br_servers; server; server = server->_next) {
		if (server->psm == psm) {
			return server;
		}
	}

	return NULL;
}

static void l2cap_br_conn_req(struct bt_l2cap_br *l2cap, uint8_t ident,
			      struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_chan *chan;
	struct bt_l2cap_server *server;
	struct bt_l2cap_conn_req *req = (void *)buf->data;
	struct bt_l2cap_conn_rsp *rsp;
	struct bt_l2cap_sig_hdr *hdr;
	uint16_t psm, scid, dcid, result;

	if (buf->len < sizeof(*req)) {
		BT_ERR("Too small L2CAP conn req packet size");
		return;
	}

	psm = sys_le16_to_cpu(req->psm);
	scid = sys_le16_to_cpu(req->scid);
	dcid = 0;

	BT_DBG("psm 0x%02x scid 0x%04x", psm, scid);

	buf = bt_l2cap_create_pdu(&br_sig);
	if (!buf) {
		return;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_CONN_RSP;
	hdr->ident = ident;
	hdr->len = sys_cpu_to_le16(sizeof(*rsp));

	rsp = net_buf_add(buf, sizeof(*rsp));
	memset(rsp, 0, sizeof(*rsp));

	/* Check if there is a server registered */
	server = l2cap_br_server_lookup_psm(psm);
	if (!server) {
		result = BT_L2CAP_ERR_PSM_NOT_SUPP;
		goto done;
	}

	/*
	 * Report security violation for non SDP channel without encryption when
	 * remote supports SSP.
	 */
	if (psm != L2CAP_BR_PSM_SDP && lmp_ssp_host_supported(conn) &&
	    !conn->encrypt) {
		result = BT_L2CAP_ERR_SEC_BLOCK;
		goto done;
	}

	if (scid < L2CAP_BR_DYN_CID_START || scid > L2CAP_BR_DYN_CID_END) {
		result = BT_L2CAP_ERR_INVALID_SCID;
		goto done;
	}

	chan = bt_l2cap_br_lookup_tx_cid(conn, scid);
	if (chan) {
		result = BT_L2CAP_ERR_SCID_IN_USE;
		goto done;
	}

	/*
	 * Request server to accept the new connection and allocate the
	 * channel.
	 */
	if (server->accept(conn, &chan) < 0) {
		result = BT_L2CAP_ERR_NO_RESOURCES;
		goto done;
	}

	l2cap_br_chan_add(conn, chan);
	BR_CHAN(chan)->tx.cid = scid;
	dcid = BR_CHAN(chan)->rx.cid;

	/*
	 * TODO: Verify security level on link if this PSM channel requires
	 * higher security.
	 */

	result = BT_L2CAP_SUCCESS;
done:
	rsp->dcid = sys_cpu_to_le16(dcid);
	rsp->scid = req->scid;
	rsp->result = sys_cpu_to_le16(result);
	/* TODO: add command timeout guard */
	bt_l2cap_send(conn, BT_L2CAP_CID_BR_SIG, buf);

	/* Disconnect link when security rules were violated */
	if (result == BT_L2CAP_ERR_SEC_BLOCK) {
		bt_conn_disconnect(conn, BT_HCI_ERR_AUTHENTICATION_FAIL);
	}
}

int bt_l2cap_br_server_register(struct bt_l2cap_server *server)
{
	if (server->psm < L2CAP_BR_PSM_START || !server->accept) {
		return -EINVAL;
	}

	/* PSM must be odd and lsb of upper byte must be 0 */
	if ((server->psm & 0x0101) != 0x0001) {
		return -EINVAL;
	}

	/* Check if given PSM is already in use */
	if (l2cap_br_server_lookup_psm(server->psm)) {
		BT_DBG("PSM already registered");
		return -EADDRINUSE;
	}

	BT_DBG("PSM 0x%04x", server->psm);

	server->_next = br_servers;
	br_servers = server;

	return 0;
}

static void l2cap_br_send_reject(struct bt_conn *conn, uint8_t ident,
				 uint16_t reason, void *data, uint8_t data_len)
{
	struct bt_l2cap_cmd_reject *rej;
	struct bt_l2cap_sig_hdr *hdr;
	struct net_buf *buf;

	buf = bt_l2cap_create_pdu(&br_sig);
	if (!buf) {
		return;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_CMD_REJECT;
	hdr->ident = ident;
	hdr->len = sys_cpu_to_le16(sizeof(*rej) + data_len);

	rej = net_buf_add(buf, sizeof(*rej));
	rej->reason = sys_cpu_to_le16(reason);

	/*
	 * optional data if available must be already in little-endian format
	 * made by caller.and be compliant with Core 4.2 [Vol 3, Part A, 4.1,
	 * table 4.4]
	 */
	if (data) {
		memcpy(net_buf_add(buf, data_len), data, data_len);
	}

	bt_l2cap_send(conn, BT_L2CAP_CID_BR_SIG, buf);
}

static void l2cap_br_chan_del(struct bt_l2cap_chan *chan)
{
	BT_DBG("conn %p chan %p cid 0x%04x", chan->conn, BR_CHAN(chan),
	       BR_CHAN(chan)->rx.cid);

	chan->conn = NULL;

	if (chan->ops && chan->ops->disconnected) {
		chan->ops->disconnected(chan);
	}
}

static struct bt_l2cap_br_chan *l2cap_br_remove_tx_cid(struct bt_conn *conn,
						       uint16_t cid)
{
	struct bt_l2cap_chan *chan, *prev;

	for (chan = conn->channels, prev = NULL; chan;
	     prev = chan, chan = chan->_next) {
		/* get the app's l2cap object wherein this chan is contained */
		struct bt_l2cap_br_chan *ch = BR_CHAN(chan);

		if (ch->tx.cid != cid) {
			continue;
		}

		if (!prev) {
			conn->channels = chan->_next;
		} else {
			prev->_next = chan->_next;
		}

		return ch;
	}

	return NULL;
}

static void l2cap_br_disconn_req(struct bt_l2cap_br *l2cap, uint8_t ident,
				 struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.chan.conn;
	struct bt_l2cap_br_chan *chan;
	struct bt_l2cap_disconn_req *req = (void *)buf->data;
	struct bt_l2cap_disconn_rsp *rsp;
	struct bt_l2cap_sig_hdr *hdr;
	uint16_t scid, dcid;

	if (buf->len < sizeof(*req)) {
		BT_ERR("Too small disconn req packet size");
		return;
	}

	dcid = sys_le16_to_cpu(req->dcid);
	scid = sys_le16_to_cpu(req->scid);

	BT_DBG("scid 0x%04x dcid 0x%04x", dcid, scid);

	chan = l2cap_br_remove_tx_cid(conn, scid);
	if (!chan) {
		struct bt_l2cap_cmd_reject_cid_data data;

		data.scid = req->scid;
		data.dcid = req->dcid;
		l2cap_br_send_reject(conn, ident, BT_L2CAP_REJ_INVALID_CID,
				     &data, sizeof(data));
		return;
	}

	buf = bt_l2cap_create_pdu(&br_sig);
	if (!buf) {
		return;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_DISCONN_RSP;
	hdr->ident = ident;
	hdr->len = sys_cpu_to_le16(sizeof(*rsp));

	rsp = net_buf_add(buf, sizeof(*rsp));
	rsp->dcid = sys_cpu_to_le16(chan->rx.cid);
	rsp->scid = sys_cpu_to_le16(chan->tx.cid);

	l2cap_br_chan_del(&chan->chan);

	bt_l2cap_send(conn, BT_L2CAP_CID_BR_SIG, buf);
}

static void l2cap_br_connected(struct bt_l2cap_chan *chan)
{
	BT_DBG("ch %p cid 0x%04x", BR_CHAN(chan), BR_CHAN(chan)->rx.cid);
}

static void l2cap_br_disconnected(struct bt_l2cap_chan *chan)
{
	BT_DBG("ch %p cid 0x%04x", BR_CHAN(chan), BR_CHAN(chan)->rx.cid);
}

int bt_l2cap_br_chan_disconnect(struct bt_l2cap_chan *chan)
{
	return -ENOTSUP;
}

int bt_l2cap_br_chan_connect(struct bt_conn *conn, struct bt_l2cap_chan *chan,
			     uint16_t psm)
{
	return -ENOTSUP;
}

int bt_l2cap_br_chan_send(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	return -ENOTSUP;
}

static void l2cap_br_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_l2cap_br *l2cap = CONTAINER_OF(chan, struct bt_l2cap_br, chan);
	struct bt_l2cap_sig_hdr *hdr = (void *)buf->data;
	uint16_t len;

	if (buf->len < sizeof(*hdr)) {
		BT_ERR("Too small L2CAP signaling PDU");
		return;
	}

	len = sys_le16_to_cpu(hdr->len);
	net_buf_pull(buf, sizeof(*hdr));

	BT_DBG("Signaling code 0x%02x ident %u len %u", hdr->code,
	       hdr->ident, len);

	if (buf->len != len) {
		BT_ERR("L2CAP length mismatch (%u != %u)", buf->len, len);
		return;
	}

	if (!hdr->ident) {
		BT_ERR("Invalid ident value in L2CAP PDU");
		return;
	}

	switch (hdr->code) {
	case BT_L2CAP_INFO_RSP:
		l2cap_br_info_rsp(l2cap, hdr->ident, buf);
		break;
	case BT_L2CAP_INFO_REQ:
		l2cap_br_info_req(l2cap, hdr->ident, buf);
		break;
	case BT_L2CAP_DISCONN_REQ:
		l2cap_br_disconn_req(l2cap, hdr->ident, buf);
		break;
	case BT_L2CAP_CONN_REQ:
		l2cap_br_conn_req(l2cap, hdr->ident, buf);
		break;
	default:
		BT_WARN("Unknown/Unsupported L2CAP PDU code 0x%02x", hdr->code);
		l2cap_br_send_reject(chan->conn, hdr->ident,
				     BT_L2CAP_REJ_NOT_UNDERSTOOD, NULL, 0);
		break;
	}
}

static int l2cap_br_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	int i;
	static struct bt_l2cap_chan_ops ops = {
		.connected = l2cap_br_connected,
		.disconnected = l2cap_br_disconnected,
		.recv = l2cap_br_recv,
	};

	BT_DBG("conn %p handle %u", conn, conn->handle);

	for (i = 0; i < ARRAY_SIZE(bt_l2cap_br_pool); i++) {
		struct bt_l2cap_br *l2cap = &bt_l2cap_br_pool[i];

		if (l2cap->chan.chan.conn) {
			continue;
		}

		l2cap->chan.chan.ops = &ops;
		*chan = &l2cap->chan.chan;
		atomic_set(l2cap->flags, 0);
		return 0;
	}

	BT_ERR("No available L2CAP context for conn %p", conn);

	return -ENOMEM;
}

static void bt_l2cap_br_fixed_chan_register(struct bt_l2cap_fixed_chan *chan)
{
	BT_DBG("CID 0x%04x", chan->cid);

	chan->_next = br_channels;
	br_channels = chan;
}

void bt_l2cap_br_init(void)
{
	static struct bt_l2cap_fixed_chan chan_br = {
			.cid	= BT_L2CAP_CID_BR_SIG,
			.mask	= BT_L2CAP_MASK_BR_SIG,
			.accept = l2cap_br_accept,
			};

	net_buf_pool_init(br_sig_pool);
	bt_l2cap_br_fixed_chan_register(&chan_br);
}
