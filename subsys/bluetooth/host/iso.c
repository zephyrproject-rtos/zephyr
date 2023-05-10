/*  Bluetooth ISO */

/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/iso.h>

#include "host/hci_core.h"
#include "host/conn_internal.h"
#include "iso_internal.h"

#include "common/assert.h"

#define LOG_LEVEL CONFIG_BT_ISO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_iso);

#if defined(CONFIG_BT_DEBUG_ISO_DATA)
#define BT_ISO_DATA_DBG(fmt, ...) LOG_DBG(fmt, ##__VA_ARGS__)
#else
#define BT_ISO_DATA_DBG(fmt, ...)
#endif /* CONFIG_BT_DEBUG_ISO_DATA */

#define iso_chan(_iso) ((_iso)->iso.chan);

#if defined(CONFIG_BT_ISO_UNICAST) || defined(CONFIG_BT_ISO_SYNC_RECEIVER)
NET_BUF_POOL_FIXED_DEFINE(iso_rx_pool, CONFIG_BT_ISO_RX_BUF_COUNT,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_RX_MTU), 8, NULL);

static struct bt_iso_recv_info iso_info_data[CONFIG_BT_ISO_RX_BUF_COUNT];
#define iso_info(buf) (&iso_info_data[net_buf_id(buf)])
#endif /* CONFIG_BT_ISO_UNICAST || CONFIG_BT_ISO_SYNC_RECEIVER */

