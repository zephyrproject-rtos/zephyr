/*  Bluetooth ISO */

/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/byteorder.h>
#include <sys/check.h>

#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/iso.h>

#include "host/hci_core.h"
#include "host/conn_internal.h"
#include "iso_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_ISO)
#define LOG_MODULE_NAME bt_iso
#include "common/log.h"

NET_BUF_POOL_FIXED_DEFINE(iso_tx_pool, CONFIG_BT_ISO_TX_BUF_COUNT,
			  CONFIG_BT_ISO_TX_MTU, NULL);
NET_BUF_POOL_FIXED_DEFINE(iso_rx_pool, CONFIG_BT_ISO_RX_BUF_COUNT,
			  CONFIG_BT_ISO_RX_MTU, NULL);

static struct bt_iso_recv_info iso_info_data[CONFIG_BT_ISO_RX_BUF_COUNT];
#define iso_info(buf) (&iso_info_data[net_buf_id(buf)])
#define iso_chan(_iso) ((_iso)->iso.chan);

#if CONFIG_BT_ISO_TX_FRAG_COUNT > 0
NET_BUF_POOL_FIXED_DEFINE(iso_frag_pool, CONFIG_BT_ISO_TX_FRAG_COUNT,
			  CONFIG_BT_ISO_TX_MTU, NULL);
#endif /* CONFIG_BT_ISO_TX_FRAG_COUNT > 0 */

struct bt_conn iso_conns[CONFIG_BT_ISO_MAX_CHAN];

/* TODO: Allow more than one server? */
#if defined(CONFIG_BT_ISO_UNICAST)
struct bt_iso_cig cigs[CONFIG_BT_ISO_MAX_CIG];
static struct bt_iso_server *iso_server;
#endif /* CONFIG_BT_ISO_UNICAST */
#if defined(CONFIG_BT_ISO_BROADCAST)
struct bt_iso_big bigs[CONFIG_BT_ISO_MAX_BIG];
#endif /* defined(CONFIG_BT_ISO_BROADCAST) */

/* Prototype */
int hci_le_remove_cig(uint8_t cig_id);

struct bt_iso_data_path {
	/* Data Path direction */
	uint8_t dir;
	/* Data Path ID */
	uint8_t pid;
	/* Data Path param reference */
	struct bt_iso_chan_path *path;
};


struct net_buf *bt_iso_get_rx(k_timeout_t timeout)
{
	struct net_buf *buf = net_buf_alloc(&iso_rx_pool, timeout);

	if (buf) {
		net_buf_reserve(buf, BT_BUF_RESERVE);
		bt_buf_set_type(buf, BT_BUF_ISO_IN);
	}

	return buf;
}

static void bt_iso_send_cb(struct bt_conn *iso, void *user_data)
{
	struct bt_iso_chan *chan = iso->iso.chan;
	struct bt_iso_chan_ops *ops;

	__ASSERT(chan != NULL, "NULL chan for iso %p", iso);

	ops = chan->ops;

	if (ops != NULL && ops->sent != NULL) {
		ops->sent(chan);
	}
}

void hci_iso(struct net_buf *buf)
{
	struct bt_hci_iso_hdr *hdr;
	uint16_t handle, len;
	struct bt_conn *iso;
	uint8_t flags;

	BT_DBG("buf %p", buf);

	BT_ASSERT(buf->len >= sizeof(*hdr));

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	len = sys_le16_to_cpu(hdr->len);
	handle = sys_le16_to_cpu(hdr->handle);
	flags = bt_iso_flags(handle);

	iso(buf)->handle = bt_iso_handle(handle);
	iso(buf)->index = BT_CONN_INDEX_INVALID;

	BT_DBG("handle %u len %u flags %u", iso(buf)->handle, len, flags);

	if (buf->len != len) {
		BT_ERR("ISO data length mismatch (%u != %u)", buf->len, len);
		net_buf_unref(buf);
		return;
	}

	iso = bt_conn_lookup_handle(iso(buf)->handle);
	if (iso == NULL) {
		BT_ERR("Unable to find conn for handle %u", iso(buf)->handle);
		net_buf_unref(buf);
		return;
	}

	iso(buf)->index = bt_conn_index(iso);

	bt_conn_recv(iso, buf, flags);
	bt_conn_unref(iso);
}

struct bt_conn *iso_new(void)
{
	struct bt_conn *iso = bt_conn_new(iso_conns, ARRAY_SIZE(iso_conns));

	if (iso) {
		iso->type = BT_CONN_TYPE_ISO;
	} else {
		BT_DBG("Could not create new ISO");
	}

	return iso;
}

#if defined(CONFIG_NET_BUF_LOG)
struct net_buf *bt_iso_create_pdu_timeout_debug(struct net_buf_pool *pool,
						size_t reserve,
						k_timeout_t timeout,
						const char *func, int line)
#else
struct net_buf *bt_iso_create_pdu_timeout(struct net_buf_pool *pool,
					  size_t reserve, k_timeout_t timeout)
#endif
{
	if (!pool) {
		pool = &iso_tx_pool;
	}

	reserve += sizeof(struct bt_hci_iso_data_hdr);

#if defined(CONFIG_NET_BUF_LOG)
	return bt_conn_create_pdu_timeout_debug(pool, reserve, timeout, func,
						line);
#else
	return bt_conn_create_pdu_timeout(pool, reserve, timeout);
#endif
}

#if defined(CONFIG_NET_BUF_LOG)
struct net_buf *bt_iso_create_frag_timeout_debug(size_t reserve,
						 k_timeout_t timeout,
						 const char *func, int line)
#else
struct net_buf *bt_iso_create_frag_timeout(size_t reserve, k_timeout_t timeout)
#endif
{
	struct net_buf_pool *pool = NULL;

#if CONFIG_BT_ISO_TX_FRAG_COUNT > 0
	pool = &iso_frag_pool;
#endif /* CONFIG_BT_ISO_TX_FRAG_COUNT > 0 */

#if defined(CONFIG_NET_BUF_LOG)
	return bt_conn_create_pdu_timeout_debug(pool, reserve, timeout, func,
						line);
#else
	return bt_conn_create_pdu_timeout(pool, reserve, timeout);
#endif
}


static int hci_le_setup_iso_data_path(struct bt_conn *iso,
				      struct bt_iso_data_path *path)
{
	struct bt_hci_cp_le_setup_iso_path *cp;
	struct bt_hci_rp_le_setup_iso_path *rp;
	struct net_buf *buf, *rsp;
	uint8_t *cc;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SETUP_ISO_PATH, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(iso->handle);
	cp->path_dir = path->dir;
	cp->path_id = path->pid;
	cp->codec_id.coding_format = path->path->format;
	cp->codec_id.company_id = sys_cpu_to_le16(path->path->cid);
	cp->codec_id.vs_codec_id = sys_cpu_to_le16(path->path->vid);
	sys_put_le24(path->path->delay, cp->controller_delay);
	cp->codec_config_len = path->path->cc_len;
	cc = net_buf_add(buf, cp->codec_config_len);
	memcpy(cc, path->path->cc, cp->codec_config_len);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SETUP_ISO_PATH, buf, &rsp);
	if (err) {
		return err;
	}

	rp = (void *)rsp->data;
	if (rp->status || (sys_le16_to_cpu(rp->handle) != iso->handle)) {
		err = -EIO;
	}

	net_buf_unref(rsp);

	return err;
}

