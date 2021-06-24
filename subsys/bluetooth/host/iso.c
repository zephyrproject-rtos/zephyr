/*  Bluetooth ISO */

/*
 * Copyright (c) 2020 Intel Corporation
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

#if CONFIG_BT_ISO_TX_FRAG_COUNT > 0
NET_BUF_POOL_FIXED_DEFINE(iso_frag_pool, CONFIG_BT_ISO_TX_FRAG_COUNT,
			  CONFIG_BT_ISO_TX_MTU, NULL);
#endif /* CONFIG_BT_ISO_TX_FRAG_COUNT > 0 */

struct bt_conn iso_conns[CONFIG_BT_ISO_MAX_CHAN];

/* TODO: Allow more than one server? */
#if defined(CONFIG_BT_ISO_UNICAST)
static struct bt_iso_server *iso_server;
#endif /* CONFIG_BT_ISO_UNICAST */
#if defined(CONFIG_BT_ISO_BROADCAST)
struct bt_iso_big bigs[CONFIG_BT_ISO_MAX_BIG];
#endif /* defined(CONFIG_BT_ISO_BROADCAST) */

/* Prototype */
int hci_le_remove_cig(uint8_t cig_id);
int bt_iso_chan_unbind(struct bt_iso_chan *chan);

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

void hci_iso(struct net_buf *buf)
{
	struct bt_hci_iso_hdr *hdr;
	uint16_t handle, len;
	struct bt_conn *conn;
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

	conn = bt_conn_lookup_handle(iso(buf)->handle);
	if (!conn) {
		BT_ERR("Unable to find conn for handle %u", iso(buf)->handle);
		net_buf_unref(buf);
		return;
	}

	iso(buf)->index = bt_conn_index(conn);

	bt_conn_recv(conn, buf, flags);
	bt_conn_unref(conn);
}

struct bt_conn *iso_new(void)
{
	struct bt_conn *iso = bt_conn_new(iso_conns, ARRAY_SIZE(iso_conns));

	if (iso) {
		iso->type = BT_CONN_TYPE_ISO;
		sys_slist_init(&iso->channels);
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


static int hci_le_setup_iso_data_path(struct bt_conn *conn,
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
	cp->handle = sys_cpu_to_le16(conn->handle);
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
	if (rp->status || (sys_le16_to_cpu(rp->handle) != conn->handle)) {
		err = -EIO;
	}

	net_buf_unref(rsp);

	return err;
}

static int hci_le_remove_iso_data_path(struct bt_conn *conn, uint8_t dir)
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
	cp->handle = conn->handle;
	cp->path_dir = dir;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_REMOVE_ISO_PATH, buf, &rsp);
	if (err) {
		return err;
	}

	rp = (void *)rsp->data;
	if (rp->status || (sys_le16_to_cpu(rp->handle) != conn->handle)) {
		err = -EIO;
	}

	net_buf_unref(rsp);

	return err;
}

static void bt_iso_chan_add(struct bt_conn *conn, struct bt_iso_chan *chan)
{
	/* Attach channel to the connection */
	sys_slist_append(&conn->channels, &chan->node);
	chan->conn = conn;

	BT_DBG("conn %p chan %p", conn, chan);
}

static int bt_iso_setup_data_path(struct bt_conn *conn)
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

	chan = SYS_SLIST_PEEK_HEAD_CONTAINER(&conn->channels, chan, node);
	if (!chan) {
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

	if (conn->iso.is_bis) {
		/* Only set one data path for BIS as per the spec */
		if (tx_qos) {
			return hci_le_setup_iso_data_path(conn, &in_path);

		} else {
			return hci_le_setup_iso_data_path(conn, &out_path);
		}

	} else {
		/* Setup both directions for CIS*/
		err = hci_le_setup_iso_data_path(conn, &in_path);
		if (err) {
			return err;
		}

		return hci_le_setup_iso_data_path(conn, &out_path);

	}
}