#if defined(CONFIG_BT_ISO_UNICAST) || defined(CONFIG_BT_ISO_BROADCAST)
NET_BUF_POOL_FIXED_DEFINE(iso_tx_pool, CONFIG_BT_ISO_TX_BUF_COUNT,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

#if CONFIG_BT_ISO_TX_FRAG_COUNT > 0
NET_BUF_POOL_FIXED_DEFINE(iso_frag_pool, CONFIG_BT_ISO_TX_FRAG_COUNT,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
#endif /* CONFIG_BT_ISO_TX_FRAG_COUNT > 0 */
#endif /* CONFIG_BT_ISO_UNICAST || CONFIG_BT_ISO_BROADCAST */

struct bt_conn iso_conns[CONFIG_BT_ISO_MAX_CHAN];

/* TODO: Allow more than one server? */
#if defined(CONFIG_BT_ISO_CENTRAL)
struct bt_iso_cig cigs[CONFIG_BT_ISO_MAX_CIG];

static struct bt_iso_cig *get_cig(const struct bt_iso_chan *iso_chan);
static void bt_iso_remove_data_path(struct bt_conn *iso);
static int hci_le_create_cis(const struct bt_iso_connect_param *param,
			     size_t count);

#endif /* CONFIG_BT_ISO_CENTRAL */

#if defined(CONFIG_BT_ISO_PERIPHERAL)
static struct bt_iso_server *iso_server;

static struct bt_conn *bt_conn_add_iso(struct bt_conn *acl);
#endif /* CONFIG_BT_ISO_PERIPHERAL */

#if defined(CONFIG_BT_ISO_BROADCAST)
struct bt_iso_big bigs[CONFIG_BT_ISO_MAX_BIG];

static struct bt_iso_big *lookup_big_by_handle(uint8_t big_handle);
#endif /* CONFIG_BT_ISO_BROADCAST */

#if defined(CONFIG_BT_ISO_UNICAST) || defined(CONFIG_BT_ISO_BROADCASTER)
static void bt_iso_send_cb(struct bt_conn *iso, void *user_data, int err)
{
	struct bt_iso_chan *chan = iso->iso.chan;
	struct bt_iso_chan_ops *ops;

	__ASSERT(chan != NULL, "NULL chan for iso %p", iso);

	ops = chan->ops;

	if (!err && ops != NULL && ops->sent != NULL) {
		ops->sent(chan);
	}
}
#endif /* CONFIG_BT_ISO_UNICAST || CONFIG_BT_ISO_BROADCASTER */


void hci_iso(struct net_buf *buf)
{
	struct bt_hci_iso_hdr *hdr;
	uint16_t handle, len;
	struct bt_conn *iso;
	uint8_t flags;

	BT_ISO_DATA_DBG("buf %p", buf);

	BT_ASSERT(buf->len >= sizeof(*hdr));

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	len = bt_iso_hdr_len(sys_le16_to_cpu(hdr->len));
	handle = sys_le16_to_cpu(hdr->handle);
	flags = bt_iso_flags(handle);

	iso(buf)->handle = bt_iso_handle(handle);
	iso(buf)->index = BT_CONN_INDEX_INVALID;

	BT_ISO_DATA_DBG("handle %u len %u flags %u",
			iso(buf)->handle, len, flags);

	if (buf->len != len) {
		LOG_ERR("ISO data length mismatch (%u != %u)", buf->len, len);
		net_buf_unref(buf);
		return;
	}

	iso = bt_conn_lookup_handle(iso(buf)->handle);
	if (iso == NULL) {
		LOG_ERR("Unable to find conn for handle %u", iso(buf)->handle);
		net_buf_unref(buf);
		return;
	}

	iso(buf)->index = bt_conn_index(iso);

	bt_conn_recv(iso, buf, flags);
	bt_conn_unref(iso);
}

static struct bt_conn *iso_new(void)
{
	struct bt_conn *iso = bt_conn_new(iso_conns, ARRAY_SIZE(iso_conns));

	if (iso) {
		iso->type = BT_CONN_TYPE_ISO;
	} else {
		LOG_DBG("Could not create new ISO");
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

static int hci_le_setup_iso_data_path(const struct bt_conn *iso, uint8_t dir,
				      const struct bt_iso_chan_path *path)
{
	struct bt_hci_cp_le_setup_iso_path *cp;
	struct bt_hci_rp_le_setup_iso_path *rp;
	struct net_buf *buf, *rsp;
	uint8_t *cc;
	int err;

	__ASSERT(dir == BT_HCI_DATAPATH_DIR_HOST_TO_CTLR ||
		 dir == BT_HCI_DATAPATH_DIR_CTLR_TO_HOST,
		 "invalid ISO data path dir: %u", dir);

	if ((path->cc == NULL && path->cc_len != 0)) {
		LOG_DBG("Invalid ISO data path CC: %p %u", path->cc, path->cc_len);

		return -EINVAL;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SETUP_ISO_PATH, sizeof(*cp) + path->cc_len);
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(iso->handle);
	cp->path_dir = dir;
	cp->path_id = path->pid;
	cp->codec_id.coding_format = path->format;
	cp->codec_id.company_id = sys_cpu_to_le16(path->cid);
	cp->codec_id.vs_codec_id = sys_cpu_to_le16(path->vid);
	sys_put_le24(path->delay, cp->controller_delay);
	cp->codec_config_len = path->cc_len;
	cc = net_buf_add(buf, cp->codec_config_len);
	memcpy(cc, path->cc, cp->codec_config_len);

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

static void bt_iso_chan_add(struct bt_conn *iso, struct bt_iso_chan *chan)
{
	/* Attach ISO channel to the connection */
	chan->iso = iso;
	iso->iso.chan = chan;

	LOG_DBG("iso %p chan %p", iso, chan);
}

static int bt_iso_setup_data_path(struct bt_iso_chan *chan)
{
	int err;
	struct bt_iso_chan_path default_hci_path = { .pid = BT_ISO_DATA_PATH_HCI,
						     .format = BT_HCI_CODING_FORMAT_TRANSPARENT,
						     .cc_len = 0x00 };
	struct bt_iso_chan_path *out_path = NULL;
	struct bt_iso_chan_path *in_path = NULL;
	struct bt_iso_chan_io_qos *tx_qos;
	struct bt_iso_chan_io_qos *rx_qos;
	struct bt_conn *iso;
	uint8_t dir;

	iso = chan->iso;

	tx_qos = chan->qos->tx;
	rx_qos = chan->qos->rx;

	/* The following code sets the in and out paths for ISO data.
	 * If the application provides a path for a direction (tx/rx) we use
	 * that, otherwise we simply fall back to HCI.
	 *
	 * If the direction is not set (by whether tx_qos or rx_qos is NULL),
	 * then we fallback to the HCI path object, but we disable the direction
	 * in the controller.
	 */

	if (tx_qos != NULL && iso->iso.info.can_send) {
		if (tx_qos->path != NULL) { /* Use application path */
			in_path = tx_qos->path;
		} else { /* else fallback to HCI path */
			in_path = &default_hci_path;
		}
	}

	if (rx_qos != NULL && iso->iso.info.can_recv) {
		if (rx_qos->path != NULL) { /* Use application path */
			out_path = rx_qos->path;
		} else { /* else fallback to HCI path */
			out_path = &default_hci_path;
		}
	}

	__ASSERT(in_path || out_path,
		 "At least one path shall be shell: in %p out %p",
		 in_path, out_path);

	if (IS_ENABLED(CONFIG_BT_ISO_BROADCASTER) &&
	    iso->iso.info.type == BT_ISO_CHAN_TYPE_BROADCASTER && in_path) {
		dir = BT_HCI_DATAPATH_DIR_HOST_TO_CTLR;
		err = hci_le_setup_iso_data_path(iso, dir, in_path);
		if (err != 0) {
			LOG_DBG("Failed to set broadcaster data path: %d", err);
		}

		return err;
	} else if (IS_ENABLED(CONFIG_BT_ISO_SYNC_RECEIVER) &&
		   iso->iso.info.type == BT_ISO_CHAN_TYPE_SYNC_RECEIVER &&
		   out_path) {
		dir = BT_HCI_DATAPATH_DIR_CTLR_TO_HOST;
		err = hci_le_setup_iso_data_path(iso, dir, out_path);
		if (err != 0) {
			LOG_DBG("Failed to set sync receiver data path: %d", err);
		}

		return err;
	} else if (IS_ENABLED(CONFIG_BT_ISO_UNICAST) &&
		   iso->iso.info.type == BT_ISO_CHAN_TYPE_CONNECTED) {
		if (in_path != NULL) {
			/* Enable TX */
			dir = BT_HCI_DATAPATH_DIR_HOST_TO_CTLR;
			err = hci_le_setup_iso_data_path(iso, dir, in_path);
			if (err) {
				return err;
			}
		}

		if (out_path != NULL) {
			/* Enable RX */
			dir = BT_HCI_DATAPATH_DIR_CTLR_TO_HOST;
			err = hci_le_setup_iso_data_path(iso, dir, out_path);
			if (err) {
				return err;
			}
		}

		return 0;
	} else {
		__ASSERT(false, "Invalid iso.info.type: %u",
			 iso->iso.info.type);
		return -EINVAL;
	}
}

void bt_iso_connected(struct bt_conn *iso)
{
	struct bt_iso_chan *chan;
	int err;

	if (iso == NULL || iso->type != BT_CONN_TYPE_ISO) {
		LOG_DBG("Invalid parameters: iso %p iso->type %u", iso, iso ? iso->type : 0);
		return;
	}

	LOG_DBG("%p", iso);

	chan = iso_chan(iso);
	if (chan == NULL) {
		LOG_ERR("Could not lookup chan from connected ISO");
		return;
	}

	err = bt_iso_setup_data_path(chan);
	if (err != 0) {
		if (false) {

#if defined(CONFIG_BT_ISO_BROADCAST)
		} else if (iso->iso.info.type == BT_ISO_CHAN_TYPE_BROADCASTER ||
			   iso->iso.info.type == BT_ISO_CHAN_TYPE_SYNC_RECEIVER) {
			struct bt_iso_big *big;
			int err;

			big = lookup_big_by_handle(iso->iso.big_handle);

			err = bt_iso_big_terminate(big);
			if (err != 0) {
				LOG_ERR("Could not terminate BIG: %d", err);
			}
#endif /* CONFIG_BT_ISO_BROADCAST */

		} else if (IS_ENABLED(CONFIG_BT_ISO_UNICAST) &&
		    iso->iso.info.type == BT_ISO_CHAN_TYPE_CONNECTED) {
			bt_conn_disconnect(iso,
					   BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		} else {
			__ASSERT(false, "Invalid iso.info.type: %u",
				 iso->iso.info.type);
		}
		return;
	}

	bt_iso_chan_set_state(chan, BT_ISO_STATE_CONNECTED);

	if (chan->ops->connected) {
		chan->ops->connected(chan);
	}
}

static void bt_iso_chan_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	LOG_DBG("%p, reason 0x%02x", chan, reason);

	__ASSERT(chan->iso != NULL, "NULL conn for iso chan %p", chan);

	bt_iso_chan_set_state(chan, BT_ISO_STATE_DISCONNECTED);

	/* The peripheral does not have the concept of a CIG, so once a CIS
	 * disconnects it is completely freed by unref'ing it
	 */
	if (IS_ENABLED(CONFIG_BT_ISO_UNICAST) &&
	    chan->iso->iso.info.type == BT_ISO_CHAN_TYPE_CONNECTED) {
		bt_iso_cleanup_acl(chan->iso);

		if (chan->iso->role == BT_HCI_ROLE_PERIPHERAL) {
			bt_conn_unref(chan->iso);
			chan->iso = NULL;
#if defined(CONFIG_BT_ISO_CENTRAL)
		} else {
			/* ISO data paths are automatically removed when the
			 * peripheral disconnects, so we only need to
			 * move it for the central
			 */
			bt_iso_remove_data_path(chan->iso);
			bool is_chan_connected;
			struct bt_iso_cig *cig;
			struct bt_iso_chan *cis_chan;

			/* Update CIG state */
			cig = get_cig(chan);
			__ASSERT(cig != NULL, "CIG was NULL");

			is_chan_connected = false;
			SYS_SLIST_FOR_EACH_CONTAINER(&cig->cis_channels, cis_chan, node) {
				if (cis_chan->state == BT_ISO_STATE_CONNECTED ||
				    cis_chan->state == BT_ISO_STATE_CONNECTING) {
					is_chan_connected = true;
					break;
				}
			}

			if (!is_chan_connected) {
				cig->state = BT_ISO_CIG_STATE_INACTIVE;
			}
#endif /* CONFIG_BT_ISO_CENTRAL */
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
		LOG_DBG("Invalid parameters: iso %p iso->type %u", iso, iso ? iso->type : 0);
		return;
	}

	LOG_DBG("%p", iso);

	chan = iso_chan(iso);
	if (chan == NULL) {
		LOG_ERR("Could not lookup chan from disconnected ISO");
		return;
	}

	bt_iso_chan_disconnected(chan, iso->err);
}

#if defined(CONFIG_BT_ISO_LOG_LEVEL_DBG)
const char *bt_iso_chan_state_str(uint8_t state)
{
	switch (state) {
	case BT_ISO_STATE_DISCONNECTED:
		return "disconnected";
	case BT_ISO_STATE_CONNECTING:
		return "connecting";
	case BT_ISO_STATE_ENCRYPT_PENDING:
		return "encryption pending";
	case BT_ISO_STATE_CONNECTED:
		return "connected";
	case BT_ISO_STATE_DISCONNECTING:
		return "disconnecting";
	default:
		return "unknown";
	}
}

void bt_iso_chan_set_state_debug(struct bt_iso_chan *chan,
				 enum bt_iso_state state,
				 const char *func, int line)
{
	LOG_DBG("chan %p iso %p %s -> %s", chan, chan->iso, bt_iso_chan_state_str(chan->state),
		bt_iso_chan_state_str(state));

	/* check transitions validness */
	switch (state) {
	case BT_ISO_STATE_DISCONNECTED:
		/* regardless of old state always allows this states */
		break;
	case BT_ISO_STATE_ENCRYPT_PENDING:
		__fallthrough;
	case BT_ISO_STATE_CONNECTING:
		if (chan->state != BT_ISO_STATE_DISCONNECTED) {
			LOG_WRN("%s()%d: invalid transition", func, line);
		}
		break;
	case BT_ISO_STATE_CONNECTED:
		if (chan->state != BT_ISO_STATE_CONNECTING) {
			LOG_WRN("%s()%d: invalid transition", func, line);
		}
		break;
	case BT_ISO_STATE_DISCONNECTING:
		if (chan->state != BT_ISO_STATE_CONNECTING &&
		    chan->state != BT_ISO_STATE_CONNECTED) {
			LOG_WRN("%s()%d: invalid transition", func, line);
		}
		break;
	default:
		LOG_ERR("%s()%d: unknown (%u) state was set", func, line, state);
		return;
	}

	chan->state = state;
}
#else
void bt_iso_chan_set_state(struct bt_iso_chan *chan, enum bt_iso_state state)
{
	chan->state = state;
}
#endif /* CONFIG_BT_ISO_LOG_LEVEL_DBG */

int bt_iso_chan_get_info(const struct bt_iso_chan *chan,
			 struct bt_iso_info *info)
{
	CHECKIF(chan == NULL) {
		LOG_DBG("chan is NULL");
		return -EINVAL;
	}

	CHECKIF(chan->iso == NULL) {
		LOG_DBG("chan->iso is NULL");
		return -EINVAL;
	}

	CHECKIF(info == NULL) {
		LOG_DBG("info is NULL");
		return -EINVAL;
	}

	(void)memcpy(info, &chan->iso->iso.info, sizeof(*info));

	return 0;
}

#if defined(CONFIG_BT_ISO_UNICAST) || defined(CONFIG_BT_ISO_SYNC_RECEIVER)
struct net_buf *bt_iso_get_rx(k_timeout_t timeout)
{
	struct net_buf *buf = net_buf_alloc(&iso_rx_pool, timeout);

	if (buf) {
		net_buf_reserve(buf, BT_BUF_RESERVE);
		bt_buf_set_type(buf, BT_BUF_ISO_IN);
	}

	return buf;
}

void bt_iso_recv(struct bt_conn *iso, struct net_buf *buf, uint8_t flags)
{
	struct bt_hci_iso_data_hdr *hdr;
	struct bt_iso_chan *chan;
	uint8_t pb, ts;
	uint16_t len, pkt_seq_no;

	pb = bt_iso_flags_pb(flags);
	ts = bt_iso_flags_ts(flags);

	BT_ISO_DATA_DBG("handle %u len %u flags 0x%02x pb 0x%02x ts 0x%02x",
			iso->handle, buf->len, flags, pb, ts);

	/* When the PB_Flag does not equal 0b00, the fields Time_Stamp,
	 * Packet_Sequence_Number, Packet_Status_Flag and ISO_SDU_Length
	 * are omitted from the HCI ISO Data packet.
	 */
	switch (pb) {
	case BT_ISO_START:
	case BT_ISO_SINGLE:
		iso_info(buf)->flags = 0;

		/* The ISO_Data_Load field contains either the first fragment
		 * of an SDU or a complete SDU.
		 */
		if (ts) {
			struct bt_hci_iso_ts_data_hdr *ts_hdr;

			ts_hdr = net_buf_pull_mem(buf, sizeof(*ts_hdr));
			iso_info(buf)->ts = sys_le32_to_cpu(ts_hdr->ts);

			hdr = &ts_hdr->data;
			iso_info(buf)->flags |= BT_ISO_FLAGS_TS;
		} else {
			hdr = net_buf_pull_mem(buf, sizeof(*hdr));
			/* TODO: Generate a timestamp? */
			iso_info(buf)->ts = 0x00000000;
		}

		len = sys_le16_to_cpu(hdr->slen);
		flags = bt_iso_pkt_flags(len);
		len = bt_iso_pkt_len(len);
		pkt_seq_no = sys_le16_to_cpu(hdr->sn);
		iso_info(buf)->seq_num = pkt_seq_no;
		if (flags == BT_ISO_DATA_VALID) {
			iso_info(buf)->flags |= BT_ISO_FLAGS_VALID;
		} else if (flags == BT_ISO_DATA_INVALID) {
			iso_info(buf)->flags |= BT_ISO_FLAGS_ERROR;
		} else if (flags == BT_ISO_DATA_NOP) {
			iso_info(buf)->flags |= BT_ISO_FLAGS_LOST;
		} else {
			LOG_WRN("Invalid ISO packet status flag: %u", flags);
			iso_info(buf)->flags = 0;
		}

		BT_ISO_DATA_DBG("%s, len %u total %u flags 0x%02x timestamp %u",
				pb == BT_ISO_START ? "Start" : "Single",
				buf->len, len, flags, iso_info(buf)->ts);

		if (iso->rx) {
			LOG_ERR("Unexpected ISO %s fragment",
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
				LOG_ERR("Unexpected ISO single fragment");
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
			LOG_ERR("Unexpected ISO continuation fragment");
			net_buf_unref(buf);
			return;
		}

		BT_ISO_DATA_DBG("Cont, len %u rx_len %u",
				buf->len, iso->rx_len);

		if (buf->len > net_buf_tailroom(iso->rx)) {
			LOG_ERR("Not enough buffer space for ISO data");
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
		BT_ISO_DATA_DBG("End, len %u rx_len %u", buf->len, iso->rx_len);

		if (iso->rx == NULL) {
			LOG_ERR("Unexpected ISO end fragment");
			net_buf_unref(buf);
			return;
		}

		if (buf->len > net_buf_tailroom(iso->rx)) {
			LOG_ERR("Not enough buffer space for ISO data");
			bt_conn_reset_rx_state(iso);
			net_buf_unref(buf);
			return;
		}

		(void)net_buf_add_mem(iso->rx, buf->data, buf->len);
		iso->rx_len -= buf->len;
		net_buf_unref(buf);

		break;
	default:
		LOG_ERR("Unexpected ISO pb flags (0x%02x)", pb);
		bt_conn_reset_rx_state(iso);
		net_buf_unref(buf);
		return;
	}

	chan = iso_chan(iso);
	if (chan == NULL) {
		LOG_ERR("Could not lookup chan from receiving ISO");
	} else if (chan->ops->recv != NULL) {
		chan->ops->recv(chan, iso_info(iso->rx), iso->rx);
	}

	bt_conn_reset_rx_state(iso);
}
#endif /* CONFIG_BT_ISO_UNICAST) || defined(CONFIG_BT_ISO_SYNC_RECEIVER */

#if defined(CONFIG_BT_ISO_UNICAST) || defined(CONFIG_BT_ISO_BROADCASTER)
static uint16_t iso_chan_max_data_len(const struct bt_iso_chan *chan,
				      uint32_t ts)
{
	size_t max_controller_data_len;
	uint16_t max_data_len;

	if (chan->qos->tx == NULL) {
		return 0;
	}

	max_data_len = chan->qos->tx->sdu;

	/* Ensure that the SDU fits when using all the buffers */
	max_controller_data_len = bt_dev.le.iso_mtu * bt_dev.le.iso_limit;

	/* Update the max_data_len to take the max_controller_data_len into account */
	max_data_len = MIN(max_data_len, max_controller_data_len);

	return max_data_len;
}

int bt_iso_chan_send(struct bt_iso_chan *chan, struct net_buf *buf,
		     uint16_t seq_num, uint32_t ts)
{
	uint16_t max_data_len;
	struct bt_conn *iso_conn;

	CHECKIF(!chan || !buf) {
		LOG_DBG("Invalid parameters: chan %p buf %p", chan, buf);
		return -EINVAL;
	}

	BT_ISO_DATA_DBG("chan %p len %zu", chan, net_buf_frags_len(buf));

	if (chan->state != BT_ISO_STATE_CONNECTED) {
		LOG_DBG("Not connected");
		return -ENOTCONN;
	}

	iso_conn = chan->iso;
	if (!iso_conn->iso.info.can_send) {
		LOG_DBG("Channel not able to send");
		return -EINVAL;
	}

	if (ts == BT_ISO_TIMESTAMP_NONE &&
	    buf->size < BT_HCI_ISO_DATA_HDR_SIZE) {
		LOG_DBG("Cannot send ISO packet with buffer size %u", buf->size);

		return -EMSGSIZE;
	} else if (buf->size < BT_HCI_ISO_TS_DATA_HDR_SIZE) {
		LOG_DBG("Cannot send ISO packet with timestamp with buffer size %u", buf->size);

		return -EMSGSIZE;
	}

	max_data_len = iso_chan_max_data_len(chan, ts);
	if (buf->len > max_data_len) {
		LOG_DBG("Cannot send %u octets, maximum %u", buf->len, max_data_len);
		return -EMSGSIZE;
	}

	if (ts == BT_ISO_TIMESTAMP_NONE) {
		struct bt_hci_iso_data_hdr *hdr;

		hdr = net_buf_push(buf, sizeof(*hdr));
		hdr->sn = sys_cpu_to_le16(seq_num);
		hdr->slen = sys_cpu_to_le16(bt_iso_pkt_len_pack(net_buf_frags_len(buf)
								- sizeof(*hdr),
								BT_ISO_DATA_VALID));
	} else {
		struct bt_hci_iso_ts_data_hdr *hdr;

		hdr = net_buf_push(buf, sizeof(*hdr));
		hdr->ts = ts;
		hdr->data.sn = sys_cpu_to_le16(seq_num);
		hdr->data.slen = sys_cpu_to_le16(bt_iso_pkt_len_pack(net_buf_frags_len(buf)
								     - sizeof(*hdr),
								     BT_ISO_DATA_VALID));
	}

	return bt_conn_send_iso_cb(iso_conn,
				   buf,
				   bt_iso_send_cb,
				   ts != BT_ISO_TIMESTAMP_NONE);
}

#if defined(CONFIG_BT_ISO_CENTRAL) || defined(CONFIG_BT_ISO_BROADCASTER)
static bool valid_chan_io_qos(const struct bt_iso_chan_io_qos *io_qos,
			      bool is_tx)
{
	const size_t max_mtu = (is_tx ? CONFIG_BT_ISO_TX_MTU : CONFIG_BT_ISO_RX_MTU);
	const size_t max_sdu = MIN(max_mtu, BT_ISO_MAX_SDU);

	if (io_qos->sdu > max_sdu) {
		LOG_DBG("sdu (%u) shall be smaller than %zu", io_qos->sdu, max_sdu);
		return false;
	}

	if (io_qos->phy > BT_GAP_LE_PHY_CODED) {
		LOG_DBG("Invalid phy %u", io_qos->phy);
		return false;
	}

	return true;
}
#endif /* CONFIG_BT_ISO_CENTRAL || CONFIG_BT_ISO_BROADCASTER */

int bt_iso_chan_get_tx_sync(const struct bt_iso_chan *chan, struct bt_iso_tx_info *info)
{
	struct bt_hci_cp_le_read_iso_tx_sync *cp;
	struct bt_hci_rp_le_read_iso_tx_sync *rp;
	struct net_buf *buf;
	struct net_buf *rsp = NULL;
	int err;

	CHECKIF(chan == NULL) {
		LOG_DBG("chan is NULL");
		return -EINVAL;
	}

	CHECKIF(chan->iso == NULL) {
		LOG_DBG("chan->iso is NULL");
		return -EINVAL;
	}

	CHECKIF(info == NULL) {
		LOG_DBG("info is NULL");
		return -EINVAL;
	}

	CHECKIF(chan->state != BT_ISO_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_READ_ISO_TX_SYNC, sizeof(*cp));
	if (!buf) {
		return -ENOMEM;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(chan->iso->handle);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_READ_ISO_TX_SYNC, buf, &rsp);
	if (err) {
		return err;
	}

	if (rsp) {
		rp = (struct bt_hci_rp_le_read_iso_tx_sync *)rsp->data;

		info->ts = sys_le32_to_cpu(rp->timestamp);
		info->seq_num = sys_le16_to_cpu(rp->seq);
		info->offset = sys_get_le24(rp->offset);

		net_buf_unref(rsp);
	} else {
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_BT_ISO_UNICAST) || CONFIG_BT_ISO_BROADCASTER */

#if defined(CONFIG_BT_ISO_UNICAST)
int bt_iso_chan_disconnect(struct bt_iso_chan *chan)
{
	int err;

	CHECKIF(!chan) {
		LOG_DBG("Invalid parameter: chan %p", chan);
		return -EINVAL;
	}

	CHECKIF(chan->iso == NULL) {
		LOG_DBG("Channel has not been initialized in a CIG");
		return -EINVAL;
	}

	if (chan->iso->iso.acl == NULL ||
	    chan->state == BT_ISO_STATE_DISCONNECTED) {
		LOG_DBG("Channel is not connected");
		return -ENOTCONN;
	}

	if (chan->state == BT_ISO_STATE_ENCRYPT_PENDING) {
		LOG_DBG("Channel already disconnected");
		bt_iso_chan_set_state(chan, BT_ISO_STATE_DISCONNECTED);

		if (chan->ops->disconnected) {
			chan->ops->disconnected(chan,
						BT_HCI_ERR_LOCALHOST_TERM_CONN);
		}

		return 0;
	}

	if (chan->state == BT_ISO_STATE_DISCONNECTING) {
		LOG_DBG("Already disconnecting");

		return -EALREADY;
	}

	err = bt_conn_disconnect(chan->iso, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err == 0) {
		bt_iso_chan_set_state(chan, BT_ISO_STATE_DISCONNECTING);
	}

	return err;
}

void bt_iso_cleanup_acl(struct bt_conn *iso)
{
	LOG_DBG("%p", iso);

	if (iso->iso.acl) {
		bt_conn_unref(iso->iso.acl);
		iso->iso.acl = NULL;
	}
}

static void store_cis_info(const struct bt_hci_evt_le_cis_established *evt,
			   struct bt_iso_info *info)
{
	struct bt_iso_unicast_info *unicast_info = &info->unicast;
	struct bt_iso_unicast_tx_info *central = &unicast_info->central;
	struct bt_iso_unicast_tx_info *peripheral = &unicast_info->peripheral;

	info->iso_interval = sys_le16_to_cpu(evt->interval);
	info->max_subevent = evt->nse;

	unicast_info->cig_sync_delay = sys_get_le24(evt->cig_sync_delay);
	unicast_info->cis_sync_delay = sys_get_le24(evt->cis_sync_delay);

	central->bn = evt->c_bn;
	central->phy = bt_get_phy(evt->c_phy);
	central->latency = sys_get_le16(evt->c_latency);
	central->max_pdu = sys_le16_to_cpu(evt->c_max_pdu);
	/* Transform to n * 1.25ms */
	central->flush_timeout = info->iso_interval * evt->c_ft;

	peripheral->bn = evt->p_bn;
	peripheral->phy = bt_get_phy(evt->p_phy);
	peripheral->latency = sys_get_le16(evt->p_latency);
	peripheral->max_pdu = sys_le16_to_cpu(evt->p_max_pdu);
	/* Transform to n * 1.25ms */
	peripheral->flush_timeout = info->iso_interval * evt->p_ft;
}

void hci_le_cis_established(struct net_buf *buf)
{
	struct bt_hci_evt_le_cis_established *evt = (void *)buf->data;
	uint16_t handle = sys_le16_to_cpu(evt->conn_handle);
	struct bt_conn *iso;

	LOG_DBG("status %u handle %u", evt->status, handle);

	/* ISO connection handles are already assigned at this point */
	iso = bt_conn_lookup_handle(handle);
	if (!iso) {
		LOG_ERR("No connection found for handle %u", handle);
		return;
	}

	CHECKIF(iso->type != BT_CONN_TYPE_ISO) {
		LOG_DBG("Invalid connection type %u", iso->type);
		bt_conn_unref(iso);
		return;
	}

	if (!evt->status) {
		struct bt_iso_chan_io_qos *tx;
		struct bt_iso_chan_io_qos *rx;
		struct bt_conn_iso *iso_conn;
		struct bt_iso_chan *chan;

		iso_conn = &iso->iso;
		chan = iso_conn->chan;

		__ASSERT(chan != NULL && chan->qos != NULL, "Invalid ISO chan");

		tx = chan->qos->tx;
		rx = chan->qos->rx;

		LOG_DBG("iso_chan %p tx %p rx %p", chan, tx, rx);

		if (iso->role == BT_HCI_ROLE_PERIPHERAL) {
			rx = chan->qos->rx;
			tx = chan->qos->tx;

			if (rx != NULL) {
				rx->phy = bt_get_phy(evt->c_phy);
				rx->sdu = sys_le16_to_cpu(evt->c_max_pdu);
			}

			if (tx != NULL) {
				tx->phy = bt_get_phy(evt->p_phy);
				tx->sdu = sys_le16_to_cpu(evt->p_max_pdu);
			}

			iso_conn->info.type = BT_ISO_CHAN_TYPE_CONNECTED;
		} /* values are already set for central */

		/* Verify if device can send */
		iso_conn->info.can_send = false;
		if (tx != NULL) {
			if (iso->role == BT_HCI_ROLE_PERIPHERAL &&
			    evt->p_bn > 0) {
				iso_conn->info.can_send = true;
			} else if (iso->role == BT_HCI_ROLE_CENTRAL &&
				   evt->c_bn > 0) {
				iso_conn->info.can_send = true;
			}
		}

		/* Verify if device can recv */
		iso_conn->info.can_recv = false;
		if (rx != NULL) {
			if (iso->role == BT_HCI_ROLE_PERIPHERAL &&
			    evt->c_bn > 0) {
				iso_conn->info.can_recv = true;
			} else if (iso->role == BT_HCI_ROLE_CENTRAL &&
				   evt->p_bn > 0) {
				iso_conn->info.can_recv = true;
			}
		}

		store_cis_info(evt, &iso_conn->info);
		bt_conn_set_state(iso, BT_CONN_CONNECTED);
		bt_conn_unref(iso);
		return;
	}

	iso->err = evt->status;
	bt_iso_disconnected(iso);
	bt_conn_unref(iso);
}

#if defined(CONFIG_BT_ISO_PERIPHERAL)
int bt_iso_server_register(struct bt_iso_server *server)
{
	CHECKIF(!server) {
		LOG_DBG("Invalid parameter: server %p", server);
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

#if defined(CONFIG_BT_SMP)
	if (server->sec_level > BT_SECURITY_L3) {
		return -EINVAL;
	} else if (server->sec_level < BT_SECURITY_L1) {
		/* Level 0 is only applicable for BR/EDR */
		server->sec_level = BT_SECURITY_L1;
	}
#endif /* CONFIG_BT_SMP */

	LOG_DBG("%p", server);

	iso_server = server;

	return 0;
}

int bt_iso_server_unregister(struct bt_iso_server *server)
{
	CHECKIF(!server) {
		LOG_DBG("Invalid parameter: server %p", server);
		return -EINVAL;
	}

	if (iso_server != server) {
		return -EINVAL;
	}

	iso_server = NULL;

	return 0;
}

static int iso_accept(struct bt_conn *acl, struct bt_conn *iso)
{
	struct bt_iso_accept_info accept_info;
	struct bt_iso_chan *chan;
	int err;

	CHECKIF(!iso || iso->type != BT_CONN_TYPE_ISO) {
		LOG_DBG("Invalid parameters: iso %p iso->type %u", iso, iso ? iso->type : 0);
		return -EINVAL;
	}

	LOG_DBG("%p", iso);

	accept_info.acl = acl;
	accept_info.cig_id = iso->iso.cig_id;
	accept_info.cis_id = iso->iso.cis_id;

	err = iso_server->accept(&accept_info, &chan);
	if (err < 0) {
		LOG_ERR("Server failed to accept: %d", err);
		return err;
	}

#if defined(CONFIG_BT_SMP)
	chan->required_sec_level = iso_server->sec_level;
#endif /* CONFIG_BT_SMP */

	bt_iso_chan_add(iso, chan);
	bt_iso_chan_set_state(chan, BT_ISO_STATE_CONNECTING);

	return 0;
}

static int hci_le_reject_cis(uint16_t handle, uint8_t reason)
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

static int hci_le_accept_cis(uint16_t handle)
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

static uint8_t iso_server_check_security(struct bt_conn *conn)
{
	if (IS_ENABLED(CONFIG_BT_CONN_DISABLE_SECURITY)) {
		return BT_HCI_ERR_SUCCESS;
	}

#if defined(CONFIG_BT_SMP)
	if (conn->sec_level >= iso_server->sec_level) {
		return BT_HCI_ERR_SUCCESS;
	}

	return BT_HCI_ERR_INSUFFICIENT_SECURITY;
#else
	return BT_HCI_ERR_SUCCESS;
#endif /* CONFIG_BT_SMP */
}

void hci_le_cis_req(struct net_buf *buf)
{
	struct bt_hci_evt_le_cis_req *evt = (void *)buf->data;
	uint16_t acl_handle = sys_le16_to_cpu(evt->acl_handle);
	uint16_t cis_handle = sys_le16_to_cpu(evt->cis_handle);
	struct bt_conn *acl, *iso;
	uint8_t sec_err;
	int err;

	LOG_DBG("acl_handle %u cis_handle %u cig_id %u cis %u", acl_handle, cis_handle, evt->cig_id,
		evt->cis_id);

	if (iso_server == NULL) {
		LOG_DBG("No ISO server registered");
		hci_le_reject_cis(cis_handle, BT_HCI_ERR_UNSPECIFIED);
		return;
	}

	/* Lookup existing connection with same handle */
	iso = bt_conn_lookup_handle(cis_handle);
	if (iso) {
		LOG_ERR("Invalid ISO handle %u", cis_handle);
		hci_le_reject_cis(cis_handle, BT_HCI_ERR_CONN_LIMIT_EXCEEDED);
		bt_conn_unref(iso);
		return;
	}

	/* Lookup ACL connection to attach */
	acl = bt_conn_lookup_handle(acl_handle);
	if (!acl) {
		LOG_ERR("Invalid ACL handle %u", acl_handle);
		hci_le_reject_cis(cis_handle, BT_HCI_ERR_UNKNOWN_CONN_ID);
		return;
	}

	sec_err = iso_server_check_security(acl);
	if (sec_err != BT_HCI_ERR_SUCCESS) {
		LOG_DBG("Insufficient security %u", sec_err);
		err = hci_le_reject_cis(cis_handle, sec_err);
		if (err != 0) {
			LOG_ERR("Failed to reject CIS");
		}

		bt_conn_unref(acl);
		return;
	}

	/* Add ISO connection */
	iso = bt_conn_add_iso(acl);

	bt_conn_unref(acl);

	if (!iso) {
		LOG_ERR("Could not create and add ISO to ACL %u", acl_handle);
		hci_le_reject_cis(cis_handle,
				  BT_HCI_ERR_INSUFFICIENT_RESOURCES);
		return;
	}

	iso->iso.info.type = BT_ISO_CHAN_TYPE_CONNECTED;
	iso->iso.cig_id = evt->cig_id;
	iso->iso.cis_id = evt->cis_id;

	/* Request application to accept */
	err = iso_accept(acl, iso);
	if (err) {
		LOG_DBG("App rejected ISO %d", err);
		bt_iso_cleanup_acl(iso);
		bt_conn_unref(iso);
		hci_le_reject_cis(cis_handle,
				  BT_HCI_ERR_INSUFFICIENT_RESOURCES);
		return;
	}

	iso->handle = cis_handle;
	iso->role = BT_HCI_ROLE_PERIPHERAL;
	bt_conn_set_state(iso, BT_CONN_CONNECTING);

	err = hci_le_accept_cis(cis_handle);
	if (err) {
		bt_iso_cleanup_acl(iso);
		bt_conn_unref(iso);
		hci_le_reject_cis(cis_handle,
				  BT_HCI_ERR_INSUFFICIENT_RESOURCES);
		return;
	}
}

static struct bt_conn *bt_conn_add_iso(struct bt_conn *acl)
{
	struct bt_conn *iso = iso_new();

	if (iso == NULL) {
		LOG_ERR("Unable to allocate ISO connection");
		return NULL;
	}

	iso->iso.acl = bt_conn_ref(acl);

	return iso;
}
#endif /* CONFIG_BT_ISO_PERIPHERAL */

#if defined(CONFIG_BT_ISO_CENTRAL)
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

static void bt_iso_remove_data_path(struct bt_conn *iso)
{
	enum bt_iso_chan_type type = iso->iso.info.type;

	LOG_DBG("%p", iso);

	/* TODO: Removing the ISO data path is never used for broadcast:
	 * Remove the following broadcast implementation?
	 */
	if ((IS_ENABLED(CONFIG_BT_ISO_BROADCASTER) &&
		type == BT_ISO_CHAN_TYPE_BROADCASTER) ||
	    (IS_ENABLED(CONFIG_BT_ISO_SYNC_RECEIVER) &&
		type == BT_ISO_CHAN_TYPE_SYNC_RECEIVER)) {
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
	} else if (IS_ENABLED(CONFIG_BT_ISO_UNICAST) &&
		   type == BT_ISO_CHAN_TYPE_CONNECTED) {
		/* Remove both directions for CIS*/

		/* TODO: Check which has been setup first to avoid removing
		 * data paths that are not setup
		 */
		(void)hci_le_remove_iso_data_path(iso,
						  BT_HCI_DATAPATH_DIR_CTLR_TO_HOST);
		(void)hci_le_remove_iso_data_path(iso,
						  BT_HCI_DATAPATH_DIR_HOST_TO_CTLR);
	} else {
		__ASSERT(false, "Invalid iso.type: %u", type);
	}
}

static bool valid_chan_qos(const struct bt_iso_chan_qos *qos)
{
	if (qos->rx != NULL) {
		if (!valid_chan_io_qos(qos->rx, false)) {
			LOG_DBG("Invalid rx qos");
			return false;
		}
	} else if (qos->tx == NULL) {
		LOG_DBG("Both rx and tx qos are NULL");
		return false;
	}

	if (qos->tx != NULL) {
		if (!valid_chan_io_qos(qos->tx, true)) {
			LOG_DBG("Invalid tx qos");
			return false;
		}
	}

	return true;
}

static int hci_le_remove_cig(uint8_t cig_id)
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

static struct net_buf *hci_le_set_cig_params(const struct bt_iso_cig *cig,
					     const struct bt_iso_cig_param *param)
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

	LOG_DBG("id %u, latency %u, interval %u, sca %u, packing %u, framing %u, num_cis %u",
		cig->id, param->latency, param->interval, param->sca, param->packing,
		param->framing, param->num_cis);

	/* Program the cis parameters */
	for (i = 0; i < param->num_cis; i++) {
		struct bt_iso_chan *cis = param->cis_channels[i];
		struct bt_iso_chan_qos *qos = cis->qos;

		cis_param = net_buf_add(buf, sizeof(*cis_param));

		memset(cis_param, 0, sizeof(*cis_param));

		cis_param->cis_id = cis->iso->iso.cis_id;

		if (!qos->tx && !qos->rx) {
			LOG_ERR("Both TX and RX QoS are disabled");
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

		LOG_DBG("[%d]: id %u, c_phy %u, c_sdu %u, c_rtn %u, p_phy %u, p_sdu %u, p_rtn %u",
			i, cis_param->cis_id, cis_param->c_phy, cis_param->c_sdu, cis_param->c_rtn,
			cis_param->p_phy, cis_param->p_sdu, cis_param->p_rtn);
	}

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_CIG_PARAMS, buf, &rsp);
	if (err) {
		return NULL;
	}

	return rsp;
}

static struct bt_iso_cig *get_cig(const struct bt_iso_chan *iso_chan)
{
	if (iso_chan->iso == NULL) {
		return NULL;
	}

	__ASSERT(iso_chan->iso->iso.cig_id < ARRAY_SIZE(cigs),
		 "Invalid cig_id %u", iso_chan->iso->iso.cig_id);

	return &cigs[iso_chan->iso->iso.cig_id];
}

static struct bt_iso_cig *get_free_cig(void)
{
	/* We can use the index in the `cigs` array as CIG ID */

	for (size_t i = 0; i < ARRAY_SIZE(cigs); i++) {
		if (cigs[i].state == BT_ISO_CIG_STATE_IDLE) {
			cigs[i].state = BT_ISO_CIG_STATE_CONFIGURED;
			cigs[i].id = i;
			sys_slist_init(&cigs[i].cis_channels);
			return &cigs[i];
		}
	}

	LOG_DBG("Could not allocate any more CIGs");

	return NULL;
}

static bool cis_is_in_cig(const struct bt_iso_cig *cig,
			  const struct bt_iso_chan *cis)
{
	return cig->id == cis->iso->iso.cig_id;
}

static int cig_init_cis(struct bt_iso_cig *cig,
			const struct bt_iso_cig_param *param)
{
	for (uint8_t i = 0; i < param->num_cis; i++) {
		struct bt_iso_chan *cis = param->cis_channels[i];

		if (cis->iso == NULL) {
			struct bt_conn_iso *iso_conn;

			cis->iso = iso_new();
			if (cis->iso == NULL) {
				LOG_ERR("Unable to allocate CIS connection");
				return -ENOMEM;
			}
			iso_conn = &cis->iso->iso;

			iso_conn->cig_id = cig->id;
			iso_conn->info.type = BT_ISO_CHAN_TYPE_CONNECTED;
			iso_conn->cis_id = cig->num_cis++;

			bt_iso_chan_add(cis->iso, cis);

			sys_slist_append(&cig->cis_channels, &cis->node);
		} /* else already initialized */
	}

	return 0;
}

static void cleanup_cig(struct bt_iso_cig *cig)
{
	struct bt_iso_chan *cis, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&cig->cis_channels, cis, tmp, node) {
		if (cis->iso != NULL) {
			bt_conn_unref(cis->iso);
			cis->iso = NULL;
		}

		sys_slist_remove(&cig->cis_channels, NULL, &cis->node);
	}

	memset(cig, 0, sizeof(*cig));
}

static bool valid_cig_param(const struct bt_iso_cig_param *param)
{
	if (param == NULL) {
		return false;
	}

	for (uint8_t i = 0; i < param->num_cis; i++) {
		struct bt_iso_chan *cis = param->cis_channels[i];

		if (cis == NULL) {
			LOG_DBG("cis_channels[%d]: NULL channel", i);
			return false;
		}

		if (cis->iso != NULL) {
			LOG_DBG("cis_channels[%d]: already allocated", i);
			return false;
		}

		if (!valid_chan_qos(cis->qos)) {
			LOG_DBG("cis_channels[%d]: Invalid QOS", i);
			return false;
		}

		for (uint8_t j = 0; j < i; j++) {
			if (cis == param->cis_channels[j]) {
				LOG_DBG("ISO %p duplicated at index %u and %u", cis, i, j);
				return false;
			}
		}
	}

	if (param->framing != BT_ISO_FRAMING_UNFRAMED &&
	    param->framing != BT_ISO_FRAMING_FRAMED) {
		LOG_DBG("Invalid framing parameter: %u", param->framing);
		return false;
	}

	if (param->packing != BT_ISO_PACKING_SEQUENTIAL &&
	    param->packing != BT_ISO_PACKING_INTERLEAVED) {
		LOG_DBG("Invalid packing parameter: %u", param->packing);
		return false;
	}

	if (param->num_cis > BT_ISO_MAX_GROUP_ISO_COUNT ||
	    param->num_cis > CONFIG_BT_ISO_MAX_CHAN) {
		LOG_DBG("num_cis (%u) shall be lower than: %u", param->num_cis,
			MAX(CONFIG_BT_ISO_MAX_CHAN, BT_ISO_MAX_GROUP_ISO_COUNT));
		return false;
	}

	if (param->interval < BT_ISO_SDU_INTERVAL_MIN ||
	    param->interval > BT_ISO_SDU_INTERVAL_MAX) {
		LOG_DBG("Invalid interval: %u", param->interval);
		return false;
	}

	if (param->latency < BT_ISO_LATENCY_MIN ||
	    param->latency > BT_ISO_LATENCY_MAX) {
		LOG_DBG("Invalid latency: %u", param->latency);
		return false;
	}

	return true;
}

int bt_iso_cig_create(const struct bt_iso_cig_param *param,
		      struct bt_iso_cig **out_cig)
{
	int err;
	struct net_buf *rsp;
	struct bt_iso_cig *cig;
	struct bt_hci_rp_le_set_cig_params *cig_rsp;
	struct bt_iso_chan *cis;
	int i;

	CHECKIF(out_cig == NULL) {
		LOG_DBG("out_cig is NULL");
		return -EINVAL;
	}

	*out_cig = NULL;

	/* Check if controller is ISO capable as a central */
	if (!BT_FEAT_LE_CIS_CENTRAL(bt_dev.le.features)) {
		return -ENOTSUP;
	}

	/* TBD: Should we allow creating empty CIGs? */
	CHECKIF(param->cis_channels == NULL) {
		LOG_DBG("NULL CIS channels");
		return -EINVAL;
	}

	CHECKIF(param->num_cis == 0) {
		LOG_DBG("Invalid number of CIS %u", param->num_cis);
		return -EINVAL;
	}

	CHECKIF(!valid_cig_param(param)) {
		LOG_DBG("Invalid CIG params");
		return -EINVAL;
	}

	cig = get_free_cig();

	if (!cig) {
		return -ENOMEM;
	}

	err = cig_init_cis(cig, param);
	if (err) {
		LOG_DBG("Could not init CIS %d", err);
		cleanup_cig(cig);
		return err;
	}

	rsp = hci_le_set_cig_params(cig, param);
	if (rsp == NULL) {
		LOG_WRN("Unexpected response to hci_le_set_cig_params");
		err = -EIO;
		cleanup_cig(cig);
		return err;
	}

	cig_rsp = (void *)rsp->data;

	if (rsp->len < sizeof(cig_rsp) ||
	    cig_rsp->num_handles != param->num_cis) {
		LOG_WRN("Unexpected response to hci_le_set_cig_params");
		err = -EIO;
		net_buf_unref(rsp);
		cleanup_cig(cig);
		return err;
	}

	i = 0;
	SYS_SLIST_FOR_EACH_CONTAINER(&cig->cis_channels, cis, node) {
		const uint16_t handle = cig_rsp->handle[i++];

		/* Assign the connection handle */
		cis->iso->handle = sys_le16_to_cpu(handle);
	}

	net_buf_unref(rsp);

	*out_cig = cig;

	return err;
}

static void restore_cig(struct bt_iso_cig *cig, uint8_t existing_num_cis)
{
	struct bt_iso_chan *cis, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&cig->cis_channels, cis, tmp, node) {
		/* Remove all newly added by comparing the cis_id to the number
		 * of CIS that was previously added before
		 * bt_iso_cig_reconfigure was called
		 */
		if (cis->iso != NULL &&
		    cis->iso->iso.cis_id >= existing_num_cis) {
			bt_conn_unref(cis->iso);
			cis->iso = NULL;

			sys_slist_remove(&cig->cis_channels, NULL, &cis->node);
			cig->num_cis--;
		}
	}
}


int bt_iso_cig_reconfigure(struct bt_iso_cig *cig,
			   const struct bt_iso_cig_param *param)
{
	struct bt_hci_rp_le_set_cig_params *cig_rsp;
	uint8_t existing_num_cis;
	struct bt_iso_chan *cis;
	struct net_buf *rsp;
	int err;
	int i;

	CHECKIF(cig == NULL) {
		LOG_DBG("cig is NULL");
		return -EINVAL;
	}

	if (cig->state != BT_ISO_CIG_STATE_CONFIGURED) {
		LOG_DBG("Invalid CIG state: %u", cig->state);
		return -EINVAL;
	}

	CHECKIF(!valid_cig_param(param)) {
		LOG_DBG("Invalid CIG params");
		return -EINVAL;
	}

	for (uint8_t i = 0; i < param->num_cis; i++) {
		struct bt_iso_chan *cis = param->cis_channels[i];

		if (cis->iso != NULL && !cis_is_in_cig(cig, cis)) {
			LOG_DBG("Cannot reconfigure other CIG's (id 0x%02X) CIS "
			       "with this CIG (id 0x%02X)",
			       cis->iso->iso.cig_id, cig->id);
			return -EINVAL;
		}
	}

	/* Used to restore CIG in case of error */
	existing_num_cis = cig->num_cis;

	err = cig_init_cis(cig, param);
	if (err != 0) {
		LOG_DBG("Could not init CIS %d", err);
		restore_cig(cig, existing_num_cis);
		return err;
	}

	rsp = hci_le_set_cig_params(cig, param);
	if (rsp == NULL) {
		LOG_WRN("Unexpected response to hci_le_set_cig_params");
		err = -EIO;
		restore_cig(cig, existing_num_cis);
		return err;
	}

	cig_rsp = (void *)rsp->data;

	if (rsp->len < sizeof(cig_rsp) ||
	    cig_rsp->num_handles != param->num_cis) {
		LOG_WRN("Unexpected response to hci_le_set_cig_params");
		err = -EIO;
		net_buf_unref(rsp);
		restore_cig(cig, existing_num_cis);
		return err;
	}

	i = 0;
	SYS_SLIST_FOR_EACH_CONTAINER(&cig->cis_channels, cis, node) {
		const uint16_t handle = cig_rsp->handle[i++];

		/* Assign the connection handle */
		cis->iso->handle = sys_le16_to_cpu(handle);
	}

	net_buf_unref(rsp);

	return err;
}

int bt_iso_cig_terminate(struct bt_iso_cig *cig)
{
	int err;

	CHECKIF(cig == NULL) {
		LOG_DBG("cig is NULL");
		return -EINVAL;
	}

	if (cig->state != BT_ISO_CIG_STATE_INACTIVE &&
	    cig->state != BT_ISO_CIG_STATE_CONFIGURED) {
		LOG_DBG("Invalid CIG state: %u", cig->state);
		return -EINVAL;
	}

	err = hci_le_remove_cig(cig->id);
	if (err != 0) {
		LOG_DBG("Failed to terminate CIG: %d", err);
		return err;
	}

	cleanup_cig(cig);

	return 0;
}

void bt_iso_security_changed(struct bt_conn *acl, uint8_t hci_status)
{
	struct bt_iso_connect_param param[CONFIG_BT_ISO_MAX_CHAN];
	size_t param_count;
	int err;

	/* The peripheral does not accept any ISO requests if security is
	 * insufficient, so we only need to handle central here.
	 * BT_ISO_STATE_ENCRYPT_PENDING is only set by the central.
	 */
	if (!IS_ENABLED(CONFIG_BT_CENTRAL) ||
	    acl->role != BT_CONN_ROLE_CENTRAL) {
		return;
	}

	param_count = 0;
	for (size_t i = 0; i < ARRAY_SIZE(iso_conns); i++) {
		struct bt_conn *iso = &iso_conns[i];
		struct bt_iso_chan *iso_chan;

		if (iso == NULL || iso->iso.acl != acl) {
			continue;
		}

		iso_chan = iso_chan(iso);
		if (iso_chan->state != BT_ISO_STATE_ENCRYPT_PENDING) {
			continue;
		}

		bt_iso_chan_set_state(iso_chan, BT_ISO_STATE_DISCONNECTED);

		if (hci_status == BT_HCI_ERR_SUCCESS) {
			param[param_count].acl = acl;
			param[param_count].iso_chan = iso_chan;
			param_count++;
		} else {
			LOG_DBG("Failed to encrypt ACL %p for ISO %p: %u", acl, iso, hci_status);

			/* We utilize the disconnected callback to make the
			 * upper layers aware of the error
			 */
			if (iso_chan->ops->disconnected) {
				iso_chan->ops->disconnected(iso_chan,
							    hci_status);
			}
		}
	}

	if (param_count == 0) {
		/* Nothing to do for ISO. This happens if security is changed,
		 * but no ISO channels were pending encryption.
		 */
		return;
	}

	err = hci_le_create_cis(param, param_count);
	if (err != 0) {
		LOG_ERR("Failed to connect CISes: %d", err);

		for (size_t i = 0; i < param_count; i++) {
			struct bt_iso_chan *iso_chan = param[i].iso_chan;

			/* We utilize the disconnected callback to make the
			 * upper layers aware of the error
			 */
			if (iso_chan->ops->disconnected) {
				iso_chan->ops->disconnected(iso_chan,
							    hci_status);
			}
		}

		return;
	}

	/* Set connection states */
	for (size_t i = 0; i < param_count; i++) {
		struct bt_iso_chan *iso_chan = param[i].iso_chan;
		struct bt_iso_cig *cig = get_cig(iso_chan);

		__ASSERT(cig != NULL, "CIG was NULL");
		cig->state = BT_ISO_CIG_STATE_ACTIVE;

		bt_conn_set_state(iso_chan->iso, BT_CONN_CONNECTING);
		bt_iso_chan_set_state(iso_chan, BT_ISO_STATE_CONNECTING);
	}
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

	/* Program the cis parameters */
	for (size_t i = 0; i < count; i++) {
		struct bt_iso_chan *iso_chan = param[i].iso_chan;

		if (iso_chan->state == BT_ISO_STATE_ENCRYPT_PENDING) {
			continue;
		}

		cis = net_buf_add(buf, sizeof(*cis));

		memset(cis, 0, sizeof(*cis));

		cis->cis_handle = sys_cpu_to_le16(param[i].iso_chan->iso->handle);
		cis->acl_handle = sys_cpu_to_le16(param[i].acl->handle);
		req->num_cis++;
	}

	/* If all CIS are pending for security, do nothing,
	 * but return a recognizable return value
	 */
	if (req->num_cis == 0) {
		net_buf_unref(buf);

		return -ECANCELED;
	}

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_CREATE_CIS, buf, NULL);
}

#if defined(CONFIG_BT_SMP)
static int iso_chan_connect_security(const struct bt_iso_connect_param *param,
				     size_t count)
{
	/* conn_idx_handled is an array of booleans for which conn indexes
	 * already have been used to call bt_conn_set_security.
	 * Using indexes avoids looping the array when checking if it has been
	 * handled.
	 */
	bool conn_idx_handled[CONFIG_BT_MAX_CONN];

	memset(conn_idx_handled, false, sizeof(conn_idx_handled));
	for (size_t i = 0; i < count; i++) {
		struct bt_iso_chan *iso_chan = param[i].iso_chan;
		struct bt_conn *acl = param[i].acl;
		uint8_t conn_idx = bt_conn_index(acl);

		if (acl->sec_level < iso_chan->required_sec_level) {
			if (!conn_idx_handled[conn_idx]) {
				int err;

				err = bt_conn_set_security(acl,
							   iso_chan->required_sec_level);
				if (err != 0) {
					LOG_DBG("[%zu]: Failed to set security: %d", i, err);

					/* Restore states */
					for (size_t j = 0; j < i; j++) {
						iso_chan = param[j].iso_chan;

						bt_iso_cleanup_acl(iso_chan->iso);
						bt_iso_chan_set_state(iso_chan,
								      BT_ISO_STATE_DISCONNECTED);
					}

					return err;
				}

				conn_idx_handled[conn_idx] = true;
			}

			iso_chan->iso->iso.acl = bt_conn_ref(acl);
			bt_iso_chan_set_state(iso_chan, BT_ISO_STATE_ENCRYPT_PENDING);
		}
	}

	return 0;
}
#endif /* CONFIG_BT_SMP */

static bool iso_chans_connecting(void)
{
	for (size_t i = 0U; i < ARRAY_SIZE(iso_conns); i++) {
		const struct bt_conn *iso = &iso_conns[i];
		const struct bt_iso_chan *iso_chan;

		if (iso == NULL ||
		    iso->iso.info.type != BT_ISO_CHAN_TYPE_CONNECTED) {
			continue;
		}

		iso_chan = iso_chan(iso);
		if (iso_chan->state == BT_ISO_STATE_CONNECTING ||
		    iso_chan->state == BT_ISO_STATE_ENCRYPT_PENDING) {
			return true;
		}
	}

	return false;
}

int bt_iso_chan_connect(const struct bt_iso_connect_param *param, size_t count)
{
	int err;

	CHECKIF(param == NULL) {
		LOG_DBG("param is NULL");
		return -EINVAL;
	}

	CHECKIF(count == 0) {
		LOG_DBG("Invalid count %zu", count);
		return -EINVAL;
	}

	CHECKIF(count > CONFIG_BT_ISO_MAX_CHAN) {
		return -EINVAL;
	}

	/* Validate input */
	for (size_t i = 0; i < count; i++) {
		CHECKIF(param[i].iso_chan == NULL) {
			LOG_DBG("[%zu]: Invalid iso (%p)", i, param[i].iso_chan);
			return -EINVAL;
		}

		CHECKIF(param[i].acl == NULL) {
			LOG_DBG("[%zu]: Invalid acl (%p)", i, param[i].acl);
			return -EINVAL;
		}

		CHECKIF((param[i].acl->type & BT_CONN_TYPE_LE) == 0) {
			LOG_DBG("[%zu]: acl type (%u) shall be an LE connection", i,
				param[i].acl->type);
			return -EINVAL;
		}

		if (param[i].iso_chan->iso == NULL) {
			LOG_DBG("[%zu]: ISO has not been initialized in a CIG", i);
			return -EINVAL;
		}

		if (param[i].iso_chan->state != BT_ISO_STATE_DISCONNECTED) {
			LOG_DBG("[%zu]: ISO is not in the BT_ISO_STATE_DISCONNECTED state: %u", i,
				param[i].iso_chan->state);
			return -EINVAL;
		}
	}

	if (iso_chans_connecting()) {
		LOG_DBG("There are pending ISO connections");
		return -EBUSY;
	}

#if defined(CONFIG_BT_SMP)
	/* Check for and initiate security for all channels that have
	 * requested encryption if the ACL link is not already secured
	 */
	err = iso_chan_connect_security(param, count);
	if (err != 0) {
		LOG_DBG("Failed to initate security for all CIS: %d", err);
		return err;
	}
#endif /* CONFIG_BT_SMP */

	err = hci_le_create_cis(param, count);
	if (err == -ECANCELED) {
		LOG_DBG("All channels are pending on security");

		return 0;
	} else if (err != 0) {
		LOG_DBG("Failed to connect CISes: %d", err);

		return err;
	}

	/* Set connection states */
	for (size_t i = 0; i < count; i++) {
		struct bt_iso_chan *iso_chan = param[i].iso_chan;
		struct bt_iso_cig *cig;

		if (iso_chan->state == BT_ISO_STATE_ENCRYPT_PENDING) {
			continue;
		}

		iso_chan->iso->iso.acl = bt_conn_ref(param[i].acl);
		bt_conn_set_state(iso_chan->iso, BT_CONN_CONNECTING);
		bt_iso_chan_set_state(iso_chan, BT_ISO_STATE_CONNECTING);

		cig = get_cig(iso_chan);
		__ASSERT(cig != NULL, "CIG was NULL");
		cig->state = BT_ISO_CIG_STATE_ACTIVE;
	}

	return 0;
}
#endif /* CONFIG_BT_ISO_CENTRAL */
#endif /* CONFIG_BT_ISO_UNICAST */

#if defined(CONFIG_BT_ISO_BROADCAST)
static struct bt_iso_big *lookup_big_by_handle(uint8_t big_handle)
{
	return &bigs[big_handle];
}

static struct bt_iso_big *get_free_big(void)
{
	/* We can use the index in the `bigs` array as BIG handles, for both
	 * broadcaster and receiver (even if the device is both!)
	 */

	for (size_t i = 0; i < ARRAY_SIZE(bigs); i++) {
		if (!atomic_test_and_set_bit(bigs[i].flags, BT_BIG_INITIALIZED)) {
			bigs[i].handle = i;
			sys_slist_init(&bigs[i].bis_channels);
			return &bigs[i];
		}
	}

	LOG_DBG("Could not allocate any more BIGs");

	return NULL;
}

static struct bt_iso_big *big_lookup_flag(int bit)
{
	for (size_t i = 0; i < ARRAY_SIZE(bigs); i++) {
		if (atomic_test_bit(bigs[i].flags, bit)) {
			return &bigs[i];
		}
	}

	LOG_DBG("No BIG with flag bit %d set", bit);

	return NULL;
}

static void cleanup_big(struct bt_iso_big *big)
{
	struct bt_iso_chan *bis, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&big->bis_channels, bis, tmp, node) {
		if (bis->iso != NULL) {
			bt_conn_unref(bis->iso);
			bis->iso = NULL;
		}

		sys_slist_remove(&big->bis_channels, NULL, &bis->node);
	}

	memset(big, 0, sizeof(*big));
}

static void big_disconnect(struct bt_iso_big *big, uint8_t reason)
{
	struct bt_iso_chan *bis;

	SYS_SLIST_FOR_EACH_CONTAINER(&big->bis_channels, bis, node) {
		bis->iso->err = reason;

		bt_iso_disconnected(bis->iso);
	}
}

static int big_init_bis(struct bt_iso_big *big,
			struct bt_iso_chan **bis_channels,
			uint8_t num_bis,
			bool broadcaster)
{
	for (uint8_t i = 0; i < num_bis; i++) {
		struct bt_iso_chan *bis = bis_channels[i];
		struct bt_conn_iso *iso_conn;

		bis->iso = iso_new();

		if (!bis->iso) {
			LOG_ERR("Unable to allocate BIS connection");
			return -ENOMEM;
		}
		iso_conn = &bis->iso->iso;

		iso_conn->big_handle = big->handle;
		iso_conn->info.type = broadcaster ? BT_ISO_CHAN_TYPE_BROADCASTER
						  : BT_ISO_CHAN_TYPE_SYNC_RECEIVER;
		iso_conn->bis_id = bt_conn_index(bis->iso);

		bt_iso_chan_add(bis->iso, bis);

		sys_slist_append(&big->bis_channels, &bis->node);
	}

	return 0;
}

#if defined(CONFIG_BT_ISO_BROADCASTER)
static int hci_le_create_big(struct bt_le_ext_adv *padv, struct bt_iso_big *big,
			     struct bt_iso_big_create_param *param)
{
	struct bt_hci_cp_le_create_big *req;
	struct bt_hci_cmd_state_set state;
	struct net_buf *buf;
	int err;
	static struct bt_iso_chan_qos *qos;
	struct bt_iso_chan *bis;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_CREATE_BIG, sizeof(*req));

	if (!buf) {
		return -ENOBUFS;
	}

	bis = SYS_SLIST_PEEK_HEAD_CONTAINER(&big->bis_channels, bis, node);
	__ASSERT(bis != NULL, "bis was NULL");

	/* All BIS will share the same QOS */
	qos = bis->qos;

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

	SYS_SLIST_FOR_EACH_CONTAINER(&big->bis_channels, bis, node) {
		bt_iso_chan_set_state(bis, BT_ISO_STATE_CONNECTING);
	}

	return err;
}

int bt_iso_big_create(struct bt_le_ext_adv *padv, struct bt_iso_big_create_param *param,
		      struct bt_iso_big **out_big)
{
	int err;
	struct bt_iso_big *big;

	if (!atomic_test_bit(padv->flags, BT_PER_ADV_PARAMS_SET)) {
		LOG_DBG("PA params not set; invalid adv object");
		return -EINVAL;
	}

	CHECKIF(!param->bis_channels) {
		LOG_DBG("NULL BIS channels");
		return -EINVAL;
	}

	CHECKIF(!param->num_bis) {
		LOG_DBG("Invalid number of BIS %u", param->num_bis);
		return -EINVAL;
	}

	for (uint8_t i = 0; i < param->num_bis; i++) {
		struct bt_iso_chan *bis = param->bis_channels[i];

		CHECKIF(bis == NULL) {
			LOG_DBG("bis_channels[%u]: NULL channel", i);
			return -EINVAL;
		}

		if (bis->iso) {
			LOG_DBG("bis_channels[%u]: already allocated", i);
			return -EALREADY;
		}

		CHECKIF(bis->qos == NULL) {
			LOG_DBG("bis_channels[%u]: qos is NULL", i);
			return -EINVAL;
		}

		CHECKIF(bis->qos->tx == NULL ||
			!valid_chan_io_qos(bis->qos->tx, true)) {
			LOG_DBG("bis_channels[%u]: Invalid QOS", i);
			return -EINVAL;
		}
	}

	CHECKIF(param->framing != BT_ISO_FRAMING_UNFRAMED &&
		param->framing != BT_ISO_FRAMING_FRAMED) {
		LOG_DBG("Invalid framing parameter: %u", param->framing);
		return -EINVAL;
	}

	CHECKIF(param->packing != BT_ISO_PACKING_SEQUENTIAL &&
		param->packing != BT_ISO_PACKING_INTERLEAVED) {
		LOG_DBG("Invalid packing parameter: %u", param->packing);
		return -EINVAL;
	}

	CHECKIF(param->num_bis > BT_ISO_MAX_GROUP_ISO_COUNT ||
		param->num_bis > CONFIG_BT_ISO_MAX_CHAN) {
		LOG_DBG("num_bis (%u) shall be lower than: %u", param->num_bis,
			MAX(CONFIG_BT_ISO_MAX_CHAN, BT_ISO_MAX_GROUP_ISO_COUNT));
		return -EINVAL;
	}

	CHECKIF(param->interval < BT_ISO_SDU_INTERVAL_MIN ||
		param->interval > BT_ISO_SDU_INTERVAL_MAX) {
		LOG_DBG("Invalid interval: %u", param->interval);
		return -EINVAL;
	}

	CHECKIF(param->latency < BT_ISO_LATENCY_MIN ||
		param->latency > BT_ISO_LATENCY_MAX) {
		LOG_DBG("Invalid latency: %u", param->latency);
		return -EINVAL;
	}

	big = get_free_big();

	if (!big) {
		return -ENOMEM;
	}

	err = big_init_bis(big, param->bis_channels, param->num_bis, true);
	if (err) {
		LOG_DBG("Could not init BIG %d", err);
		cleanup_big(big);
		return err;
	}
	big->num_bis = param->num_bis;

	err = hci_le_create_big(padv, big, param);
	if (err) {
		LOG_DBG("Could not create BIG %d", err);
		cleanup_big(big);
		return err;
	}

	*out_big = big;

	return err;
}

static void store_bis_broadcaster_info(const struct bt_hci_evt_le_big_complete *evt,
				       struct bt_iso_info *info)
{
	struct bt_iso_broadcaster_info *broadcaster_info = &info->broadcaster;

	info->iso_interval = sys_le16_to_cpu(evt->iso_interval);
	info->max_subevent = evt->nse;

	broadcaster_info->sync_delay = sys_get_le24(evt->sync_delay);
	broadcaster_info->latency = sys_get_le24(evt->latency);
	broadcaster_info->phy = bt_get_phy(evt->phy);
	broadcaster_info->bn = evt->bn;
	broadcaster_info->irc = evt->irc;
	/* Transform to n * 1.25ms */
	broadcaster_info->pto = info->iso_interval * evt->pto;
	broadcaster_info->max_pdu = sys_le16_to_cpu(evt->max_pdu);

	info->can_send = true;
	info->can_recv = false;
}

void hci_le_big_complete(struct net_buf *buf)
{
	struct bt_hci_evt_le_big_complete *evt = (void *)buf->data;
	struct bt_iso_chan *bis;
	struct bt_iso_big *big;
	int i;

	if (evt->big_handle >= ARRAY_SIZE(bigs)) {
		LOG_WRN("Invalid BIG handle");

		big = big_lookup_flag(BT_BIG_PENDING);
		if (big) {
			big_disconnect(big, evt->status ? evt->status : BT_HCI_ERR_UNSPECIFIED);
			cleanup_big(big);
		}

		return;
	}

	big = lookup_big_by_handle(evt->big_handle);
	atomic_clear_bit(big->flags, BT_BIG_PENDING);

	LOG_DBG("BIG[%u] %p completed, status %u", big->handle, big, evt->status);

	if (evt->status || evt->num_bis != big->num_bis) {
		if (evt->status == BT_HCI_ERR_SUCCESS && evt->num_bis != big->num_bis) {
			LOG_ERR("Invalid number of BIS created, was %u expected %u", evt->num_bis,
				big->num_bis);
		}
		big_disconnect(big, evt->status ? evt->status : BT_HCI_ERR_UNSPECIFIED);
		cleanup_big(big);
		return;
	}

	i = 0;
	SYS_SLIST_FOR_EACH_CONTAINER(&big->bis_channels, bis, node) {
		const uint16_t handle = evt->handle[i++];
		struct bt_conn *iso_conn = bis->iso;

		iso_conn->handle = sys_le16_to_cpu(handle);
		store_bis_broadcaster_info(evt, &iso_conn->iso.info);
		bt_conn_set_state(iso_conn, BT_CONN_CONNECTED);
	}
}

void hci_le_big_terminate(struct net_buf *buf)
{
	struct bt_hci_evt_le_big_terminate *evt = (void *)buf->data;
	struct bt_iso_big *big;

	if (evt->big_handle >= ARRAY_SIZE(bigs)) {
		LOG_WRN("Invalid BIG handle");
		return;
	}

	big = lookup_big_by_handle(evt->big_handle);

	LOG_DBG("BIG[%u] %p terminated", big->handle, big);

	big_disconnect(big, evt->reason);
	cleanup_big(big);
}
#endif /* CONFIG_BT_ISO_BROADCASTER */

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
	struct bt_iso_chan *bis;
	int err;

	if (!atomic_test_bit(big->flags, BT_BIG_INITIALIZED) || !big->num_bis) {
		LOG_DBG("BIG not initialized");
		return -EINVAL;
	}

	bis = SYS_SLIST_PEEK_HEAD_CONTAINER(&big->bis_channels, bis, node);
	__ASSERT(bis != NULL, "bis was NULL");

	if (IS_ENABLED(CONFIG_BT_ISO_BROADCASTER) &&
	    bis->iso->iso.info.type == BT_ISO_CHAN_TYPE_BROADCASTER) {
		err = hci_le_terminate_big(big);

		/* Wait for BT_HCI_EVT_LE_BIG_TERMINATE before cleaning up
		 * the BIG in hci_le_big_terminate
		 */
		if (!err) {
			SYS_SLIST_FOR_EACH_CONTAINER(&big->bis_channels, bis, node) {
				bt_iso_chan_set_state(bis, BT_ISO_STATE_DISCONNECTING);
			}
		}
	} else if (IS_ENABLED(CONFIG_BT_ISO_SYNC_RECEIVER) &&
		   bis->iso->iso.info.type == BT_ISO_CHAN_TYPE_SYNC_RECEIVER) {
		err = hci_le_big_sync_term(big);

		if (!err) {
			big_disconnect(big, BT_HCI_ERR_LOCALHOST_TERM_CONN);
			cleanup_big(big);
		}
	} else {
		err = -EINVAL;
	}

	if (err) {
		LOG_DBG("Could not terminate BIG %d", err);
	}

	return err;
}

#if defined(CONFIG_BT_ISO_SYNC_RECEIVER)
static void store_bis_sync_receiver_info(const struct bt_hci_evt_le_big_sync_established *evt,
					 struct bt_iso_info *info)
{
	struct bt_iso_sync_receiver_info *receiver_info = &info->sync_receiver;

	info->max_subevent = evt->nse;
	info->iso_interval = sys_le16_to_cpu(evt->iso_interval);

	receiver_info->latency = sys_get_le24(evt->latency);
	receiver_info->bn = evt->bn;
	receiver_info->irc = evt->irc;
	/* Transform to n * 1.25ms */
	receiver_info->pto = info->iso_interval * evt->pto;
	receiver_info->max_pdu = sys_le16_to_cpu(evt->max_pdu);

	info->can_send = false;
	info->can_recv = true;
}

void hci_le_big_sync_established(struct net_buf *buf)
{
	struct bt_hci_evt_le_big_sync_established *evt = (void *)buf->data;
	struct bt_iso_chan *bis;
	struct bt_iso_big *big;
	int i;

	if (evt->big_handle >= ARRAY_SIZE(bigs)) {
		LOG_WRN("Invalid BIG handle");
		big = big_lookup_flag(BT_BIG_SYNCING);
		if (big) {
			big_disconnect(big, evt->status ? evt->status : BT_HCI_ERR_UNSPECIFIED);
			cleanup_big(big);
		}

		return;
	}

	big = lookup_big_by_handle(evt->big_handle);
	atomic_clear_bit(big->flags, BT_BIG_SYNCING);

	LOG_DBG("BIG[%u] %p sync established, status %u", big->handle, big, evt->status);

	if (evt->status || evt->num_bis != big->num_bis) {
		if (evt->status == BT_HCI_ERR_SUCCESS && evt->num_bis != big->num_bis) {
			LOG_ERR("Invalid number of BIS synced, was %u expected %u", evt->num_bis,
				big->num_bis);
		}
		big_disconnect(big, evt->status ? evt->status : BT_HCI_ERR_UNSPECIFIED);
		cleanup_big(big);
		return;
	}

	i = 0;
	SYS_SLIST_FOR_EACH_CONTAINER(&big->bis_channels, bis, node) {
		const uint16_t handle = evt->handle[i++];
		struct bt_conn *iso_conn = bis->iso;

		iso_conn->handle = sys_le16_to_cpu(handle);
		store_bis_sync_receiver_info(evt, &iso_conn->iso.info);
		bt_conn_set_state(iso_conn, BT_CONN_CONNECTED);
	}
}

void hci_le_big_sync_lost(struct net_buf *buf)
{
	struct bt_hci_evt_le_big_sync_lost *evt = (void *)buf->data;
	struct bt_iso_big *big;

	if (evt->big_handle >= ARRAY_SIZE(bigs)) {
		LOG_WRN("Invalid BIG handle");
		return;
	}

	big = lookup_big_by_handle(evt->big_handle);

	LOG_DBG("BIG[%u] %p sync lost", big->handle, big);

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
				LOG_DBG("BIG cannot contain %u BISes", bit_idx + 1);
				return -EINVAL;
			}
			req->bis[bit_idx++] = i;
		}
	}

	if (bit_idx != big->num_bis) {
		LOG_DBG("Number of bits in bis_bitfield (%u) doesn't match num_bis (%u)", bit_idx,
			big->num_bis);
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
	struct bt_iso_chan *bis;
	struct bt_iso_big *big;

	if (!atomic_test_bit(sync->flags, BT_PER_ADV_SYNC_SYNCED)) {
		LOG_DBG("PA sync not synced");
		return -EINVAL;
	}

	CHECKIF(param->mse > BT_ISO_SYNC_MSE_MAX) {
		LOG_DBG("Invalid MSE 0x%02x", param->mse);
		return -EINVAL;
	}

	CHECKIF(param->sync_timeout < BT_ISO_SYNC_TIMEOUT_MIN ||
		param->sync_timeout > BT_ISO_SYNC_TIMEOUT_MAX) {
		LOG_DBG("Invalid sync timeout 0x%04x", param->sync_timeout);
		return -EINVAL;
	}

	CHECKIF(param->bis_bitfield <= BIT(0)) {
		LOG_DBG("Invalid BIS bitfield 0x%08x", param->bis_bitfield);
		return -EINVAL;
	}

	CHECKIF(!param->bis_channels) {
		LOG_DBG("NULL BIS channels");
		return -EINVAL;
	}

	CHECKIF(!param->num_bis) {
		LOG_DBG("Invalid number of BIS %u", param->num_bis);
		return -EINVAL;
	}

	for (uint8_t i = 0; i < param->num_bis; i++) {
		struct bt_iso_chan *bis = param->bis_channels[i];

		CHECKIF(bis == NULL) {
			LOG_DBG("bis_channels[%u]: NULL channel", i);
			return -EINVAL;
		}

		if (bis->iso) {
			LOG_DBG("bis_channels[%u]: already allocated", i);
			return -EALREADY;
		}

		CHECKIF(bis->qos == NULL) {
			LOG_DBG("bis_channels[%u]: qos is NULL", i);
			return -EINVAL;
		}

		CHECKIF(bis->qos->rx == NULL) {
			LOG_DBG("bis_channels[%u]: qos->rx is NULL", i);
			return -EINVAL;
		}
	}

	big = get_free_big();

	if (!big) {
		return -ENOMEM;
	}

	err = big_init_bis(big, param->bis_channels, param->num_bis, false);
	if (err) {
		LOG_DBG("Could not init BIG %d", err);
		cleanup_big(big);
		return err;
	}
	big->num_bis = param->num_bis;

	err = hci_le_big_create_sync(sync, big, param);
	if (err) {
		LOG_DBG("Could not create BIG sync %d", err);
		cleanup_big(big);
		return err;
	}


	SYS_SLIST_FOR_EACH_CONTAINER(&big->bis_channels, bis, node) {
		bt_iso_chan_set_state(bis, BT_ISO_STATE_CONNECTING);
	}

	*out_big = big;

	return 0;
}
#endif /* CONFIG_BT_ISO_SYNC_RECEIVER */
#endif /* CONFIG_BT_ISO_BROADCAST */