static int hci_le_remove_iso_data_path(struct bt_conn *iso, uint8_t dir)
{
	struct bt_hci_cp_le_remove_iso_path *cp;
	struct bt_hci_rp_le_remove_iso_path *rp;
	struct net_buf *buf, *rsp;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_REMOVE_ISO_PATH, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = iso->handle;
	cp->path_dir = dir;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_REMOVE_ISO_PATH, buf, &rsp);
	if (err) {
		return err;
	}

	rp = (void *)rsp->data;
	if (rp->status || (sys_le16_to_cpu(rp->handle) != iso->handle)) {
		err = -EIO;
	}

	net_buf_unref(rsp);

	return err;
}

static void bt_iso_chan_add(struct bt_conn *iso, struct bt_iso_chan *chan)
{
	/* Attach ISO channel to the connection */
	chan->iso = iso;
	iso->iso.chan = chan;

	BT_DBG("iso %p chan %p", iso, chan);
}

static int bt_iso_setup_data_path(struct bt_conn *iso)
{
	int err;
	struct bt_iso_chan *chan;
	struct bt_iso_chan_path default_path = { .pid = BT_ISO_DATA_PATH_HCI };
	struct bt_iso_data_path out_path = {
		.dir = BT_HCI_DATAPATH_DIR_CTLR_TO_HOST };
	struct bt_iso_data_path in_path = {
		.dir = BT_HCI_DATAPATH_DIR_HOST_TO_CTLR };
	struct bt_iso_chan_io_qos *tx_qos;
	struct bt_iso_chan_io_qos *rx_qos;

	chan = iso_chan(iso);
	if (chan == NULL) {
		return -EINVAL;
	}

	tx_qos = chan->qos->tx;
	rx_qos = chan->qos->rx;

	in_path.path = tx_qos && tx_qos->path ? tx_qos->path : &default_path;
	out_path.path = rx_qos && rx_qos->path ? rx_qos->path : &default_path;

	if (!tx_qos) {
		in_path.pid = BT_ISO_DATA_PATH_DISABLED;
	}

	if (!rx_qos) {
		out_path.pid = BT_ISO_DATA_PATH_DISABLED;
	}

	if (iso->iso.is_bis) {
		/* Only set one data path for BIS as per the spec */
		if (tx_qos) {
			return hci_le_setup_iso_data_path(iso, &in_path);

		} else {
			return hci_le_setup_iso_data_path(iso, &out_path);
		}

	} else {
		/* Setup both directions for CIS*/
		err = hci_le_setup_iso_data_path(iso, &in_path);
		if (err) {
			return err;
		}

		return hci_le_setup_iso_data_path(iso, &out_path);

	}
}

void bt_iso_connected(struct bt_conn *iso)
{
	struct bt_iso_chan *chan;

	if (iso == NULL || iso->type != BT_CONN_TYPE_ISO) {
		BT_DBG("Invalid parameters: iso %p iso->type %u", iso,
		       iso ? iso->type : 0);
		return;
	}

	BT_DBG("%p", iso);

	if (bt_iso_setup_data_path(iso)) {
		BT_ERR("Unable to setup data path");
		if (iso->iso.is_bis && IS_ENABLED(CONFIG_BT_CONN)) {
			bt_conn_disconnect(iso,
					   BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		}
		/* TODO: Handle BIG terminate for BIS */
		return;
	}

	chan = iso_chan(iso);
	if (chan == NULL) {
		BT_ERR("Could not lookup chan from connected ISO");
		return;
	}

	bt_iso_chan_set_state(chan, BT_ISO_CONNECTED);

	if (chan->ops->connected) {
		chan->ops->connected(chan);
	}
}

void bt_iso_remove_data_path(struct bt_conn *iso)
{
	BT_DBG("%p", iso);

	if (iso->iso.is_bis) {
		struct bt_iso_chan *chan;
		struct bt_iso_chan_io_qos *tx_qos;
		uint8_t dir;

		chan = iso_chan(iso);
		if (chan == NULL) {
			return;
		}

		tx_qos = chan->qos->tx;

		/* Only remove one data path for BIS as per the spec */
		if (tx_qos) {
			dir = BT_HCI_DATAPATH_DIR_HOST_TO_CTLR;
		} else {
			dir = BT_HCI_DATAPATH_DIR_CTLR_TO_HOST;
		}

		(void)hci_le_remove_iso_data_path(iso, dir);
	} else {
		/* Remove both directions for CIS*/

		/* TODO: Check which has been setup first to avoid removing
		 * data paths that are not setup
		 */
		(void)hci_le_remove_iso_data_path(iso,
						  BT_HCI_DATAPATH_DIR_CTLR_TO_HOST);
		(void)hci_le_remove_iso_data_path(iso,
						  BT_HCI_DATAPATH_DIR_HOST_TO_CTLR);
	}
}

static void bt_iso_chan_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	BT_DBG("%p, reason 0x%02x", chan, reason);

	__ASSERT(chan->iso != NULL, "NULL conn for iso chan %p", chan);

	bt_iso_chan_set_state(chan, BT_ISO_DISCONNECTED);

	/* The peripheral does not have the concept of a CIG, so once a CIS
	 * disconnects it is completely freed by unref'ing it
	 */
	if (IS_ENABLED(CONFIG_BT_ISO_UNICAST) && !chan->iso->iso.is_bis) {
		bt_iso_cleanup_acl(chan->iso);

		if (chan->iso->role == BT_HCI_ROLE_PERIPHERAL) {
			bt_conn_unref(chan->iso);
			chan->iso = NULL;
		} else {
			/* ISO data paths are automatically removed when the
			 * peripheral disconnects, so we only need to
			 * move it for the central
			 */
			bt_iso_remove_data_path(chan->iso);
		}
	}

	if (chan->ops->disconnected) {
		chan->ops->disconnected(chan, reason);
	}
}

void bt_iso_disconnected(struct bt_conn *iso)
{
	struct bt_iso_chan *chan;

	if (iso == NULL || iso->type != BT_CONN_TYPE_ISO) {
		BT_DBG("Invalid parameters: iso %p iso->type %u", iso,
		       iso ? iso->type : 0);
		return;
	}

	BT_DBG("%p", iso);

	chan = iso_chan(iso);
	if (chan == NULL) {
		BT_ERR("Could not lookup chan from disconnected ISO");
		return;
	}

	bt_iso_chan_disconnected(chan, iso->err);
}

#if defined(CONFIG_BT_DEBUG_ISO)
const char *bt_iso_chan_state_str(uint8_t state)
{
	switch (state) {
	case BT_ISO_DISCONNECTED:
		return "disconnected";
	case BT_ISO_CONNECT:
		return "connect";
	case BT_ISO_CONNECTED:
		return "connected";
	case BT_ISO_DISCONNECT:
		return "disconnect";
	default:
		return "unknown";
	}
}

void bt_iso_chan_set_state_debug(struct bt_iso_chan *chan, uint8_t state,
				 const char *func, int line)
{
	BT_DBG("chan %p iso %p %s -> %s", chan, chan->iso,
	       bt_iso_chan_state_str(chan->state),
	       bt_iso_chan_state_str(state));

	/* check transitions validness */
	switch (state) {
	case BT_ISO_DISCONNECTED:
		/* regardless of old state always allows this states */
		break;
	case BT_ISO_CONNECT:
		if (chan->state != BT_ISO_DISCONNECTED) {
			BT_WARN("%s()%d: invalid transition", func, line);
		}
		break;
	case BT_ISO_CONNECTED:
		if (chan->state != BT_ISO_CONNECT) {
			BT_WARN("%s()%d: invalid transition", func, line);
		}
		break;
	case BT_ISO_DISCONNECT:
		if (chan->state != BT_ISO_CONNECTED) {
			BT_WARN("%s()%d: invalid transition", func, line);
		}
		break;
	default:
		BT_ERR("%s()%d: unknown (%u) state was set", func, line, state);
		return;
	}

	chan->state = state;
}
#else
void bt_iso_chan_set_state(struct bt_iso_chan *chan, uint8_t state)
{
	chan->state = state;
}
#endif /* CONFIG_BT_DEBUG_ISO */