void bt_iso_connected(struct bt_conn *conn)
{
	struct bt_iso_chan *chan;

	CHECKIF(!conn || conn->type != BT_CONN_TYPE_ISO) {
		BT_DBG("Invalid parameters: conn %p conn->type %u", conn,
		       conn ? conn->type : 0);
		return;
	}

	BT_DBG("%p", conn);

	if (bt_iso_setup_data_path(conn)) {
		BT_ERR("Unable to setup data path");
		if (conn->iso.is_bis && IS_ENABLED(CONFIG_BT_CONN)) {
			bt_conn_disconnect(conn,
					   BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		}
		/* TODO: Handle BIG terminate for BIS */
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&conn->channels, chan, node) {
		bt_iso_chan_set_state(chan, BT_ISO_CONNECTED);

		if (chan->ops->connected) {
			chan->ops->connected(chan);
		}
	}
}

void bt_iso_remove_data_path(struct bt_conn *conn)
{
	BT_DBG("%p", conn);

	if (conn->iso.is_bis) {
		struct bt_iso_chan *chan;
		struct bt_iso_chan_io_qos *tx_qos;
		uint8_t dir;

		chan = SYS_SLIST_PEEK_HEAD_CONTAINER(&conn->channels, chan, node);
		if (!chan) {
			return;
		}

		tx_qos = chan->qos->tx;

		/* Only remove one data path for BIS as per the spec */
		if (tx_qos) {
			dir = BT_HCI_DATAPATH_DIR_HOST_TO_CTLR;
		} else {
			dir = BT_HCI_DATAPATH_DIR_CTLR_TO_HOST;
		}

		(void)hci_le_remove_iso_data_path(conn, dir);
	} else {
		/* Remove both directions for CIS*/

		/* TODO: Check which has been setup first to avoid removing
		 * data paths that are not setup
		 */
		(void)hci_le_remove_iso_data_path(conn,
						  BT_HCI_DATAPATH_DIR_CTLR_TO_HOST);
		(void)hci_le_remove_iso_data_path(conn,
						  BT_HCI_DATAPATH_DIR_HOST_TO_CTLR);
	}
}

static void bt_iso_chan_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	BT_DBG("%p, reason 0x%02x", chan, reason);

	if (!chan->conn) {
		bt_iso_chan_set_state(chan, BT_ISO_DISCONNECTED);
		return;
	}

	if (chan->conn->iso.is_bis) {
		bt_iso_chan_set_state(chan, BT_ISO_DISCONNECTED);
	} else if (IS_ENABLED(CONFIG_BT_ISO_UNICAST)) {
		bt_iso_chan_set_state(chan, BT_ISO_BOUND);

		/* Unbind if acting as slave or ACL has been disconnected */
		if (chan->conn->role == BT_HCI_ROLE_SLAVE ||
		    chan->conn->iso.acl->state == BT_CONN_DISCONNECTED) {
			bt_iso_chan_unbind(chan);
		}
	} else {
		BT_ERR("Invalid ISO channel");
		return;
	}

	if (chan->ops->disconnected) {
		chan->ops->disconnected(chan, reason);
	}
}

void bt_iso_disconnected(struct bt_conn *conn)
{
	struct bt_iso_chan *chan, *next;

	CHECKIF(!conn || conn->type != BT_CONN_TYPE_ISO) {
		BT_DBG("Invalid parameters: conn %p conn->type %u", conn,
		       conn ? conn->type : 0);
		return;
	}

	BT_DBG("%p", conn);

	if (sys_slist_is_empty(&conn->channels)) {
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&conn->channels, chan, next, node) {
		bt_iso_chan_disconnected(chan, conn->err);
	}
}