void bt_iso_recv(struct bt_conn *iso, struct net_buf *buf, uint8_t flags)
{
	struct bt_hci_iso_data_hdr *hdr;
	struct bt_iso_chan *chan;
	uint8_t pb, ts;
	uint16_t len, pkt_seq_no;

	pb = bt_iso_flags_pb(flags);
	ts = bt_iso_flags_ts(flags);

	BT_DBG("handle %u len %u flags 0x%02x pb 0x%02x ts 0x%02x",
	       iso->handle, buf->len, flags, pb, ts);

	/* When the PB_Flag does not equal 0b00, the fields Time_Stamp,
	 * Packet_Sequence_Number, Packet_Status_Flag and ISO_SDU_Length
	 * are omitted from the HCI ISO Data packet.
	 */
	switch (pb) {
	case BT_ISO_START:
	case BT_ISO_SINGLE:
		/* The ISO_Data_Load field contains either the first fragment
		 * of an SDU or a complete SDU.
		 */
		if (ts) {
			struct bt_hci_iso_ts_data_hdr *ts_hdr;

			ts_hdr = net_buf_pull_mem(buf, sizeof(*ts_hdr));
			iso_info(buf)->ts = sys_le32_to_cpu(ts_hdr->ts);

			hdr = &ts_hdr->data;
		} else {
			hdr = net_buf_pull_mem(buf, sizeof(*hdr));
			/* TODO: Generate a timestamp? */
			iso_info(buf)->ts = 0x00000000;
		}

		len = sys_le16_to_cpu(hdr->slen);
		flags = bt_iso_pkt_flags(len);
		len = bt_iso_pkt_len(len);
		pkt_seq_no = sys_le16_to_cpu(hdr->sn);
		iso_info(buf)->sn = pkt_seq_no;

		if (flags == BT_ISO_DATA_VALID) {
			iso_info(buf)->flags = BT_ISO_FLAGS_VALID;
		} else if (flags == BT_ISO_DATA_INVALID) {
			iso_info(buf)->flags = BT_ISO_FLAGS_ERROR;
		} else if (flags == BT_ISO_DATA_NOP) {
			iso_info(buf)->flags = BT_ISO_FLAGS_LOST;
		} else {
			BT_WARN("Invalid ISO packet status flag: %u", flags);
			iso_info(buf)->flags = 0;
		}

		BT_DBG("%s, len %u total %u flags 0x%02x timestamp %u",
		       pb == BT_ISO_START ? "Start" : "Single", buf->len, len,
		       flags, iso_info(buf)->ts);

		if (iso->rx) {
			BT_ERR("Unexpected ISO %s fragment",
			       pb == BT_ISO_START ? "Start" : "Single");
			bt_conn_reset_rx_state(iso);
		}

		iso->rx = buf;
		iso->rx_len = len - buf->len;
		if (iso->rx_len) {
			/* if iso->rx_len then package is longer than the
			 * buf->len and cannot fit in a SINGLE package
			 */
			if (pb == BT_ISO_SINGLE) {
				BT_ERR("Unexpected ISO single fragment");
				bt_conn_reset_rx_state(iso);
			}
			return;
		}
		break;

	case BT_ISO_CONT:
		/* The ISO_Data_Load field contains a continuation fragment of
		 * an SDU.
		 */
		if (!iso->rx) {
			BT_ERR("Unexpected ISO continuation fragment");
			net_buf_unref(buf);
			return;
		}

		BT_DBG("Cont, len %u rx_len %u", buf->len, iso->rx_len);

		if (buf->len > net_buf_tailroom(iso->rx)) {
			BT_ERR("Not enough buffer space for ISO data");
			bt_conn_reset_rx_state(iso);
			net_buf_unref(buf);
			return;
		}

		net_buf_add_mem(iso->rx, buf->data, buf->len);
		iso->rx_len -= buf->len;
		net_buf_unref(buf);
		return;

	case BT_ISO_END:
		/* The ISO_Data_Load field contains the last fragment of an
		 * SDU.
		 */
		BT_DBG("End, len %u rx_len %u", buf->len, iso->rx_len);

		if (iso->rx == NULL) {
			BT_ERR("Unexpected ISO end fragment");
			net_buf_unref(buf);
			return;
		}

		if (buf->len > net_buf_tailroom(iso->rx)) {
			BT_ERR("Not enough buffer space for ISO data");
			bt_conn_reset_rx_state(iso);
			net_buf_unref(buf);
			return;
		}

		(void)net_buf_add_mem(iso->rx, buf->data, buf->len);
		iso->rx_len -= buf->len;
		net_buf_unref(buf);

		break;
	default:
		BT_ERR("Unexpected ISO pb flags (0x%02x)", pb);
		bt_conn_reset_rx_state(iso);
		net_buf_unref(buf);
		return;
	}

	chan = iso_chan(iso);
	if (chan == NULL) {
		BT_ERR("Could not lookup chan from receiving ISO");
	} else if (chan->ops->recv != NULL) {
		chan->ops->recv(chan, iso_info(iso->rx), iso->rx);
	}

	bt_conn_reset_rx_state(iso);
}

int bt_iso_chan_send(struct bt_iso_chan *chan, struct net_buf *buf)
{
	struct bt_hci_iso_data_hdr *hdr;
	static uint16_t sn;

	CHECKIF(!chan || !buf) {
		BT_DBG("Invalid parameters: chan %p buf %p", chan, buf);
		return -EINVAL;
	}

	BT_DBG("chan %p len %zu", chan, net_buf_frags_len(buf));

	if (chan->state != BT_ISO_CONNECTED) {
		BT_DBG("Not connected");
		return -ENOTCONN;
	}

	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->sn = sys_cpu_to_le16(sn++);
	hdr->slen = sys_cpu_to_le16(bt_iso_pkt_len_pack(net_buf_frags_len(buf)
							- sizeof(*hdr),
							BT_ISO_DATA_VALID));

	return bt_conn_send_cb(chan->iso, buf, bt_iso_send_cb, NULL);
}

static bool valid_chan_io_qos(const struct bt_iso_chan_io_qos *io_qos,
			      bool is_tx)
{
	const size_t max_mtu = (is_tx ? CONFIG_BT_ISO_TX_MTU : CONFIG_BT_ISO_RX_MTU) -
					BT_ISO_CHAN_SEND_RESERVE;
	const size_t max_sdu = MIN(max_mtu, BT_ISO_MAX_SDU);

	if (io_qos->sdu > max_sdu) {
		BT_DBG("sdu (%u) shall be smaller than %zu",
		       io_qos->sdu, max_sdu);
		return false;
	}

	if (io_qos->phy > BT_GAP_LE_PHY_CODED) {
		BT_DBG("Invalid phy %u", io_qos->phy);
		return false;
	}

	return true;
}

#if defined(CONFIG_BT_ISO_UNICAST)
static bool valid_chan_qos(const struct bt_iso_chan_qos *qos)
{
	if (qos->rx != NULL) {
		if (!valid_chan_io_qos(qos->rx, false)) {
			BT_DBG("Invalid rx qos");
			return false;
		}
	} else if (qos->tx == NULL) {
		BT_DBG("Both rx and tx qos are NULL");
		return false;
	}

	if (qos->tx != NULL) {
		if (!valid_chan_io_qos(qos->tx, true)) {
			BT_DBG("Invalid tx qos");
			return false;
		}
	}

	return true;
}

void bt_iso_cleanup_acl(struct bt_conn *iso)
{
	BT_DBG("%p", iso);

	if (iso->iso.acl) {
		bt_conn_unref(iso->iso.acl);
		iso->iso.acl = NULL;
	}
}

void hci_le_cis_estabilished(struct net_buf *buf)
{
	struct bt_hci_evt_le_cis_established *evt = (void *)buf->data;
	uint16_t handle = sys_le16_to_cpu(evt->conn_handle);
	struct bt_conn *iso;

	BT_DBG("status %u handle %u", evt->status, handle);

	/* ISO connection handles are already assigned at this point */
	iso = bt_conn_lookup_handle(handle);
	if (!iso) {
		BT_ERR("No connection found for handle %u", handle);
		return;
	}

	CHECKIF(iso->type != BT_CONN_TYPE_ISO) {
		BT_DBG("Invalid connection type %u", iso->type);
		return;
	}

	if (!evt->status) {
		/* TODO: Add CIG sync delay */
		bt_conn_set_state(iso, BT_CONN_CONNECTED);
		bt_conn_unref(iso);
		return;
	}

	iso->err = evt->status;
	bt_iso_disconnected(iso);
	bt_conn_unref(iso);
}

int hci_le_reject_cis(uint16_t handle, uint8_t reason)
{
	struct bt_hci_cp_le_reject_cis *cp;
	struct net_buf *buf;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_REJECT_CIS, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(handle);
	cp->reason = reason;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_REJECT_CIS, buf, NULL);
	if (err) {
		return err;
	}

	return 0;
}

int hci_le_accept_cis(uint16_t handle)
{
	struct bt_hci_cp_le_accept_cis *cp;
	struct net_buf *buf;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_ACCEPT_CIS, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(handle);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_ACCEPT_CIS, buf, NULL);
	if (err) {
		return err;
	}

	return 0;
}

void hci_le_cis_req(struct net_buf *buf)
{
	struct bt_hci_evt_le_cis_req *evt = (void *)buf->data;
	uint16_t acl_handle = sys_le16_to_cpu(evt->acl_handle);
	uint16_t cis_handle = sys_le16_to_cpu(evt->cis_handle);
	struct bt_conn *acl, *iso;
	int err;

	BT_DBG("acl_handle %u cis_handle %u cig_id %u cis %u",
		acl_handle, cis_handle, evt->cig_id, evt->cis_id);

	/* Lookup existing connection with same handle */
	iso = bt_conn_lookup_handle(cis_handle);
	if (iso) {
		BT_ERR("Invalid ISO handle %u", cis_handle);
		hci_le_reject_cis(cis_handle, BT_HCI_ERR_CONN_LIMIT_EXCEEDED);
		bt_conn_unref(iso);
		return;
	}

	/* Lookup ACL connection to attach */
	acl = bt_conn_lookup_handle(acl_handle);
	if (!acl) {
		BT_ERR("Invalid ACL handle %u", acl_handle);
		hci_le_reject_cis(cis_handle, BT_HCI_ERR_UNKNOWN_CONN_ID);
		return;
	}

	/* Add ISO connection */
	iso = bt_conn_add_iso(acl);

	bt_conn_unref(acl);

	if (!iso) {
		BT_ERR("Could not create and add ISO to ACL %u", acl_handle);
		hci_le_reject_cis(cis_handle,
				  BT_HCI_ERR_INSUFFICIENT_RESOURCES);
		return;
	}

	iso->iso.cig_id = evt->cig_id;
	iso->iso.cis_id = evt->cis_id;

	/* Request application to accept */
	err = bt_iso_accept(acl, iso);
	if (err) {
		BT_DBG("App rejected ISO %d", err);
		bt_conn_unref(iso);
		hci_le_reject_cis(cis_handle,
				  BT_HCI_ERR_INSUFFICIENT_RESOURCES);
		return;
	}

	iso->handle = cis_handle;
	iso->role = BT_HCI_ROLE_PERIPHERAL;
	bt_conn_set_state(iso, BT_CONN_CONNECT);

	err = hci_le_accept_cis(cis_handle);
	if (err) {
		bt_conn_unref(iso);
		hci_le_reject_cis(cis_handle,
				  BT_HCI_ERR_INSUFFICIENT_RESOURCES);
		return;
	}
}

int hci_le_remove_cig(uint8_t cig_id)
{
	struct bt_hci_cp_le_remove_cig *req;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_REMOVE_CIG, sizeof(*req));
	if (!buf) {
		return -ENOBUFS;
	}

	req = net_buf_add(buf, sizeof(*req));

	memset(req, 0, sizeof(*req));

	req->cig_id = cig_id;

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_REMOVE_CIG, buf, NULL);
}

struct bt_conn *bt_conn_add_iso(struct bt_conn *acl)
{
	struct bt_conn *iso = iso_new();

	if (iso == NULL) {
		BT_ERR("Unable to allocate ISO connection");
		return NULL;
	}

	iso->iso.acl = bt_conn_ref(acl);

	return iso;
}

static struct net_buf *hci_le_set_cig_params(const struct bt_iso_cig *cig,
					     const struct bt_iso_cig_create_param *param)
{
	struct bt_hci_cp_le_set_cig_params *req;
	struct bt_hci_cis_params *cis_param;
	struct net_buf *buf;
	struct net_buf *rsp;
	int i, err;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_CIG_PARAMS,
				sizeof(*req) + sizeof(*cis_param) * param->num_cis);
	if (!buf) {
		return NULL;
	}

	req = net_buf_add(buf, sizeof(*req));

	memset(req, 0, sizeof(*req));

	req->cig_id = cig->id;
	req->c_latency = sys_cpu_to_le16(param->latency);
	req->p_latency = sys_cpu_to_le16(param->latency);
	sys_put_le24(param->interval, req->c_interval);
	sys_put_le24(param->interval, req->p_interval);

	req->sca = param->sca;
	req->packing = param->packing;
	req->framing = param->framing;
	req->num_cis = param->num_cis;

	/* Program the cis parameters */
	for (i = 0; i < param->num_cis; i++) {
		struct bt_iso_chan *cis = param->cis_channels[i];
		struct bt_iso_chan_qos *qos = cis->qos;

		cis_param = net_buf_add(buf, sizeof(*cis_param));

		memset(cis_param, 0, sizeof(*cis_param));

		cis_param->cis_id = cis->iso->iso.cis_id;

		if (!qos->tx && !qos->rx) {
			BT_ERR("Both TX and RX QoS are disabled");
			net_buf_unref(buf);
			return NULL;
		}

		if (!qos->tx) {
			/* Use RX PHY if TX is not set (disabled)
			 * to avoid setting invalid values
			 */
			cis_param->c_phy = qos->rx->phy;
		} else {
			cis_param->c_sdu = sys_cpu_to_le16(qos->tx->sdu);
			cis_param->c_phy = qos->tx->phy;
			cis_param->c_rtn = qos->tx->rtn;
		}

		if (!qos->rx) {
			/* Use TX PHY if RX is not set (disabled)
			 * to avoid setting invalid values
			 */
			cis_param->p_phy = qos->tx->phy;
		} else {
			cis_param->p_sdu = sys_cpu_to_le16(qos->rx->sdu);
			cis_param->p_phy = qos->rx->phy;
			cis_param->p_rtn = qos->rx->rtn;
		}
	}

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_CIG_PARAMS, buf, &rsp);
	if (err) {
		return NULL;
	}

	return rsp;
}