#if defined(CONFIG_BT_DEBUG_ISO)
const char *bt_iso_chan_state_str(uint8_t state)
{
	switch (state) {
	case BT_ISO_DISCONNECTED:
		return "disconnected";
	case BT_ISO_BOUND:
		return "bound";
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
	BT_DBG("chan %p conn %p %s -> %s", chan, chan->conn,
	       bt_iso_chan_state_str(chan->state),
	       bt_iso_chan_state_str(state));

	/* check transitions validness */
	switch (state) {
	case BT_ISO_DISCONNECTED:
	case BT_ISO_BOUND:
		/* regardless of old state always allows these states */
		break;
	case BT_ISO_CONNECT:
		if (chan->state != BT_ISO_BOUND) {
			BT_WARN("%s()%d: invalid transition", func, line);
		}
		break;
	case BT_ISO_CONNECTED:
		if (chan->state != BT_ISO_BOUND &&
		    chan->state != BT_ISO_CONNECT) {
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

bool bt_iso_chan_remove(struct bt_conn *conn, struct bt_iso_chan *chan)
{
	return sys_slist_find_and_remove(&conn->channels, &chan->node);
}

void bt_iso_recv(struct bt_conn *conn, struct net_buf *buf, uint8_t flags)
{
	struct bt_hci_iso_data_hdr *hdr;
	struct bt_iso_chan *chan;
	uint8_t pb, ts;
	uint16_t len, pkt_seq_no;

	pb = bt_iso_flags_pb(flags);
	ts = bt_iso_flags_ts(flags);

	BT_DBG("handle %u len %u flags 0x%02x pb 0x%02x ts 0x%02x",
	       conn->handle, buf->len, flags, pb, ts);

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

		if (conn->rx) {
			BT_ERR("Unexpected ISO %s fragment",
			       pb == BT_ISO_START ? "Start" : "Single");
			bt_conn_reset_rx_state(conn);
		}

		conn->rx = buf;
		conn->rx_len = len - buf->len;
		if (conn->rx_len) {
			/* if conn->rx_len then package is longer than the
			 * buf->len and cannot fit in a SINGLE package
			 */
			if (pb == BT_ISO_SINGLE) {
				BT_ERR("Unexpected ISO single fragment");
				bt_conn_reset_rx_state(conn);
			}
			return;
		}
		break;

	case BT_ISO_CONT:
		/* The ISO_Data_Load field contains a continuation fragment of
		 * an SDU.
		 */
		if (!conn->rx) {
			BT_ERR("Unexpected ISO continuation fragment");
			net_buf_unref(buf);
			return;
		}

		BT_DBG("Cont, len %u rx_len %u", buf->len, conn->rx_len);

		if (buf->len > net_buf_tailroom(conn->rx)) {
			BT_ERR("Not enough buffer space for ISO data");
			bt_conn_reset_rx_state(conn);
			net_buf_unref(buf);
			return;
		}

		net_buf_add_mem(conn->rx, buf->data, buf->len);
		conn->rx_len -= buf->len;
		net_buf_unref(buf);
		return;

	case BT_ISO_END:
		/* The ISO_Data_Load field contains the last fragment of an
		 * SDU.
		 */
		BT_DBG("End, len %u rx_len %u", buf->len, conn->rx_len);

		if (!conn->rx) {
			BT_ERR("Unexpected ISO end fragment");
			net_buf_unref(buf);
			return;
		}

		if (buf->len > net_buf_tailroom(conn->rx)) {
			BT_ERR("Not enough buffer space for ISO data");
			bt_conn_reset_rx_state(conn);
			net_buf_unref(buf);
			return;
		}

		net_buf_add_mem(conn->rx, buf->data, buf->len);
		conn->rx_len -= buf->len;
		net_buf_unref(buf);

		break;
	default:
		BT_ERR("Unexpected ISO pb flags (0x%02x)", pb);
		bt_conn_reset_rx_state(conn);
		net_buf_unref(buf);
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&conn->channels, chan, node) {
		if (chan->ops->recv) {
			chan->ops->recv(chan, iso_info(conn->rx), conn->rx);
		}
	}

	bt_conn_reset_rx_state(conn);
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

	if (!chan->conn) {
		BT_DBG("Not connected");
		return -ENOTCONN;
	}

	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->sn = sys_cpu_to_le16(sn++);
	hdr->slen = sys_cpu_to_le16(bt_iso_pkt_len_pack(net_buf_frags_len(buf)
							- sizeof(*hdr),
							BT_ISO_DATA_VALID));

	return bt_conn_send(chan->conn, buf);
}

struct bt_conn_iso *bt_conn_iso(struct bt_conn *conn)
{
	CHECKIF(!conn || conn->type != BT_CONN_TYPE_ISO) {
		BT_DBG("Invalid parameters: conn %p conn->type %u", conn,
		       conn ? conn->type : 0);
		return NULL;
	}

	return &conn->iso;
}

#if defined(CONFIG_BT_ISO_UNICAST)
void bt_iso_cleanup(struct bt_conn *conn)
{
	struct bt_conn_iso *iso = bt_conn_iso(conn);
	int i;

	BT_DBG("%p", conn);

	if (iso->acl) {
		bt_conn_unref(iso->acl);
		iso->acl = NULL;

		if (conn->role == BT_CONN_ROLE_SLAVE) {
			return;
		}

		/* Check if conn is last of CIG */
		for (i = 0; i < CONFIG_BT_ISO_MAX_CHAN; i++) {
			if (conn == &iso_conns[i]) {
				continue;
			}

			if (atomic_get(&iso_conns[i].ref) &&
			    iso_conns[i].iso.cig_id == conn->iso.cig_id) {
				break;
			}
		}

		if (i == CONFIG_BT_ISO_MAX_CHAN) {
			hci_le_remove_cig(conn->iso.cig_id);
		}
	}
}

void hci_le_cis_estabilished(struct net_buf *buf)
{
	struct bt_hci_evt_le_cis_established *evt = (void *)buf->data;
	uint16_t handle = sys_le16_to_cpu(evt->conn_handle);
	struct bt_conn *conn;

	BT_DBG("status %u handle %u", evt->status, handle);

	/* ISO connection handles are already assigned at this point */
	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("No connection found for handle %u", handle);
		return;
	}

	CHECKIF(conn->type != BT_CONN_TYPE_ISO) {
		BT_DBG("Invalid connection type %u", conn->type);
		return;
	}

	if (!evt->status) {
		/* TODO: Add CIG sync delay */
		bt_conn_set_state(conn, BT_CONN_CONNECTED);
		bt_conn_unref(conn);
		return;
	}

	conn->err = evt->status;
	bt_iso_disconnected(conn);
	bt_conn_unref(conn);
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
	struct bt_conn *conn, *iso;
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
	conn = bt_conn_lookup_handle(acl_handle);
	if (!conn) {
		BT_ERR("Invalid ACL handle %u", acl_handle);
		hci_le_reject_cis(cis_handle, BT_HCI_ERR_UNKNOWN_CONN_ID);
		return;
	}

	/* Add ISO connection */
	iso = bt_conn_add_iso(conn);

	bt_conn_unref(conn);

	if (!iso) {
		BT_ERR("Could not create and add ISO to conn %u", acl_handle);
		hci_le_reject_cis(cis_handle,
				  BT_HCI_ERR_INSUFFICIENT_RESOURCES);
		return;
	}

	iso->iso.cig_id = evt->cig_id;
	iso->iso.cis_id = evt->cis_id;

	/* Request application to accept */
	err = bt_iso_accept(iso);
	if (err) {
		BT_DBG("App rejected ISO %d", err);
		bt_conn_unref(iso);
		hci_le_reject_cis(cis_handle,
				  BT_HCI_ERR_INSUFFICIENT_RESOURCES);
		return;
	}

	iso->handle = cis_handle;
	iso->role = BT_HCI_ROLE_SLAVE;
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
	struct bt_conn *conn = iso_new();

	if (!conn) {
		BT_ERR("Unable to allocate connection");
		return NULL;
	}

	conn->iso.acl = bt_conn_ref(acl);

	return conn;
}

static struct net_buf *hci_le_set_cig_params(struct bt_iso_create_param *param)
{
	struct bt_hci_cis_params *cis;
	struct bt_hci_cp_le_set_cig_params *req;
	struct net_buf *buf;
	struct net_buf *rsp;
	int i, err;

	if (!param->chans[0]->qos->tx && !param->chans[0]->qos->rx) {
		BT_ERR("Both TX and RX QoS are disabled");
		return NULL;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_CIG_PARAMS,
				sizeof(*req) + sizeof(*cis) * param->num_conns);
	if (!buf) {
		return NULL;
	}

	req = net_buf_add(buf, sizeof(*req));

	memset(req, 0, sizeof(*req));

	req->cig_id = param->conns[0]->iso.cig_id;
	if (param->chans[0]->qos->tx) {
		sys_put_le24(param->chans[0]->qos->tx->interval, req->m_interval);
		req->m_latency = sys_cpu_to_le16(param->chans[0]->qos->tx->latency);
	} else {
		/* Use RX values if TX is disabled.
		 * If TX is disabled then the values don't have any meaning, as
		 * we will create a new CIG in case we want to change the
		 * directions, but will need to be set to the fields to some
		 * valid (nonzero) values for the controller to accept them.
		 */
		sys_put_le24(param->chans[0]->qos->rx->interval, req->m_interval);
		req->m_latency = sys_cpu_to_le16(param->chans[0]->qos->rx->latency);
	}

	if (param->chans[0]->qos->rx) {
		sys_put_le24(param->chans[0]->qos->rx->interval, req->s_interval);
		req->s_latency = sys_cpu_to_le16(param->chans[0]->qos->rx->latency);
	} else {
		/* Use TX values if RX is disabled.
		 * If RX is disabled then the values don't have any meaning, as
		 * we will create a new CIG in case we want to change the
		 * directions, but will need to be set to the fields to some
		 * valid (nonzero) values for the controller to accept them.
		 */
		sys_put_le24(param->chans[0]->qos->tx->interval, req->s_interval);
		req->s_latency = sys_cpu_to_le16(param->chans[0]->qos->tx->latency);
	}

	req->sca = param->chans[0]->qos->sca;
	req->packing = param->chans[0]->qos->packing;
	req->framing = param->chans[0]->qos->framing;
	req->num_cis = param->num_conns;

	/* Program the cis parameters */
	for (i = 0; i < param->num_conns; i++) {
		struct bt_iso_chan_qos *qos = param->chans[i]->qos;

		cis = net_buf_add(buf, sizeof(*cis));

		memset(cis, 0, sizeof(*cis));

		cis->cis_id = param->conns[i]->iso.cis_id;

		if (!qos->tx && !qos->rx) {
			BT_ERR("Both TX and RX QoS are disabled");
			net_buf_unref(buf);
			return NULL;
		}

		if (!qos->tx) {
			/* Use RX PHY if TX is not set (disabled) */
			cis->m_phy = qos->rx->phy;
		} else {
			cis->m_sdu = sys_cpu_to_le16(qos->tx->sdu);
			cis->m_phy = qos->tx->phy;
			cis->m_rtn = qos->tx->rtn;
		}

		if (!qos->rx) {
			/* Use TX PHY if RX is not set (disabled) */
			cis->s_phy = qos->tx->phy;
		} else {
			cis->s_sdu = sys_cpu_to_le16(qos->rx->sdu);
			cis->s_phy = qos->rx->phy;
			cis->s_rtn = qos->rx->rtn;
		}
	}

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_CIG_PARAMS, buf, &rsp);
	if (err) {
		return NULL;
	}

	return rsp;
}

int bt_conn_bind_iso(struct bt_iso_create_param *param)
{
	struct bt_conn *conn;
	struct net_buf *rsp;
	struct bt_hci_rp_le_set_cig_params *cig_rsp;
	int i, err;

	/* Check if controller is ISO capable */
	if (!BT_FEAT_LE_CIS_MASTER(bt_dev.le.features)) {
		return -ENOTSUP;
	}

	if (!param->num_conns || param->num_conns > CONFIG_BT_ISO_MAX_CHAN) {
		return -EINVAL;
	}

	/* Assign ISO connections to each LE connection */
	for (i = 0; i < param->num_conns; i++) {
		conn = param->conns[i];

		if (conn->type != BT_CONN_TYPE_LE) {
			err = -EINVAL;
			goto failed;
		}

		conn = bt_conn_add_iso(conn);
		if (!conn) {
			err = -ENOMEM;
			goto failed;
		}

		conn->iso.cig_id = param->id;
		conn->iso.cis_id = bt_conn_index(conn);

		param->conns[i] = conn;
	}

	rsp = hci_le_set_cig_params(param);
	if (!rsp) {
		err = -EIO;
		goto failed;
	}

	cig_rsp = (void *)rsp->data;

	if (rsp->len < sizeof(cig_rsp) ||
	    cig_rsp->num_handles != param->num_conns) {
		BT_WARN("Unexpected response to hci_le_set_cig_params");
		err = -EIO;
		net_buf_unref(rsp);
		goto failed;
	}

	for (i = 0; i < cig_rsp->num_handles; i++) {
		/* Assign the connection handle */
		param->conns[i]->handle = cig_rsp->handle[i];
	}

	net_buf_unref(rsp);

	return 0;

failed:
	for (i = 0; i < param->num_conns; i++) {
		conn = param->conns[i];

		if (conn->type == BT_CONN_TYPE_ISO) {
			bt_iso_cleanup(conn);
		}
	}

	return err;
}

static int hci_le_create_cis(struct bt_conn **conn, uint8_t num_conns)
{
	struct bt_hci_cis *cis;
	struct bt_hci_cp_le_create_cis *req;
	struct net_buf *buf;
	int i;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_CREATE_CIS,
				sizeof(*req) + sizeof(*cis) * num_conns);
	if (!buf) {
		return -ENOBUFS;
	}

	req = net_buf_add(buf, sizeof(*req));

	memset(req, 0, sizeof(*req));

	req->num_cis = num_conns;

	/* Program the cis parameters */
	for (i = 0; i < num_conns; i++) {
		cis = net_buf_add(buf, sizeof(*cis));

		memset(cis, 0, sizeof(*cis));

		cis->cis_handle = sys_cpu_to_le16(conn[i]->handle);
		cis->acl_handle = sys_cpu_to_le16(conn[i]->iso.acl->handle);
	}

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_CREATE_CIS, buf, NULL);
}