static struct bt_iso_cig *get_free_cig(void)
{
	/* We can use the index in the `cigs` array as CIG ID */

	for (int i = 0; i < ARRAY_SIZE(cigs); i++) {
		if (!cigs[i].initialized) {
			cigs[i].initialized = true;
			cigs[i].id = i;
			return &cigs[i];
		}
	}

	BT_DBG("Could not allocate any more CIGs");

	return NULL;
}

static int cig_init_cis(struct bt_iso_cig *cig)
{
	for (int i = 0; i < cig->num_cis; i++) {
		struct bt_iso_chan *cis = cig->cis[i];

		CHECKIF(cis == NULL) {
			BT_DBG("CIS was NULL");
			return -EINVAL;
		}

		CHECKIF(!valid_chan_qos(cis->qos)) {
			BT_DBG("Invalid QOS");
			return -EINVAL;
		}

		if (cis->iso != NULL) {
			BT_DBG("CIS conn was already allocated");
			return -EALREADY;
		}

		cis->iso = iso_new();
		if (cis->iso == NULL) {
			BT_ERR("Unable to allocate CIS connection");
			return -ENOMEM;
		}

		cis->iso->iso.cig_id = cig->id;
		cis->iso->iso.is_bis = false;
		cis->iso->iso.cis_id = i;

		bt_iso_chan_add(cis->iso, cis);
	}

	return 0;
}

static void cleanup_cig(struct bt_iso_cig *cig)
{
	for (int i = 0; i < cig->num_cis; i++) {
		struct bt_iso_chan *cis = cig->cis[i];

		if (cis != NULL && cis->iso != NULL) {
			bt_conn_unref(cis->iso);
			cis->iso = NULL;
		}
	}

	memset(cig, 0, sizeof(*cig));
}

int bt_iso_cig_create(const struct bt_iso_cig_create_param *param,
		      struct bt_iso_cig **out_cig)
{
	int err;
	struct net_buf *rsp;
	struct bt_iso_cig *cig;
	struct bt_hci_rp_le_set_cig_params *cig_rsp;

	CHECKIF(out_cig == NULL) {
		BT_DBG("out_cig is NULL");
		return -EINVAL;
	}

	*out_cig = NULL;

	/* Check if controller is ISO capable as a central */
	if (!BT_FEAT_LE_CIS_CENTRAL(bt_dev.le.features)) {
		return -ENOTSUP;
	}

	CHECKIF(param->cis_channels == NULL) {
		BT_DBG("NULL CIS channels");
		return -EINVAL;
	}

	CHECKIF(param->num_cis == 0) {
		BT_DBG("Invalid number of CIS %u", param->num_cis);
		return -EINVAL;
	}

	for (int i = 0; i < param->num_cis; i++) {
		CHECKIF(param->cis_channels[i] == NULL) {
			BT_DBG("NULL channel in cis_channels[%d]", i);
			return -EINVAL;
		}
	}

	CHECKIF(param->framing != BT_ISO_FRAMING_UNFRAMED &&
		param->framing != BT_ISO_FRAMING_FRAMED) {
		BT_DBG("Invalid framing parameter: %u", param->framing);
		return -EINVAL;
	}

	CHECKIF(param->packing != BT_ISO_PACKING_SEQUENTIAL &&
		param->packing != BT_ISO_PACKING_INTERLEAVED) {
		BT_DBG("Invalid packing parameter: %u", param->packing);
		return -EINVAL;
	}

	CHECKIF(param->num_cis > BT_ISO_MAX_GROUP_ISO_COUNT ||
		param->num_cis > CONFIG_BT_ISO_MAX_CHAN) {
		BT_DBG("num_cis (%u) shall be lower than: %u", param->num_cis,
		       MAX(CONFIG_BT_ISO_MAX_CHAN, BT_ISO_MAX_GROUP_ISO_COUNT));
		return -EINVAL;
	}

	CHECKIF(param->interval < BT_ISO_INTERVAL_MIN ||
		param->interval > BT_ISO_INTERVAL_MAX) {
		BT_DBG("Invalid interval: %u", param->interval);
		return -EINVAL;
	}

	CHECKIF(param->latency < BT_ISO_LATENCY_MIN ||
		param->latency > BT_ISO_LATENCY_MAX) {
		BT_DBG("Invalid latency: %u", param->latency);
		return -EINVAL;
	}

	cig = get_free_cig();

	if (!cig) {
		return -ENOMEM;
	}

	cig->cis = param->cis_channels;
	cig->num_cis = param->num_cis;

	err = cig_init_cis(cig);
	if (err) {
		BT_DBG("Could not init CIS %d", err);
		cleanup_cig(cig);
		return err;
	}

	rsp = hci_le_set_cig_params(cig, param);
	if (rsp == NULL) {
		BT_WARN("Unexpected response to hci_le_set_cig_params");
		err = -EIO;
		cleanup_cig(cig);
		return err;
	}

	cig_rsp = (void *)rsp->data;

	if (rsp->len < sizeof(cig_rsp) ||
	    cig_rsp->num_handles != param->num_cis) {
		BT_WARN("Unexpected response to hci_le_set_cig_params");
		err = -EIO;
		net_buf_unref(rsp);
		cleanup_cig(cig);
		return err;
	}

	for (int i = 0; i < cig_rsp->num_handles; i++) {
		struct bt_iso_chan *chan;

		chan = param->cis_channels[i];

		/* Assign the connection handle */
		chan->iso->handle = sys_le16_to_cpu(cig_rsp->handle[i]);
	}

	net_buf_unref(rsp);

	*out_cig = cig;

	return err;
}

int bt_iso_cig_terminate(struct bt_iso_cig *cig)
{
	int err;

	CHECKIF(cig == NULL) {
		BT_DBG("cig is NULL");
		return -EINVAL;
	}

	for (int i = 0; i < cig->num_cis; i++) {
		if (cig->cis[i]->state != BT_ISO_DISCONNECTED) {
			BT_DBG("[%d]: Channel is not disconnected", i);
			return -EINVAL;
		}
	}

	err = hci_le_remove_cig(cig->id);
	if (err != 0) {
		BT_DBG("Failed to terminate CIG: %d", err);
		return err;
	}

	cleanup_cig(cig);

	return 0;
}

static int hci_le_create_cis(const struct bt_iso_connect_param *param,
			     size_t count)
{
	struct bt_hci_cis *cis;
	struct bt_hci_cp_le_create_cis *req;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_CREATE_CIS,
				sizeof(*req) + sizeof(*cis) * count);
	if (!buf) {
		return -ENOBUFS;
	}

	req = net_buf_add(buf, sizeof(*req));

	memset(req, 0, sizeof(*req));

	req->num_cis = count;

	/* Program the cis parameters */
	for (int i = 0; i < count; i++) {
		cis = net_buf_add(buf, sizeof(*cis));

		memset(cis, 0, sizeof(*cis));

		cis->cis_handle = sys_cpu_to_le16(param[i].iso_chan->iso->handle);
		cis->acl_handle = sys_cpu_to_le16(param[i].acl->handle);
	}

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_CREATE_CIS, buf, NULL);
}

int bt_iso_accept(struct bt_conn *acl, struct bt_conn *iso)
{
	struct bt_iso_chan *chan;
	int err;

	CHECKIF(!iso || iso->type != BT_CONN_TYPE_ISO) {
		BT_DBG("Invalid parameters: iso %p iso->type %u", iso,
		       iso ? iso->type : 0);
		return -EINVAL;
	}

	BT_DBG("%p", iso);

	if (!iso_server) {
		return -ENOMEM;
	}

	err = iso_server->accept(acl, &chan);
	if (err < 0) {
		BT_ERR("Server failed to accept: %d", err);
		return err;
	}

	bt_iso_chan_add(iso, chan);
	bt_iso_chan_set_state(chan, BT_ISO_CONNECT);

	return 0;
}

int bt_iso_chan_connect(const struct bt_iso_connect_param *param, size_t count)
{
	int err;

	CHECKIF(param == NULL || count == 0) {
		BT_DBG("param is NULL");
		return -EINVAL;
	}

	CHECKIF(count == 0) {
		BT_DBG("Invalid count %zu", count);
		return -EINVAL;
	}

	CHECKIF(count > CONFIG_BT_ISO_MAX_CHAN) {
		return -EINVAL;
	}

	/* Validate input */
	for (int i = 0; i < count; i++) {
		CHECKIF(param[i].iso_chan == NULL) {
			BT_DBG("[%d]: Invalid iso (%p)", i, param[i].iso_chan);
			return -EINVAL;
		}

		CHECKIF(param[i].acl == NULL) {
			BT_DBG("[%d]: Invalid acl (%p)", i, param[i].acl);
			return -EINVAL;
		}

		CHECKIF((param[i].acl->type & BT_CONN_TYPE_LE) == 0) {
			BT_DBG("[%d]: acl type (%u) shall be an LE connection",
			       i, param[i].acl->type);
			return -EINVAL;
		}

		if (param[i].iso_chan->iso == NULL) {
			BT_DBG("[%d]: ISO has not been initialized in a CIG",
			       i);
			return -EINVAL;
		}

		if (param[i].iso_chan->state != BT_ISO_DISCONNECTED) {
			BT_DBG("[%d]: ISO is not in the BT_ISO_DISCONNECTED state: %u",
			       i, param[i].iso_chan->state);
			return -EINVAL;
		}
	}

	err = hci_le_create_cis(param, count);
	if (err) {
		BT_DBG("Failed to connect CISes: %d", err);
		return err;
	}

	/* Set connection states */
	for (int i = 0; i < count; i++) {
		param[i].iso_chan->iso->iso.acl = bt_conn_ref(param[i].acl);
		bt_conn_set_state(param[i].iso_chan->iso, BT_CONN_CONNECT);
		bt_iso_chan_set_state(param[i].iso_chan, BT_ISO_CONNECT);
	}

	return 0;
}

int bt_iso_chan_disconnect(struct bt_iso_chan *chan)
{
	CHECKIF(!chan) {
		BT_DBG("Invalid parameter: chan %p", chan);
		return -EINVAL;
	}

	CHECKIF(chan->iso == NULL) {
		BT_DBG("Channel has not been initialized in a CIG");
		return -EINVAL;
	}

	if (chan->iso->iso.acl == NULL) {
		BT_DBG("Channel is not connected");
		return -ENOTCONN;
	}

	return bt_conn_disconnect(chan->iso, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

int bt_iso_server_register(struct bt_iso_server *server)
{
	CHECKIF(!server) {
		BT_DBG("Invalid parameter: server %p", server);
		return -EINVAL;
	}

	/* Check if controller is ISO capable */
	if (!BT_FEAT_LE_CIS_PERIPHERAL(bt_dev.le.features)) {
		return -ENOTSUP;
	}

	if (iso_server) {
		return -EADDRINUSE;
	}

	if (!server->accept) {
		return -EINVAL;
	}

	if (server->sec_level > BT_SECURITY_L3) {
		return -EINVAL;
	} else if (server->sec_level < BT_SECURITY_L1) {
		/* Level 0 is only applicable for BR/EDR */
		server->sec_level = BT_SECURITY_L1;
	}

	BT_DBG("%p", server);

	iso_server = server;

	return 0;
}
#endif /* CONFIG_BT_ISO_UNICAST */

#if defined(CONFIG_BT_ISO_BROADCAST)

static struct bt_iso_big *get_free_big(void)
{
	/* We can use the index in the `bigs` array as BIG handles, for both
	 * broadcaster and receiver (even if the device is both!)
	 */

	for (int i = 0; i < ARRAY_SIZE(bigs); i++) {
		if (!atomic_test_and_set_bit(bigs[i].flags, BT_BIG_INITIALIZED)) {
			bigs[i].handle = i;
			return &bigs[i];
		}
	}

	BT_DBG("Could not allocate any more BIGs");

	return NULL;
}

static struct bt_iso_big *big_lookup_flag(int bit)
{
	for (int i = 0; i < ARRAY_SIZE(bigs); i++) {
		if (atomic_test_bit(bigs[i].flags, bit)) {
			return &bigs[i];
		}
	}

	BT_DBG("No BIG with flag bit %d set", bit);

	return NULL;
}

static void cleanup_big(struct bt_iso_big *big)
{
	for (int i = 0; i < big->num_bis; i++) {
		struct bt_iso_chan *bis = big->bis[i];

		if (bis != NULL && bis->iso != NULL) {
			bt_conn_unref(bis->iso);
			bis->iso = NULL;
		}
	}

	memset(big, 0, sizeof(*big));
}

static void big_disconnect(struct bt_iso_big *big, uint8_t reason)
{
	for (int i = 0; i < big->num_bis; i++) {
		big->bis[i]->iso->err = reason;

		bt_iso_disconnected(big->bis[i]->iso);
	}
}

static int big_init_bis(struct bt_iso_big *big, bool broadcaster)
{
	for (int i = 0; i < big->num_bis; i++) {
		struct bt_iso_chan *bis = big->bis[i];

		if (!bis) {
			BT_DBG("BIS was NULL");
			return -EINVAL;
		}

		if (bis->iso) {
			BT_DBG("BIS conn was already allocated");
			return -EALREADY;
		}

		CHECKIF(bis->qos == NULL) {
			BT_DBG("BIS QOS is NULL");
			return -EINVAL;
		}

		if (broadcaster) {
			CHECKIF(bis->qos->tx == NULL ||
				!valid_chan_io_qos(bis->qos->tx, true)) {
				BT_DBG("Invalid BIS QOS");
				return -EINVAL;
			}
		} else {
			CHECKIF(bis->qos->rx == NULL) {
				BT_DBG("Invalid BIS QOS");
				return -EINVAL;
			}
		}

		bis->iso = iso_new();

		if (!bis->iso) {
			BT_ERR("Unable to allocate BIS connection");
			return -ENOMEM;
		}

		bis->iso->iso.big_handle = big->handle;
		bis->iso->iso.is_bis = true;
		bis->iso->iso.bis_id = bt_conn_index(bis->iso);

		bt_iso_chan_add(bis->iso, bis);
	}

	return 0;
}

static int hci_le_create_big(struct bt_le_ext_adv *padv, struct bt_iso_big *big,
			     struct bt_iso_big_create_param *param)
{
	struct bt_hci_cp_le_create_big *req;
	struct bt_hci_cmd_state_set state;
	struct net_buf *buf;
	int err;
	static struct bt_iso_chan_qos *qos;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_CREATE_BIG, sizeof(*req));

	if (!buf) {
		return -ENOBUFS;
	}

	/* All BIS will share the same QOS */
	qos = big->bis[0]->qos;

	req = net_buf_add(buf, sizeof(*req));
	req->big_handle = big->handle;
	req->adv_handle = padv->handle;
	req->num_bis = big->num_bis;
	sys_put_le24(param->interval, req->sdu_interval);
	req->max_sdu = sys_cpu_to_le16(qos->tx->sdu);
	req->max_latency = sys_cpu_to_le16(param->latency);
	req->rtn = qos->tx->rtn;
	req->phy = qos->tx->phy;
	req->packing = param->packing;
	req->framing = param->framing;
	req->encryption = param->encryption;
	if (req->encryption) {
		memcpy(req->bcode, param->bcode, sizeof(req->bcode));
	} else {
		memset(req->bcode, 0, sizeof(req->bcode));
	}

	bt_hci_cmd_state_set_init(buf, &state, big->flags, BT_BIG_PENDING, true);
	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_CREATE_BIG, buf, NULL);

	if (err) {
		return err;
	}

	for (int i = 0; i < big->num_bis; i++) {
		bt_iso_chan_set_state(big->bis[i], BT_ISO_CONNECT);
	}
	return err;
}