int bt_conn_connect_iso(struct bt_conn **conns, uint8_t num_conns)
{
	int i, err;

	/* Check if controller is ISO capable */
	if (!BT_FEAT_LE_CIS_MASTER(bt_dev.le.features)) {
		return -ENOTSUP;
	}

	if (num_conns > CONFIG_BT_ISO_MAX_CHAN) {
		return -EINVAL;
	}

	for (i = 0; i < num_conns; i++) {
		if (conns[i]->type != BT_CONN_TYPE_ISO) {
			return -EINVAL;
		}
	}

	err = hci_le_create_cis(conns, num_conns);
	if (err) {
		return err;
	}

	/* Set connection state */
	for (i = 0; i < num_conns; i++) {
		bt_conn_set_state(conns[i], BT_CONN_CONNECT);
	}

	return 0;
}

int bt_iso_accept(struct bt_conn *conn)
{
	struct bt_iso_chan *chan;
	int err;

	CHECKIF(!conn || conn->type != BT_CONN_TYPE_ISO) {
		BT_DBG("Invalid parameters: conn %p conn->type %u", conn,
		       conn ? conn->type : 0);
		return -EINVAL;
	}

	BT_DBG("%p", conn);

	if (!iso_server) {
		return -ENOMEM;
	}

	err = iso_server->accept(conn, &chan);
	if (err < 0) {
		BT_ERR("err %d", err);
		return err;
	}

	bt_iso_chan_add(conn, chan);
	bt_iso_chan_set_state(chan, BT_ISO_BOUND);

	return 0;
}