int bt_iso_big_create(struct bt_le_ext_adv *padv, struct bt_iso_big_create_param *param,
		      struct bt_iso_big **out_big)
{
	int err;
	struct bt_iso_big *big;

	if (!atomic_test_bit(padv->flags, BT_PER_ADV_PARAMS_SET)) {
		BT_DBG("PA params not set; invalid adv object");
		return -EINVAL;
	}

	CHECKIF(!param->bis_channels) {
		BT_DBG("NULL BIS channels");
		return -EINVAL;
	}

	CHECKIF(!param->num_bis) {
		BT_DBG("Invalid number of BIS %u", param->num_bis);
		return -EINVAL;
	}

	for (int i = 0; i < param->num_bis; i++) {
		CHECKIF(param->bis_channels[i] == NULL) {
			BT_DBG("NULL channel in bis_channels[%d]", i);
			return -EINVAL;
		}
	}

	CHECKIF(param->framing != BT_ISO_FRAMING_UNFRAMED &&
		param->framing != BT_ISO_FRAMING_FRAMED) {
		BT_DBG("Invalid framing parameter: %u", param->framing);
		return -EINVAL;
	}

	CHECKIF(param->packing != BT_ISO_PACKING_SEQUENTIAL &&
		param->packing != BT_ISO_PACKING_INTERLEAVED) {
		BT_DBG("Invalid packing parameter: %u", param->packing);
		return -EINVAL;
	}

	CHECKIF(param->num_bis > BT_ISO_MAX_GROUP_ISO_COUNT ||
		param->num_bis > CONFIG_BT_ISO_MAX_CHAN) {
		BT_DBG("num_bis (%u) shall be lower than: %u", param->num_bis,
		       MAX(CONFIG_BT_ISO_MAX_CHAN, BT_ISO_MAX_GROUP_ISO_COUNT));
		return -EINVAL;
	}

	CHECKIF(param->interval < BT_ISO_INTERVAL_MIN ||
		param->interval > BT_ISO_INTERVAL_MAX) {
		BT_DBG("Invalid interval: %u", param->interval);
		return -EINVAL;
	}

	CHECKIF(param->latency < BT_ISO_LATENCY_MIN ||
		param->latency > BT_ISO_LATENCY_MAX) {
		BT_DBG("Invalid latency: %u", param->latency);
		return -EINVAL;
	}

	big = get_free_big();

	if (!big) {
		return -ENOMEM;
	}

	big->bis = param->bis_channels;
	big->num_bis = param->num_bis;

	err = big_init_bis(big, true);
	if (err) {
		BT_DBG("Could not init BIG %d", err);
		cleanup_big(big);
		return err;
	}

	err = hci_le_create_big(padv, big, param);
	if (err) {
		BT_DBG("Could not create BIG %d", err);
		cleanup_big(big);
		return err;
	}

	*out_big = big;

	return err;
}

static int hci_le_terminate_big(struct bt_iso_big *big)
{
	struct bt_hci_cp_le_terminate_big *req;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_TERMINATE_BIG, sizeof(*req));
	if (!buf) {
		return -ENOBUFS;
	}

	req = net_buf_add(buf, sizeof(*req));
	req->big_handle = big->handle;
	req->reason = BT_HCI_ERR_REMOTE_USER_TERM_CONN;

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_TERMINATE_BIG, buf, NULL);
}

static int hci_le_big_sync_term(struct bt_iso_big *big)
{
	struct bt_hci_cp_le_big_terminate_sync *req;
	struct bt_hci_rp_le_big_terminate_sync *evt;
	struct net_buf *buf;
	struct net_buf *rsp;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_BIG_TERMINATE_SYNC, sizeof(*req));
	if (!buf) {
		return -ENOBUFS;
	}

	req = net_buf_add(buf, sizeof(*req));
	req->big_handle = big->handle;
	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_BIG_TERMINATE_SYNC, buf, &rsp);
	if (err) {
		return err;
	}

	evt = (struct bt_hci_rp_le_big_terminate_sync *)rsp->data;
	if (evt->status || (evt->big_handle != big->handle)) {
		err = -EIO;
	}

	net_buf_unref(rsp);

	return err;
}

int bt_iso_big_terminate(struct bt_iso_big *big)
{
	int err;
	bool broadcaster;

	if (!atomic_test_bit(big->flags, BT_BIG_INITIALIZED) || !big->num_bis || !big->bis) {
		BT_DBG("BIG not initialized");
		return -EINVAL;
	}

	for (int i = 0; i < big->num_bis; i++) {
		if (!big->bis[i]) {
			BT_DBG("BIG BIS[%d] not initialized", i);
			return -EINVAL;
		}
	}

	/* They all have the same QOS dir so we can just check the first */
	broadcaster = big->bis[0]->qos->tx ? true : false;

	if (broadcaster) {
		err = hci_le_terminate_big(big);

		/* Wait for BT_HCI_EVT_LE_BIG_TERMINATE before cleaning up
		 * the BIG in hci_le_big_terminate
		 */
		if (!err) {
			for (int i = 0; i < big->num_bis; i++) {
				bt_iso_chan_set_state(big->bis[i], BT_ISO_DISCONNECT);
			}
		}
	} else {
		err = hci_le_big_sync_term(big);

		if (!err) {
			big_disconnect(big, BT_HCI_ERR_LOCALHOST_TERM_CONN);
			cleanup_big(big);
		}
	}

	if (err) {
		BT_DBG("Could not terminate BIG %d", err);
	}

	return err;
}

void hci_le_big_complete(struct net_buf *buf)
{
	struct bt_hci_evt_le_big_complete *evt = (void *)buf->data;
	struct bt_iso_big *big;

	if (evt->big_handle >= ARRAY_SIZE(bigs)) {
		BT_WARN("Invalid BIG handle");

		big = big_lookup_flag(BT_BIG_PENDING);
		if (big) {
			big_disconnect(big, evt->status ? evt->status : BT_HCI_ERR_UNSPECIFIED);
			cleanup_big(big);
		}

		return;
	}

	big = &bigs[evt->big_handle];
	atomic_clear_bit(big->flags, BT_BIG_PENDING);

	BT_DBG("BIG[%u] %p completed, status %u", big->handle, big, evt->status);

	if (evt->status || evt->num_bis != big->num_bis) {
		if (evt->status == BT_HCI_ERR_SUCCESS && evt->num_bis != big->num_bis) {
			BT_ERR("Invalid number of BIS created, was %u expected %u",
			       evt->num_bis, big->num_bis);
		}
		big_disconnect(big, evt->status ? evt->status : BT_HCI_ERR_UNSPECIFIED);
		cleanup_big(big);
		return;
	}

	for (int i = 0; i < big->num_bis; i++) {
		struct bt_iso_chan *bis = big->bis[i];

		bis->iso->handle = sys_le16_to_cpu(evt->handle[i]);
		bt_conn_set_state(bis->iso, BT_CONN_CONNECTED);
	}
}

void hci_le_big_terminate(struct net_buf *buf)
{
	struct bt_hci_evt_le_big_terminate *evt = (void *)buf->data;
	struct bt_iso_big *big;

	if (evt->big_handle >= ARRAY_SIZE(bigs)) {
		BT_WARN("Invalid BIG handle");
		return;
	}

	big = &bigs[evt->big_handle];

	BT_DBG("BIG[%u] %p terminated", big->handle, big);

	big_disconnect(big, evt->reason);
	cleanup_big(big);
}

void hci_le_big_sync_established(struct net_buf *buf)
{
	struct bt_hci_evt_le_big_sync_established *evt = (void *)buf->data;
	struct bt_iso_big *big;

	if (evt->big_handle >= ARRAY_SIZE(bigs)) {
		BT_WARN("Invalid BIG handle");
		big = big_lookup_flag(BT_BIG_SYNCING);
		if (big) {
			big_disconnect(big, evt->status ? evt->status : BT_HCI_ERR_UNSPECIFIED);
			cleanup_big(big);
		}

		return;
	}

	big = &bigs[evt->big_handle];
	atomic_clear_bit(big->flags, BT_BIG_SYNCING);

	BT_DBG("BIG[%u] %p sync established, status %u", big->handle, big, evt->status);

	if (evt->status || evt->num_bis != big->num_bis) {
		if (evt->status == BT_HCI_ERR_SUCCESS && evt->num_bis != big->num_bis) {
			BT_ERR("Invalid number of BIS synced, was %u expected %u",
			       evt->num_bis, big->num_bis);
		}
		big_disconnect(big, evt->status ? evt->status : BT_HCI_ERR_UNSPECIFIED);
		cleanup_big(big);
		return;
	}

	for (int i = 0; i < big->num_bis; i++) {
		struct bt_iso_chan *bis = big->bis[i];
		uint16_t bis_handle = sys_le16_to_cpu(evt->handle[i]);

		bis->iso->handle = bis_handle;

		bt_conn_set_state(bis->iso, BT_CONN_CONNECTED);
	}

	/* TODO: Deal with the rest of the fields in the event,
	 * if it makes sense
	 */
}

void hci_le_big_sync_lost(struct net_buf *buf)
{
	struct bt_hci_evt_le_big_sync_lost *evt = (void *)buf->data;
	struct bt_iso_big *big;

	if (evt->big_handle >= ARRAY_SIZE(bigs)) {
		BT_WARN("Invalid BIG handle");
		return;
	}

	big = &bigs[evt->big_handle];

	BT_DBG("BIG[%u] %p sync lost", big->handle, big);

	big_disconnect(big, evt->reason);
	cleanup_big(big);
}

static int hci_le_big_create_sync(const struct bt_le_per_adv_sync *sync, struct bt_iso_big *big,
				  const struct bt_iso_big_sync_param *param)
{
	struct bt_hci_cp_le_big_create_sync *req;
	struct bt_hci_cmd_state_set state;
	struct net_buf *buf;
	int err;
	uint8_t bit_idx = 0;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_BIG_CREATE_SYNC, sizeof(*req) + big->num_bis);
	if (!buf) {
		return -ENOBUFS;
	}

	req = net_buf_add(buf, sizeof(*req) + big->num_bis);
	req->big_handle = big->handle;
	req->sync_handle = sys_cpu_to_le16(sync->handle);
	req->encryption = param->encryption;
	if (req->encryption) {
		memcpy(req->bcode, param->bcode, sizeof(req->bcode));
	} else {
		memset(req->bcode, 0, sizeof(req->bcode));
	}
	req->mse = param->mse;
	req->sync_timeout = sys_cpu_to_le16(param->sync_timeout);
	req->num_bis = big->num_bis;
	/* Transform from bitfield to array */
	for (int i = 1; i <= BT_ISO_MAX_GROUP_ISO_COUNT; i++) {
		if (param->bis_bitfield & BIT(i)) {
			if (bit_idx == big->num_bis) {
				BT_DBG("BIG cannot contain %u BISes", bit_idx + 1);
				return -EINVAL;
			}
			req->bis[bit_idx++] = i;
		}
	}

	if (bit_idx != big->num_bis) {
		BT_DBG("Number of bits in bis_bitfield (%u) doesn't match num_bis (%u)",
		       bit_idx, big->num_bis);
		return -EINVAL;
	}

	bt_hci_cmd_state_set_init(buf, &state, big->flags, BT_BIG_SYNCING, true);
	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_BIG_CREATE_SYNC, buf, NULL);

	return err;
}

int bt_iso_big_sync(struct bt_le_per_adv_sync *sync, struct bt_iso_big_sync_param *param,
		    struct bt_iso_big **out_big)
{
	int err;
	struct bt_iso_big *big;

	if (!atomic_test_bit(sync->flags, BT_PER_ADV_SYNC_SYNCED)) {
		BT_DBG("PA sync not synced");
		return -EINVAL;
	}

	CHECKIF(param->mse > BT_ISO_SYNC_MSE_MAX) {
		BT_DBG("Invalid MSE 0x%02x", param->mse);
		return -EINVAL;
	}

	CHECKIF(param->sync_timeout < BT_ISO_SYNC_TIMEOUT_MIN ||
		param->sync_timeout > BT_ISO_SYNC_TIMEOUT_MAX) {
		BT_DBG("Invalid sync timeout 0x%04x", param->sync_timeout);
		return -EINVAL;
	}

	CHECKIF(param->bis_bitfield <= BIT(0)) {
		BT_DBG("Invalid BIS bitfield 0x%08x", param->bis_bitfield);
		return -EINVAL;
	}

	CHECKIF(!param->bis_channels) {
		BT_DBG("NULL BIS channels");
		return -EINVAL;
	}

	CHECKIF(!param->num_bis) {
		BT_DBG("Invalid number of BIS %u", param->num_bis);
		return -EINVAL;
	}

	for (int i = 0; i < param->num_bis; i++) {
		if (param->bis_channels[i] == NULL) {
			BT_DBG("NULL channel in bis_channels[%d]", i);
			return -EINVAL;
		}
	}

	big = get_free_big();

	if (!big) {
		return -ENOMEM;
	}

	big->bis = param->bis_channels;
	big->num_bis = param->num_bis;

	err = big_init_bis(big, false);
	if (err) {
		BT_DBG("Could not init BIG %d", err);
		cleanup_big(big);
		return err;
	}

	err = hci_le_big_create_sync(sync, big, param);
	if (err) {
		BT_DBG("Could not create BIG sync %d", err);
		cleanup_big(big);
		return err;
	}

	for (int i = 0; i < big->num_bis; i++) {
		bt_iso_chan_set_state(big->bis[i], BT_ISO_CONNECT);
	}

	*out_big = big;

	return 0;
}
#endif /* defined(CONFIG_BT_ISO_BROADCAST) */