int bt_iso_chan_connect(struct bt_iso_chan **chans, uint8_t num_chans)
{
	struct bt_conn *conns[CONFIG_BT_ISO_MAX_CHAN];
	int i, err;

	CHECKIF(!chans || !num_chans) {
		BT_DBG("Invalid parameters: chans %p num_chans %u", chans,
		       num_chans);
		return -EINVAL;
	}

	for (i = 0; i < num_chans; i++) {
		if (!chans[i]->conn) {
			return -ENOTCONN;
		}

		conns[i] = chans[i]->conn;
	}

	err = bt_conn_connect_iso(conns, num_chans);
	if (err) {
		return err;
	}

	for (i = 0; i < num_chans; i++) {
		bt_iso_chan_set_state(chans[i], BT_ISO_CONNECT);
	}

	return 0;
}

int bt_iso_chan_disconnect(struct bt_iso_chan *chan)
{
	CHECKIF(!chan) {
		BT_DBG("Invalid parameter: chan %p", chan);
		return -EINVAL;
	}

	if (!chan->conn) {
		return -ENOTCONN;
	}

	if (chan->state == BT_ISO_BOUND) {
		bt_iso_chan_disconnected(chan, BT_HCI_ERR_LOCALHOST_TERM_CONN);
		return 0;
	}

	return bt_conn_disconnect(chan->conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

int bt_iso_server_register(struct bt_iso_server *server)
{
	CHECKIF(!server) {
		BT_DBG("Invalid parameter: server %p", server);
		return -EINVAL;
	}

	/* Check if controller is ISO capable */
	if (!BT_FEAT_LE_CIS_SLAVE(bt_dev.le.features)) {
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

int bt_iso_chan_bind(struct bt_conn **conns, uint8_t num_conns,
		     struct bt_iso_chan **chans)
{
	struct bt_iso_create_param param;
	int i, err;
	static uint8_t id;

	CHECKIF(!conns || !num_conns || !chans) {
		BT_DBG("Invalid parameters: conns %p num_conns %u chans %p",
		       conns, num_conns, chans);
		return -EINVAL;
	}

	memset(&param, 0, sizeof(param));

	param.id = id++;
	param.num_conns = num_conns;
	param.conns = conns;
	param.chans = chans;

	err = bt_conn_bind_iso(&param);
	if (err) {
		return err;
	}

	/* Bind respective connection to channel */
	for (i = 0; i < num_conns; i++) {
		bt_iso_chan_add(conns[i], chans[i]);
		bt_iso_chan_set_state(chans[i], BT_ISO_BOUND);
	}

	return 0;
}

int bt_iso_chan_unbind(struct bt_iso_chan *chan)
{
	CHECKIF(!chan) {
		BT_DBG("Invalid parameter: chan %p", chan);
		return -EINVAL;
	}

	if (!chan->conn) {
		return -EINVAL;
	}

	if (!bt_iso_chan_remove(chan->conn, chan)) {
		return -ENOENT;
	}

	bt_conn_unref(chan->conn);
	chan->conn = NULL;

	bt_iso_chan_set_state(chan, BT_ISO_DISCONNECTED);

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

		if (bis->conn) {
			bt_conn_unref(bis->conn);
			bis->conn = NULL;
		}
	}

	memset(big, 0, sizeof(*big));
}

static void big_disconnect(struct bt_iso_big *big, uint8_t reason)
{
	for (int i = 0; i < big->num_bis; i++) {
		big->bis[i]->conn->err = reason;

		bt_iso_disconnected(big->bis[i]->conn);
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

		if (bis->conn) {
			BT_DBG("BIS conn was already allocated");
			return -EALREADY;
		}

		if (!bis->qos || (!bis->qos->tx && broadcaster)) {
			BT_DBG("BIS QOS was invalid");
			return -EINVAL;
		}

		bis->conn = iso_new();

		if (!bis->conn) {
			BT_ERR("Unable to allocate BIS connection");
			return -ENOMEM;
		}

		bis->conn->iso.big_handle = big->handle;
		bis->conn->iso.is_bis = true;
		bis->conn->iso.bis_id = bt_conn_index(bis->conn);

		bt_iso_chan_add(bis->conn, bis);
		bt_iso_chan_set_state(bis, BT_ISO_BOUND);
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
	sys_put_le24(qos->tx->interval, req->sdu_interval);
	req->max_sdu = sys_cpu_to_le16(qos->tx->sdu);
	req->max_latency = sys_cpu_to_le16(qos->tx->latency);
	req->rtn = qos->tx->rtn;
	req->phy = qos->tx->phy;
	req->packing = qos->packing;
	req->framing = qos->framing;
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

		bis->conn->handle = sys_le16_to_cpu(evt->handle[i]);
		bt_conn_set_state(bis->conn, BT_CONN_CONNECTED);
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

		bis->conn->handle = bis_handle;

		bt_conn_set_state(bis->conn, BT_CONN_CONNECTED);
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
	for (int i = 0; i < 0x1F; i++) {
		if (param->bis_bitfield & BIT(i)) {
			if (bit_idx == big->num_bis) {
				BT_DBG("BIG cannot contain %u BISes", bit_idx + 1);
				return -EINVAL;
			}
			req->bis[bit_idx++] = i + 1; /* indices start from 1 */
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

	CHECKIF(param->mse > 0x1F) {
		BT_DBG("Invalid MSE 0x%02x", param->mse);
		return -EINVAL;
	}

	CHECKIF(param->sync_timeout < 0x000A || param->sync_timeout > 0x4000) {
		BT_DBG("Invalid sync timeout 0x%04x", param->sync_timeout);
		return -EINVAL;
	}

	CHECKIF(!param->bis_bitfield) {
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
