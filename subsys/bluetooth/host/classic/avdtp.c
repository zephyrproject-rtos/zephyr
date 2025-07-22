/*
 * Audio Video Distribution Protocol
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/avdtp.h>

#include "host/hci_core.h"
#include "host/conn_internal.h"
#include "l2cap_br_internal.h"
#include "avdtp_internal.h"

#define LOG_LEVEL CONFIG_BT_AVDTP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_avdtp);

#define AVDTP_MSG_MASK GENMASK(1, 0)
#define AVDTP_PKT_MASK GENMASK(3, 2)
#define AVDTP_TID_MASK GENMASK(7, 4)
#define AVDTP_SIGID_MASK GENMASK(5, 0)

#define AVDTP_MSG_PREP(val) FIELD_PREP(AVDTP_MSG_MASK, val)
#define AVDTP_PKT_PREP(val) FIELD_PREP(AVDTP_PKT_MASK, val)
#define AVDTP_TID_PREP(val) FIELD_PREP(AVDTP_TID_MASK, val)
#define AVDTP_SIGID_PREP(val) FIELD_PREP(AVDTP_SIGID_MASK, val)

#define AVDTP_MSG_GET(hdr) FIELD_GET(AVDTP_MSG_MASK, hdr)
#define AVDTP_PKT_GET(hdr) FIELD_GET(AVDTP_PKT_MASK, hdr)
#define AVDTP_TID_GET(hdr) FIELD_GET(AVDTP_TID_MASK, hdr)
#define AVDTP_SIGID_GET(s) FIELD_GET(AVDTP_SIGID_MASK, s)

static struct bt_avdtp_event_cb *event_cb;
static sys_slist_t seps;

#define AVDTP_CHAN(_ch) CONTAINER_OF(_ch, struct bt_avdtp, br_chan.chan)

#define AVDTP_KWORK(_work)                                                                         \
	CONTAINER_OF(CONTAINER_OF(_work, struct k_work_delayable, work), struct bt_avdtp,          \
		     timeout_work)

#define DISCOVER_REQ(_req) CONTAINER_OF(_req, struct bt_avdtp_discover_params, req)
#define GET_CAP_REQ(_req)  CONTAINER_OF(_req, struct bt_avdtp_get_capabilities_params, req)
#define SET_CONF_REQ(_req) CONTAINER_OF(_req, struct bt_avdtp_set_configuration_params, req)
#define CTRL_REQ(_req)     CONTAINER_OF(_req, struct bt_avdtp_ctrl_params, req)

#define AVDTP_TIMEOUT K_SECONDS(6)

K_SEM_DEFINE(avdtp_sem_lock, 1U, 1U);

struct avdtp_buf_user_data {
	struct bt_avdtp *session;
	struct bt_avdtp_single_sig_hdr hdr;
	uint8_t frag_count;
	uint8_t current_frag;
} __packed;

#define AVDTP_POOL_USER_DATA_SIZE MAX(CONFIG_BT_CONN_TX_USER_DATA_SIZE,\
				      sizeof(struct avdtp_buf_user_data))

NET_BUF_POOL_DEFINE(avdtp_pool, CONFIG_BT_MAX_CONN * 2,
		    BT_L2CAP_BUF_SIZE(CONFIG_BT_AVDTP_SIGNAL_SDU_MAX),
		    AVDTP_POOL_USER_DATA_SIZE, NULL);

/* When allocating from acl_tx_pool fail, keep at least one buf to send data, then the sending
 * callback trigger the fragmentation process.
 */
NET_BUF_POOL_DEFINE(avdtp_frag_pool, 1,
		    BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
		    CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

/* tx list for packets */
sys_slist_t avdtp_tx_list = SYS_SLIST_STATIC_INIT(&avdtp_tx_list);
static void avdtp_tx_processor(struct k_work *item);
static K_WORK_DEFINE(avdtp_tx_work, avdtp_tx_processor);

enum sep_state {
	AVDTP_IDLE = BIT(0),
	AVDTP_CONFIGURED = BIT(1),
	/* establishing the transport sessions. */
	AVDTP_OPENING = BIT(2),
	AVDTP_OPEN = BIT(3),
	AVDTP_STREAMING = BIT(4),
	AVDTP_CLOSING = BIT(5),
	AVDTP_ABORTING = BIT(6),
};

static uint8_t avdtp_get_tid(struct bt_avdtp *session)
{
	return session->tid_sent++ % 16; /* Loop for 16 because only 4 bits */
}

static void avdtp_lock(struct bt_avdtp *session)
{
	k_sem_take(&session->sem_lock, K_FOREVER);
}

static void avdtp_unlock(struct bt_avdtp *session)
{
	k_sem_give(&session->sem_lock);
}

static void avdtp_sep_lock(struct bt_avdtp_sep *sep)
{
	if (sep != NULL) {
		k_sem_take(&sep->sem_lock, K_FOREVER);
	}
}

static void avdtp_sep_unlock(struct bt_avdtp_sep *sep)
{
	if (sep != NULL) {
		k_sem_give(&sep->sem_lock);
	}
}

static void bt_avdtp_set_state(struct bt_avdtp_sep *sep, uint8_t state)
{
	sep->state = state;

	if (state != AVDTP_IDLE) {
		sep->sep_info.inuse = 1U;
	} else {
		sep->sep_info.inuse = 0U;
	}
}

static void bt_avdtp_set_state_lock(struct bt_avdtp_sep *sep, uint8_t state)
{
	avdtp_sep_lock(sep);
	bt_avdtp_set_state(sep, state);
	avdtp_sep_unlock(sep);
}

static inline void bt_avdtp_clear_req(struct bt_avdtp *session)
{
	avdtp_lock(session);
	session->req = NULL;
	avdtp_unlock(session);
}

/* L2CAP Interface callbacks */
void bt_avdtp_media_l2cap_connected(struct bt_l2cap_chan *chan)
{
	struct bt_avdtp *session;
	struct bt_avdtp_sep *sep = CONTAINER_OF(chan, struct bt_avdtp_sep, chan.chan);

	if (!chan) {
		LOG_ERR("Invalid AVDTP chan");
		return;
	}

	session = sep->session;
	if (session == NULL) {
		return;
	}

	LOG_DBG("chan %p session %p", chan, session);
	bt_avdtp_set_state_lock(sep, AVDTP_OPEN);

	if (session->req != NULL) {
		struct bt_avdtp_req *req = session->req;

		req->status = BT_AVDTP_SUCCESS;
		bt_avdtp_clear_req(session);

		if (req->func != NULL) {
			req->func(req, NULL);
		}
	}
}

void bt_avdtp_media_l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_avdtp *session;
	struct bt_avdtp_sep *sep = CONTAINER_OF(chan, struct bt_avdtp_sep, chan.chan);

	session = sep->session;
	if (session == NULL) {
		return;
	}

	LOG_DBG("chan %p", chan);
	chan->conn = NULL;
	avdtp_sep_lock(sep);

	if ((sep->state == AVDTP_CLOSING) && (session->req != NULL) &&
	    (session->req->sig == BT_AVDTP_CLOSE)) {
		/* closing the stream */
		struct bt_avdtp_req *req = session->req;

		bt_avdtp_set_state(sep, AVDTP_IDLE);
		avdtp_sep_unlock(sep);
		req->status = BT_AVDTP_SUCCESS;
		bt_avdtp_clear_req(session);

		if (req->func != NULL) {
			req->func(req, NULL);
		}
	} else if (sep->state > AVDTP_OPENING) {
		bt_avdtp_set_state(sep, AVDTP_IDLE);
		avdtp_sep_unlock(sep);
		/* the l2cap is disconnected by other unexpected reasons */
		session->ops->stream_l2cap_disconnected(session, sep);
	} else {
		avdtp_sep_unlock(sep);
	}
}

int bt_avdtp_media_l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	/* media data is received */
	struct bt_avdtp_sep *sep = CONTAINER_OF(chan, struct bt_avdtp_sep, chan.chan);

	if (sep->media_data_cb != NULL) {
		sep->media_data_cb(sep, buf);
	}
	return 0;
}

static const struct bt_l2cap_chan_ops stream_chan_ops = {
	.connected = bt_avdtp_media_l2cap_connected,
	.disconnected = bt_avdtp_media_l2cap_disconnected,
	.recv = bt_avdtp_media_l2cap_recv,
};

static int avdtp_media_connect(struct bt_avdtp *session, struct bt_avdtp_sep *sep)
{
	if (!session) {
		return -EINVAL;
	}

	sep->session = session;
	sep->chan.rx.mtu = BT_L2CAP_RX_MTU;
	sep->chan.chan.ops = &stream_chan_ops;
	sep->chan.required_sec_level = BT_SECURITY_L2;

	return bt_l2cap_chan_connect(session->br_chan.chan.conn, &sep->chan.chan,
				     BT_L2CAP_PSM_AVDTP);
}

static int avdtp_media_disconnect(struct bt_avdtp_sep *sep)
{
	if (sep == NULL || sep->chan.chan.conn == NULL || sep->chan.chan.ops == NULL) {
		return -EINVAL;
	}

	return bt_l2cap_chan_disconnect(&sep->chan.chan);
}

static struct net_buf *avdtp_create_pdu(uint8_t msg_type, uint8_t sig_id, uint8_t tid)
{
	struct net_buf *buf;
	struct bt_avdtp_single_sig_hdr *hdr;

	LOG_DBG("");

	buf = bt_l2cap_create_pdu(&avdtp_pool, 0);
	if (!buf) {
		LOG_ERR("Error: No Buff available");
		return NULL;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));

	hdr->hdr = AVDTP_MSG_PREP(msg_type) | AVDTP_PKT_PREP(BT_AVDTP_PACKET_TYPE_SINGLE) |
		   AVDTP_TID_PREP(tid);
	hdr->signal_id = AVDTP_SIGID_PREP(sig_id);

	LOG_DBG("hdr = 0x%02X, Signal_ID = 0x%02X", hdr->hdr, hdr->signal_id);
	return buf;
}

static void avdtp_tx_raise(void)
{
	if (!sys_slist_is_empty(&avdtp_tx_list)) {
		LOG_DBG("kick TX");
		k_work_submit(&avdtp_tx_work);
	}
}

static void avdtp_tx_cb(struct bt_conn *conn, void *user_data, int err)
{
	avdtp_tx_raise();
}

static void avdtp_tx_remove(struct net_buf *buf)
{
	k_sem_take(&avdtp_sem_lock, K_FOREVER);
	sys_slist_find_and_remove(&avdtp_tx_list, &buf->node);
	k_sem_give(&avdtp_sem_lock);
}

static void avdtp_tx_rel_buf(struct net_buf *buf, struct net_buf *frag)
{
	avdtp_tx_remove(buf);
	net_buf_unref(buf);
	net_buf_unref(frag);

	avdtp_tx_raise();
}

static void avdtp_tx_signal(struct bt_avdtp *session, struct net_buf *buf)
{
	int err;

	avdtp_tx_remove(buf);
	err = bt_l2cap_br_chan_send_cb(&session->br_chan.chan, buf, avdtp_tx_cb, NULL);

	if (err != 0) {
		LOG_ERR("Failed to send l2cap: err=%d", err);
		net_buf_unref(buf);
	}
}

static void avdtp_tx_frags(struct bt_avdtp *session, struct net_buf *buf,
			   struct avdtp_buf_user_data *user_data)
{
	struct net_buf *frag;
	uint16_t mtu;
	int err;

	mtu = MIN(session->br_chan.tx.mtu, CONFIG_BT_L2CAP_TX_MTU);

	while (user_data->frag_count > user_data->current_frag && buf->len > 0) {
		uint16_t len;

		frag = bt_l2cap_create_pdu_timeout(NULL, 0, K_NO_WAIT);
		if (frag == NULL) {
			frag = bt_l2cap_create_pdu_timeout(&avdtp_frag_pool, 0, K_NO_WAIT);
		}

		if (frag == NULL) {
			LOG_DBG("No Buff available, wait tx cb to trigger this work again");
			return;
		}

		user_data->current_frag++;

		if (user_data->current_frag == 1) {
			struct bt_avdtp_start_sig_hdr *start_hdr;
			struct bt_avdtp_single_sig_hdr *sig_hdr;

			sig_hdr = net_buf_pull_mem(buf, sizeof(*sig_hdr));
			user_data->hdr = *sig_hdr;

			start_hdr = net_buf_add(frag, sizeof(*start_hdr));
			/* use same transaction label and message type */
			start_hdr->hdr = (user_data->hdr.hdr & ~AVDTP_PKT_MASK) |
					AVDTP_PKT_PREP(BT_AVDTP_PACKET_TYPE_START);
			start_hdr->num_of_signal_pkts = user_data->frag_count;
			start_hdr->signal_id = user_data->hdr.signal_id;

			len = mtu - sizeof(*start_hdr);
			if (len >= buf->len) {
				LOG_ERR("The start packet can send all data");
				avdtp_tx_rel_buf(buf, frag);
				return;
			}

			net_buf_add_mem(frag, net_buf_pull_mem(buf, len), len);
		} else {
			struct bt_avdtp_continue_end_sig_hdr *cont_hdr;
			uint8_t pkt_type = (user_data->frag_count == user_data->current_frag) ?
					   BT_AVDTP_PACKET_TYPE_END : BT_AVDTP_PACKET_TYPE_CONTINUE;

			cont_hdr = net_buf_add(frag, sizeof(*cont_hdr));
			/* use same transaction label and message type */
			cont_hdr->hdr = (user_data->hdr.hdr & ~AVDTP_PKT_MASK) |
					AVDTP_PKT_PREP(pkt_type);

			len = mtu - sizeof(*cont_hdr);
			if (pkt_type == BT_AVDTP_PACKET_TYPE_CONTINUE && len >= buf->len) {
				LOG_ERR("The continue packet can send all data");
				avdtp_tx_rel_buf(buf, frag);
				return;
			} else if (pkt_type == BT_AVDTP_PACKET_TYPE_END && len < buf->len) {
				LOG_ERR("The end packet can't send all data");
				avdtp_tx_rel_buf(buf, frag);
				return;
			}

			len = MIN(len, buf->len);
			net_buf_add_mem(frag, net_buf_pull_mem(buf, len), len);
		}

		LOG_DBG("Sending fragment %d: len=%d", user_data->current_frag, frag->len);
		err = bt_l2cap_br_chan_send_cb(&session->br_chan.chan, frag, avdtp_tx_cb, NULL);
		if (err != 0) {
			LOG_ERR("Failed to send fragment: err=%d", err);
			avdtp_tx_rel_buf(buf, frag);
			return;
		}
	}

	if ((user_data->frag_count <= user_data->current_frag) || (buf->len == 0)) {
		avdtp_tx_remove(buf);
		net_buf_unref(buf);
	}
}

static void avdtp_tx_processor(struct k_work *item)
{
	struct bt_avdtp *session;
	struct avdtp_buf_user_data *user_data;
	const sys_snode_t *node;
	struct net_buf *buf;

	k_sem_take(&avdtp_sem_lock, K_FOREVER);
	node = sys_slist_peek_head(&avdtp_tx_list);
	if (node == NULL) {
		k_sem_give(&avdtp_sem_lock);
		LOG_DBG("No pending tx");
		return;
	}
	k_sem_give(&avdtp_sem_lock);

	buf = CONTAINER_OF(node, struct net_buf, node);
	user_data = net_buf_user_data(buf);

	session = user_data->session;
	__ASSERT_NO_MSG(session != NULL);

	/* The buf can be sent directly */
	if (user_data->frag_count == 1) {
		avdtp_tx_signal(session, buf);
		avdtp_tx_raise();
		return;
	}

	avdtp_tx_frags(session, buf, user_data);

	avdtp_tx_raise();
}

static void avdtp_buf_init_user_data(struct bt_avdtp *session, struct net_buf *buf)
{
	struct avdtp_buf_user_data *user_data;

	user_data = net_buf_user_data(buf);
	user_data->session = session;
	user_data->current_frag = 0;

	if (buf->len > session->br_chan.tx.mtu) {
		uint8_t num_of_frag_pkts;
		uint16_t sdu_len;
		uint16_t continue_pkt_size;
		uint16_t mtu = MIN(session->br_chan.tx.mtu, CONFIG_BT_L2CAP_TX_MTU);

		/* First start packet's header is different */
		sdu_len = buf->len - sizeof(struct bt_avdtp_single_sig_hdr) +
			  sizeof(struct bt_avdtp_start_sig_hdr);
		sdu_len -= mtu;

		/* Calculate the number of signal packets needed */
		continue_pkt_size = mtu - sizeof(struct bt_avdtp_continue_end_sig_hdr);
		num_of_frag_pkts = 1 + (sdu_len + continue_pkt_size - 1) / continue_pkt_size;
		LOG_DBG("Fragmenting buffer: len=%d, packets=%d", buf->len, num_of_frag_pkts);

		user_data->frag_count = num_of_frag_pkts;
	} else {
		user_data->frag_count = 1;
	}
}

static void avdtp_send_common(struct bt_avdtp *session, struct net_buf *buf)
{
	avdtp_buf_init_user_data(session, buf);
	k_sem_take(&avdtp_sem_lock, K_FOREVER);
	sys_slist_append(&avdtp_tx_list, &buf->node);
	k_sem_give(&avdtp_sem_lock);

	avdtp_tx_raise();
}

static int avdtp_send_rsp(struct bt_avdtp *session, struct net_buf *buf)
{
	/* From all the calls, the session and buf can't be NULL. */
	__ASSERT_NO_MSG((session != NULL && buf != NULL));

	LOG_DBG("Response sent: buf=%p", buf);

	avdtp_send_common(session, buf);

	return 0;
}

static void avdtp_set_status(struct bt_avdtp_req *req, struct net_buf *buf, uint8_t msg_type)
{
	if (msg_type == BT_AVDTP_ACCEPT) {
		req->status = BT_AVDTP_SUCCESS;
	} else if (msg_type == BT_AVDTP_REJECT) {
		if (buf->len >= 1U) {
			req->status = net_buf_pull_u8(buf);
		} else {
			LOG_WRN("Invalid RSP frame");
			req->status = BT_AVDTP_BAD_LENGTH;
		}
	} else if (msg_type == BT_AVDTP_GEN_REJECT) {
		req->status = BT_AVDTP_NOT_SUPPORTED_COMMAND;
	} else {
		req->status = BT_AVDTP_BAD_HEADER_FORMAT;
	}
}

static void avdtp_discover_cmd(struct bt_avdtp *session, struct net_buf *buf, uint8_t tid)
{
	int err;
	struct bt_avdtp_sep *sep;
	struct net_buf *rsp_buf;
	uint8_t error_code = 0;

	if (session->ops->discovery_ind == NULL) {
		err = -ENOTSUP;
	} else {
		err = session->ops->discovery_ind(session, &error_code);
	}

	rsp_buf = avdtp_create_pdu(err ? BT_AVDTP_REJECT : BT_AVDTP_ACCEPT, BT_AVDTP_DISCOVER, tid);
	if (!rsp_buf) {
		return;
	}

	if (err) {
		if (error_code == 0) {
			error_code = BT_AVDTP_BAD_STATE;
		}

		LOG_DBG("discover err code:%d", error_code);
		net_buf_add_u8(rsp_buf, error_code);
	} else {
		struct bt_avdtp_sep_data sep_data;

		SYS_SLIST_FOR_EACH_CONTAINER(&seps, sep, _node) {
			memset(&sep_data, 0, sizeof(sep_data));
			sep_data.inuse = sep->sep_info.inuse;
			sep_data.id = sep->sep_info.id;
			sep_data.tsep = sep->sep_info.tsep;
			sep_data.media_type = sep->sep_info.media_type;
			net_buf_add_mem(rsp_buf, &sep_data, sizeof(sep_data));
		}
	}

	(void)avdtp_send_rsp(session, rsp_buf);
}

static void avdtp_discover_rsp(struct bt_avdtp *session, struct net_buf *buf, uint8_t msg_type)
{
	struct bt_avdtp_req *req = session->req;

	if (req == NULL) {
		return;
	}

	k_work_cancel_delayable(&session->timeout_work);
	avdtp_set_status(req, buf, msg_type);
	bt_avdtp_clear_req(session);

	if (req->func != NULL) {
		req->func(req, buf);
	}
}

static struct bt_avdtp_sep *avdtp_get_sep(uint8_t stream_endpoint_id)
{
	struct bt_avdtp_sep *sep = NULL;

	SYS_SLIST_FOR_EACH_CONTAINER(&seps, sep, _node) {
		if (sep->sep_info.id == stream_endpoint_id) {
			break;
		}
	}

	return sep;
}

static struct bt_avdtp_sep *avdtp_get_cmd_sep(struct net_buf *buf, uint8_t *error_code,
					      uint8_t *seid)
{
	struct bt_avdtp_sep *sep;
	uint8_t id;

	if (buf->len < 1U) {
		*error_code = BT_AVDTP_BAD_LENGTH;
		LOG_WRN("Malformed packet");
		return NULL;
	}

	id = net_buf_pull_u8(buf) >> 2;
	if ((id < BT_AVDTP_MIN_SEID) || (id > BT_AVDTP_MAX_SEID)) {
		*error_code = BT_AVDTP_BAD_ACP_SEID;
		LOG_WRN("Invalid ACP SEID");
		return NULL;
	}

	if (seid != NULL) {
		*seid = id;
	}

	sep = avdtp_get_sep(id);
	return sep;
}

static void avdtp_get_caps_cmd_internal(struct bt_avdtp *session, struct net_buf *buf, uint8_t tid,
					bool get_all_caps)
{
	int err = 0;
	struct net_buf *rsp_buf;
	struct bt_avdtp_sep *sep;
	uint8_t error_code = 0;

	sep = avdtp_get_cmd_sep(buf, &error_code, NULL);

	if ((sep == NULL) || (session->ops->get_capabilities_ind == NULL)) {
		err = -ENOTSUP;
	} else {
		rsp_buf = avdtp_create_pdu(BT_AVDTP_ACCEPT, get_all_caps ?
					   BT_AVDTP_GET_ALL_CAPABILITIES :
					   BT_AVDTP_GET_CAPABILITIES, tid);
		if (!rsp_buf) {
			return;
		}

		err = session->ops->get_capabilities_ind(session, sep, rsp_buf, get_all_caps,
							 &error_code);
		if (err) {
			net_buf_unref(rsp_buf);
		}
	}

	if (err) {
		rsp_buf = avdtp_create_pdu(BT_AVDTP_REJECT, get_all_caps ?
					   BT_AVDTP_GET_ALL_CAPABILITIES :
					   BT_AVDTP_GET_CAPABILITIES, tid);
		if (!rsp_buf) {
			return;
		}

		if (error_code == 0) {
			error_code = BT_AVDTP_BAD_ACP_SEID;
		}

		LOG_DBG("get cap err code:%d", error_code);
		net_buf_add_u8(rsp_buf, error_code);
	}

	(void)avdtp_send_rsp(session, rsp_buf);
}

static void avdtp_get_capabilities_cmd(struct bt_avdtp *session, struct net_buf *buf, uint8_t tid)
{
	avdtp_get_caps_cmd_internal(session, buf, tid, false);
}

static void avdtp_get_all_capabilities_cmd(struct bt_avdtp *session,
					   struct net_buf *buf, uint8_t tid)
{
	avdtp_get_caps_cmd_internal(session, buf, tid, true);
}

static void avdtp_get_capabilities_rsp(struct bt_avdtp *session, struct net_buf *buf,
				       uint8_t msg_type)
{
	struct bt_avdtp_req *req = session->req;

	if (req == NULL) {
		return;
	}

	k_work_cancel_delayable(&session->timeout_work);
	avdtp_set_status(req, buf, msg_type);
	bt_avdtp_clear_req(session);

	if (req->func != NULL) {
		req->func(req, buf);
	}
}

struct bt_avdtp_service_category_handler {
	uint8_t min_len;
	uint8_t max_len;
	bool reconfig_support;
	uint8_t err_code;
	uint8_t (*handler)(struct net_buf *buf);
};

uint8_t bt_avdtp_check_media_recovery_type(struct net_buf *buf)
{
	struct bt_avdtp_recovery_capabilities *cap
				= (struct bt_avdtp_recovery_capabilities *)buf->data;

	if (cap->recovery_type != BT_AVDTP_RECOVERY_TYPE_FORBIDDEN &&
	    cap->recovery_type != BT_ADVTP_RECOVERY_TYPE_RFC2733) {
		return BT_AVDTP_BAD_RECOVERY_TYPE;
	}
	return BT_AVDTP_SUCCESS;
}

static struct bt_avdtp_service_category_handler category_handler[]  = {
	{0, 0, false, 0, NULL},                             /*None*/
	{0, 0, false, BT_AVDTP_BAD_MEDIA_TRANSPORT_FORMAT,
	 NULL},                                             /*BT_AVDTP_SERVICE_MEDIA_TRANSPORT*/
	{0, 0, false, BT_AVDTP_BAD_LENGTH, NULL},           /*BT_AVDTP_SERVICE_REPORTING*/
	{sizeof(struct bt_avdtp_recovery_capabilities),
	 sizeof(struct bt_avdtp_recovery_capabilities), false, BT_AVDTP_BAD_RECOVERY_FORMAT,
	 bt_avdtp_check_media_recovery_type},               /*BT_AVDTP_SERVICE_MEDIA_RECOVERY*/
	{sizeof(struct bt_avdtp_content_protection_capabilities), UINT8_MAX,
	 true, BT_AVDTP_BAD_CP_FORMAT, NULL},             /*BT_AVDTP_SERVICE_CONTENT_PROTECTION*/
	{sizeof(struct bt_avdtp_header_compression_capabilities),
	 sizeof(struct bt_avdtp_header_compression_capabilities),
	 false, BT_AVDTP_BAD_LENGTH, NULL},               /*BT_AVDTP_SERVICE_HEADER_COMPRESSION*/
	{sizeof(struct bt_avdtp_multiplexing_capabilities),
	 sizeof(struct bt_avdtp_multiplexing_capabilities),
	 false, BT_AVDTP_BAD_MULTIPLEXING_FORMAT, NULL},  /*BT_AVDTP_SERVICE_MULTIPLEXING*/
	{sizeof(struct bt_avdtp_media_codec_capabilities), UINT8_MAX,
	 true, BT_AVDTP_BAD_LENGTH, NULL},                /*BT_AVDTP_SERVICE_MEDIA_CODEC*/
	{0, 0, 0, BT_AVDTP_BAD_LENGTH, NULL},             /*BT_AVDTP_SERVICE_DELAY_REPORTING*/
};

uint8_t bt_avdtp_check_service_category(struct net_buf *buf, uint8_t *service_category,
					bool reconfig)
{
	struct bt_avdtp_generic_service_cap *hdr;
	uint8_t err;
	struct bt_avdtp_service_category_handler *handler;
	struct net_buf_simple_state state;

	if (buf->len == 0U) {
		LOG_DBG("Error: buf not valid");
		return BT_AVDTP_BAD_LENGTH;
	}

	while (buf->len > 0U) {
		if (buf->len < sizeof(*hdr)) {
			LOG_DBG("Error: buf not valid");
			return BT_AVDTP_BAD_LENGTH;
		}

		hdr = net_buf_pull_mem(buf, sizeof(*hdr));
		*service_category = hdr->service_category;

		if (hdr->service_category != 0 &&
		    hdr->service_category < ARRAY_SIZE(category_handler)) {
			handler = &category_handler[hdr->service_category];

			if (hdr->losc > buf->len || hdr->losc > handler->max_len ||
			    hdr->losc < handler->min_len) {
				return handler->err_code;
			}

			if (!handler->reconfig_support && reconfig) {
				return BT_AVDTP_INVALID_CAPABILITIES;
			}

			if (handler->handler != NULL) {
				net_buf_simple_save(&buf->b, &state);
				err = handler->handler(buf);
				net_buf_simple_restore(&buf->b, &state);
				if (err != BT_AVDTP_SUCCESS) {
					return err;
				}
			}
			net_buf_pull_mem(buf, hdr->losc);
		} else {
			return BT_AVDTP_BAD_SERV_CATEGORY;
		}
	}

	return BT_AVDTP_SUCCESS;
}

static void avdtp_process_configuration_cmd(struct bt_avdtp *session, struct net_buf *buf,
					    uint8_t tid, bool reconfig)
{
	int err = 0;
	struct bt_avdtp_sep *sep;
	struct net_buf *rsp_buf;
	uint8_t avdtp_err_code = 0;
	struct net_buf_simple_state state;
	uint8_t service_category = 0;

	sep = avdtp_get_cmd_sep(buf, &avdtp_err_code, NULL);
	avdtp_sep_lock(sep);

	if (sep == NULL) {
		err = -ENOTSUP;
	} else if (!reconfig && session->ops->set_configuration_ind == NULL) {
		err = -ENOTSUP;
	} else if (reconfig && session->ops->re_configuration_ind == NULL) {
		err = -ENOTSUP;
	} else if (!reconfig && sep->sep_info.inuse == 1) {
		avdtp_err_code = BT_AVDTP_SEP_IN_USE;
		err = -EBUSY;
	} else {
		uint8_t expected_state;

		if (reconfig) {
			expected_state = AVDTP_OPEN | AVDTP_OPENING;
		} else {
			expected_state = AVDTP_IDLE;
		}

		if (!(sep->state & expected_state)) {
			err = -ENOTSUP;
			avdtp_err_code = BT_AVDTP_BAD_STATE;
		} else if (buf->len >= 1U) {
			uint8_t int_seid = 0;
			uint8_t err_code = 0;

			/* INT Stream Endpoint ID */
			if (!reconfig) {
				/* int seid not in reconfig cmd*/
				int_seid = net_buf_pull_u8(buf) >> 2;
			}
			net_buf_simple_save(&buf->b, &state);
			err_code = bt_avdtp_check_service_category(buf, &service_category,
								   reconfig);
			net_buf_simple_restore(&buf->b, &state);
			if (err_code) {
				avdtp_err_code = err_code;
				err = -ENOTSUP;
			} else {
				if (!reconfig) {
					err = session->ops->set_configuration_ind(
						session, sep, int_seid, buf, &avdtp_err_code);
				} else {
					err = session->ops->re_configuration_ind(session, sep, buf,
										 &avdtp_err_code);
				}
			}
		} else {
			LOG_WRN("Invalid INT SEID");
			err = -ENOTSUP;
			avdtp_err_code = BT_AVDTP_BAD_LENGTH;
		}
	}

	rsp_buf = avdtp_create_pdu(err ? BT_AVDTP_REJECT : BT_AVDTP_ACCEPT, reconfig ?
				   BT_AVDTP_RECONFIGURE : BT_AVDTP_SET_CONFIGURATION, tid);
	if (!rsp_buf) {
		avdtp_sep_unlock(sep);
		return;
	}

	if (err) {
		if (avdtp_err_code == 0) {
			avdtp_err_code = BT_AVDTP_BAD_ACP_SEID;
		}

		LOG_DBG("set configuration err code:%d", avdtp_err_code);
		/* error Service Category*/
		net_buf_add_u8(rsp_buf, service_category);
		/* ERROR CODE */
		net_buf_add_u8(rsp_buf, avdtp_err_code);
	}

	err = avdtp_send_rsp(session, rsp_buf);

	if (!reconfig && !err && !avdtp_err_code) {
		bt_avdtp_set_state(sep, AVDTP_CONFIGURED);
	}

	avdtp_sep_unlock(sep);
}

static void avdtp_process_configuration_rsp(struct bt_avdtp *session, struct net_buf *buf,
					    uint8_t msg_type, bool reconfig)
{
	struct bt_avdtp_req *req = session->req;

	if (req == NULL) {
		return;
	}

	k_work_cancel_delayable(&session->timeout_work);

	if (msg_type == BT_AVDTP_ACCEPT) {
		if (!reconfig) {
			bt_avdtp_set_state_lock(SET_CONF_REQ(req)->sep, AVDTP_CONFIGURED);
		}
	} else if (msg_type == BT_AVDTP_REJECT) {
		if (buf->len < 1U) {
			LOG_WRN("Invalid RSP frame");
			req->status = BT_AVDTP_BAD_LENGTH;
		} else {
			/* Service Category */
			net_buf_pull_u8(buf);
		}
	}

	if (req->status == BT_AVDTP_SUCCESS) {
		avdtp_set_status(req, buf, msg_type);
	}

	bt_avdtp_clear_req(session);

	if (req->func != NULL) {
		req->func(req, NULL);
	}
}

static void avdtp_set_configuration_cmd(struct bt_avdtp *session, struct net_buf *buf, uint8_t tid)
{
	avdtp_process_configuration_cmd(session, buf, tid, false);
}

static void avdtp_set_configuration_rsp(struct bt_avdtp *session, struct net_buf *buf,
					uint8_t msg_type)
{
	avdtp_process_configuration_rsp(session, buf, msg_type, false);
}

static void avdtp_re_configure_cmd(struct bt_avdtp *session, struct net_buf *buf, uint8_t tid)
{
	avdtp_process_configuration_cmd(session, buf, tid, true);
}

static void avdtp_re_configure_rsp(struct bt_avdtp *session, struct net_buf *buf, uint8_t msg_type)
{
	avdtp_process_configuration_rsp(session, buf, msg_type, true);
}

static void avdtp_open_cmd(struct bt_avdtp *session, struct net_buf *buf, uint8_t tid)
{
	int err = 0;
	struct bt_avdtp_sep *sep;
	struct net_buf *rsp_buf;
	uint8_t avdtp_err_code = 0;

	sep = avdtp_get_cmd_sep(buf, &avdtp_err_code, NULL);
	avdtp_sep_lock(sep);

	if ((sep == NULL) || (session->ops->open_ind == NULL)) {
		err = -ENOTSUP;
	} else {
		if (sep->state != AVDTP_CONFIGURED) {
			err = -ENOTSUP;
			avdtp_err_code = BT_AVDTP_BAD_STATE;
		} else {
			err = session->ops->open_ind(session, sep, &avdtp_err_code);
		}
	}

	rsp_buf = avdtp_create_pdu(err ? BT_AVDTP_REJECT : BT_AVDTP_ACCEPT, BT_AVDTP_OPEN, tid);
	if (!rsp_buf) {
		avdtp_sep_unlock(sep);
		return;
	}

	if (err) {
		if (avdtp_err_code == 0) {
			avdtp_err_code = BT_AVDTP_BAD_ACP_SEID;
		}

		LOG_DBG("open_ind err code:%d", avdtp_err_code);
		net_buf_add_u8(rsp_buf, avdtp_err_code);
	} else {
		session->current_sep = sep;
	}

	err = avdtp_send_rsp(session, rsp_buf);

	if (!err && !avdtp_err_code) {
		bt_avdtp_set_state(sep, AVDTP_OPENING);
	}

	avdtp_sep_unlock(sep);
}

static void avdtp_open_rsp(struct bt_avdtp *session, struct net_buf *buf, uint8_t msg_type)
{
	struct bt_avdtp_req *req = session->req;

	if (req == NULL) {
		return;
	}

	k_work_cancel_delayable(&session->timeout_work);
	avdtp_set_status(req, buf, msg_type);

	if (req->status == BT_AVDTP_SUCCESS) {
		bt_avdtp_set_state_lock(CTRL_REQ(req)->sep, AVDTP_OPENING);

		/* wait the media l2cap is established */
		if (!avdtp_media_connect(session, CTRL_REQ(req)->sep)) {
			return;
		}
	}

	if (req->status != BT_AVDTP_SUCCESS) {
		bt_avdtp_clear_req(session);

		if (req->func != NULL) {
			req->func(req, NULL);
		}
	}
}

static void avdtp_handle_reject(struct net_buf *buf, struct bt_avdtp_req *req)
{
	if (buf->len > 1U) {
		uint8_t acp_seid;

		acp_seid = net_buf_pull_u8(buf);

		if (acp_seid != CTRL_REQ(req)->acp_stream_ep_id) {
			req->status = BT_AVDTP_BAD_ACP_SEID;
		}
	} else {
		req->status = BT_AVDTP_BAD_LENGTH;
	}
}

static void avdtp_start_cmd(struct bt_avdtp *session, struct net_buf *buf, uint8_t tid)
{
	int err = 0;
	struct bt_avdtp_sep *sep;
	struct net_buf *rsp_buf;
	uint8_t avdtp_err_code = 0;
	uint8_t acp_seid = 0;

	sep = avdtp_get_cmd_sep(buf, &avdtp_err_code, &acp_seid);

	avdtp_sep_lock(sep);

	if ((sep == NULL) || (session->ops->start_ind == NULL)) {
		err = -ENOTSUP;
	} else {
		if (sep->state != AVDTP_OPEN) {
			err = -ENOTSUP;
			avdtp_err_code = BT_AVDTP_BAD_STATE;
		} else {
			err = session->ops->start_ind(session, sep, &avdtp_err_code);
		}
	}

	rsp_buf = avdtp_create_pdu(err ? BT_AVDTP_REJECT : BT_AVDTP_ACCEPT, BT_AVDTP_START, tid);
	if (!rsp_buf) {
		avdtp_sep_unlock(sep);
		return;
	}

	if (err) {
		if (avdtp_err_code == 0) {
			avdtp_err_code = BT_AVDTP_BAD_ACP_SEID;
		}

		LOG_DBG("start err code:%d", avdtp_err_code);
		net_buf_add_u8(rsp_buf, acp_seid);
		net_buf_add_u8(rsp_buf, avdtp_err_code);
	}

	err = avdtp_send_rsp(session, rsp_buf);

	if (!err && !avdtp_err_code) {
		bt_avdtp_set_state(sep, AVDTP_STREAMING);
	}

	avdtp_sep_unlock(sep);
}

static void avdtp_start_rsp(struct bt_avdtp *session, struct net_buf *buf, uint8_t msg_type)
{
	struct bt_avdtp_req *req = session->req;

	if (req == NULL) {
		return;
	}

	k_work_cancel_delayable(&session->timeout_work);

	if (msg_type == BT_AVDTP_ACCEPT) {
		bt_avdtp_set_state_lock(CTRL_REQ(req)->sep, AVDTP_STREAMING);
	} else if (msg_type == BT_AVDTP_REJECT) {
		avdtp_handle_reject(buf, req);
	}

	if (req->status == BT_AVDTP_SUCCESS) {
		avdtp_set_status(req, buf, msg_type);
	}

	bt_avdtp_clear_req(session);

	if (req->func != NULL) {
		req->func(req, NULL);
	}
}

static void avdtp_close_cmd(struct bt_avdtp *session, struct net_buf *buf, uint8_t tid)
{
	int err = 0;
	struct bt_avdtp_sep *sep;
	struct net_buf *rsp_buf;
	uint8_t avdtp_err_code = 0;

	sep = avdtp_get_cmd_sep(buf, &avdtp_err_code, NULL);
	avdtp_sep_lock(sep);

	if ((sep == NULL) || (session->ops->close_ind == NULL)) {
		err = -ENOTSUP;
	} else {
		if (!(sep->state & (AVDTP_OPEN | AVDTP_STREAMING))) {
			err = -ENOTSUP;
			avdtp_err_code = BT_AVDTP_BAD_STATE;
		} else {
			err = session->ops->close_ind(session, sep, &avdtp_err_code);
		}
	}

	rsp_buf = avdtp_create_pdu(err ? BT_AVDTP_REJECT : BT_AVDTP_ACCEPT, BT_AVDTP_CLOSE, tid);
	if (!rsp_buf) {
		avdtp_sep_unlock(sep);
		return;
	}

	if (err) {
		if (avdtp_err_code == 0) {
			avdtp_err_code = BT_AVDTP_BAD_ACP_SEID;
		}

		LOG_DBG("close err code:%d", avdtp_err_code);
		net_buf_add_u8(rsp_buf, avdtp_err_code);
	} else {
		bt_avdtp_set_state(sep, AVDTP_CLOSING);
	}

	err = avdtp_send_rsp(session, rsp_buf);

	if (!err && !avdtp_err_code) {
		bt_avdtp_set_state(sep, AVDTP_IDLE);
	}

	avdtp_sep_unlock(sep);
}

static void avdtp_close_rsp(struct bt_avdtp *session, struct net_buf *buf, uint8_t msg_type)
{
	struct bt_avdtp_req *req = session->req;

	if (req == NULL) {
		return;
	}

	k_work_cancel_delayable(&session->timeout_work);
	avdtp_set_status(req, buf, msg_type);

	if (req->status == BT_AVDTP_SUCCESS) {
		bt_avdtp_set_state_lock(CTRL_REQ(req)->sep, AVDTP_CLOSING);

		if (!avdtp_media_disconnect(CTRL_REQ(req)->sep)) {
			return;
		}
	}

	bt_avdtp_clear_req(session);

	if (req->func != NULL) {
		req->func(req, NULL);
	}
}

static void avdtp_suspend_cmd(struct bt_avdtp *session, struct net_buf *buf, uint8_t tid)
{
	int err = 0;
	struct bt_avdtp_sep *sep;
	struct net_buf *rsp_buf;
	uint8_t avdtp_err_code = 0;
	uint8_t acp_seid = 0;

	sep = avdtp_get_cmd_sep(buf, &avdtp_err_code, &acp_seid);
	avdtp_sep_lock(sep);

	if ((sep == NULL) || (session->ops->suspend_ind == NULL)) {
		err = -ENOTSUP;
	} else {
		if (sep->state != AVDTP_STREAMING) {
			err = -ENOTSUP;
			avdtp_err_code = BT_AVDTP_BAD_STATE;
		} else {
			err = session->ops->suspend_ind(session, sep, &avdtp_err_code);
		}
	}

	rsp_buf = avdtp_create_pdu(err ? BT_AVDTP_REJECT : BT_AVDTP_ACCEPT, BT_AVDTP_SUSPEND, tid);
	if (!rsp_buf) {
		avdtp_sep_unlock(sep);
		return;
	}

	if (err) {
		if (avdtp_err_code == 0) {
			avdtp_err_code = BT_AVDTP_BAD_ACP_SEID;
		}

		LOG_DBG("suspend err code:%d", avdtp_err_code);
		net_buf_add_u8(rsp_buf, acp_seid);
		net_buf_add_u8(rsp_buf, avdtp_err_code);
	}

	err = avdtp_send_rsp(session, rsp_buf);

	if (!err && !avdtp_err_code) {
		bt_avdtp_set_state(sep, AVDTP_OPEN);
	}

	avdtp_sep_unlock(sep);
}

static void avdtp_suspend_rsp(struct bt_avdtp *session, struct net_buf *buf, uint8_t msg_type)
{
	struct bt_avdtp_req *req = session->req;

	if (req == NULL) {
		return;
	}

	k_work_cancel_delayable(&session->timeout_work);

	if (msg_type == BT_AVDTP_ACCEPT) {
		bt_avdtp_set_state_lock(CTRL_REQ(req)->sep, AVDTP_OPEN);
	} else if (msg_type == BT_AVDTP_REJECT) {
		avdtp_handle_reject(buf, req);
	}

	if (req->status == BT_AVDTP_SUCCESS) {
		avdtp_set_status(req, buf, msg_type);
	}

	bt_avdtp_clear_req(session);

	if (req->func != NULL) {
		req->func(req, NULL);
	}
}

static void avdtp_abort_cmd(struct bt_avdtp *session, struct net_buf *buf, uint8_t tid)
{
	int err = 0;
	struct bt_avdtp_sep *sep;
	struct net_buf *rsp_buf;
	uint8_t avdtp_err_code = 0;

	sep = avdtp_get_cmd_sep(buf, &avdtp_err_code, NULL);
	avdtp_sep_lock(sep);

	if ((sep == NULL) || (session->ops->abort_ind == NULL)) {
		err = -ENOTSUP;
	} else {
		/* all current sep state is OK for abort operation */
		err = session->ops->abort_ind(session, sep, &avdtp_err_code);
	}

	rsp_buf = avdtp_create_pdu(err ? BT_AVDTP_REJECT : BT_AVDTP_ACCEPT, BT_AVDTP_ABORT, tid);
	if (!rsp_buf) {
		avdtp_sep_unlock(sep);
		return;
	}

	if (err) {
		if (avdtp_err_code == 0) {
			avdtp_err_code = BT_AVDTP_BAD_ACP_SEID;
		}

		LOG_DBG("abort err code:%d", avdtp_err_code);
		net_buf_add_u8(rsp_buf, avdtp_err_code);
	}

	err = avdtp_send_rsp(session, rsp_buf);

	if (!err && !avdtp_err_code) {
		if ((sep->state & (AVDTP_OPEN | AVDTP_STREAMING)) &&
		    (sep->chan.state == BT_L2CAP_CONNECTED)) {
			bt_avdtp_set_state(sep, AVDTP_ABORTING);
		} else {
			bt_avdtp_set_state(sep, AVDTP_IDLE);
		}
	}

	avdtp_sep_unlock(sep);
}

static void avdtp_abort_rsp(struct bt_avdtp *session, struct net_buf *buf, uint8_t msg_type)
{
	struct bt_avdtp_req *req = session->req;

	if (req == NULL) {
		return;
	}

	k_work_cancel_delayable(&session->timeout_work);

	if (msg_type == BT_AVDTP_ACCEPT) {
		uint8_t pre_state = CTRL_REQ(req)->sep->state;

		bt_avdtp_set_state_lock(CTRL_REQ(req)->sep, AVDTP_ABORTING);

		/* release stream */
		if (pre_state & (AVDTP_OPEN | AVDTP_STREAMING)) {
			avdtp_media_disconnect(CTRL_REQ(req)->sep);
		}

		/* For abort, make sure the state revert to IDLE state after
		 * releasing l2cap channel.
		 */
		bt_avdtp_set_state_lock(CTRL_REQ(req)->sep, AVDTP_IDLE);
	} else if (msg_type == BT_AVDTP_REJECT) {
		avdtp_handle_reject(buf, req);
	}

	if (req->status == BT_AVDTP_SUCCESS) {
		avdtp_set_status(req, buf, msg_type);
	}

	bt_avdtp_clear_req(session);

	if (req->func != NULL) {
		req->func(req, NULL);
	}
}

/* Timeout handler */
static void avdtp_timeout(struct k_work *work)
{
	struct bt_avdtp_req *req = (AVDTP_KWORK(work))->req;

	/* add process code */
	/* Gracefully Disconnect the Signalling and streaming L2cap chann*/
	if (req) {
		LOG_DBG("Failed Signal_id = %d", req->sig);

		switch (req->sig) {
		case BT_AVDTP_DISCOVER:
		case BT_AVDTP_GET_CAPABILITIES:
		case BT_AVDTP_SET_CONFIGURATION:
		case BT_AVDTP_RECONFIGURE:
		case BT_AVDTP_OPEN:
		case BT_AVDTP_CLOSE:
		case BT_AVDTP_START:
		case BT_AVDTP_SUSPEND:
			req->status = BT_AVDTP_TIME_OUT;
			req->func(req, NULL);
			break;
		default:
			break;
		}

		AVDTP_KWORK(work)->req = NULL;
	}
}

static int avdtp_send_cmd(struct bt_avdtp *session, struct net_buf *buf, struct bt_avdtp_req *req)
{
	struct bt_avdtp_single_sig_hdr *hdr;

	/* From all the calls, the session, buf and req can't be NULL. */
	__ASSERT_NO_MSG((session != NULL && buf != NULL && req != NULL));

	avdtp_lock(session);

	if (session->req != NULL) {
		avdtp_unlock(session);
		net_buf_unref(buf);
		return -EBUSY;
	}

	session->req = req;
	avdtp_unlock(session);

	hdr = (struct bt_avdtp_single_sig_hdr *)buf->data;
	/* Save the sent request information */
	req->sig = AVDTP_SIGID_GET(hdr->signal_id);
	req->tid = AVDTP_TID_GET(hdr->hdr);
	LOG_DBG("Command sent: buf=%p, sig=0x%02X, tid=0x%02X", buf, req->sig, req->tid);

	avdtp_send_common(session, buf);

	/* Initialize and start timeout timer */
	k_work_init_delayable(&session->timeout_work, avdtp_timeout);
	/* Start timeout work */
	k_work_reschedule(&session->timeout_work, AVDTP_TIMEOUT);

	return 0;
}

/* L2CAP Interface callbacks */
void bt_avdtp_l2cap_connected(struct bt_l2cap_chan *chan)
{
	struct bt_avdtp *session;

	if (!chan) {
		LOG_ERR("Invalid AVDTP chan");
		return;
	}

	session = AVDTP_CHAN(chan);
	LOG_DBG("chan %p session %p", chan, session);

	/* notify a2dp connection result */
	session->ops->connected(session);
}

void bt_avdtp_clear_tx(struct bt_avdtp *session)
{
	struct net_buf *buf, *next;
	struct avdtp_buf_user_data *user_data;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&avdtp_tx_list, buf, next, node) {
		user_data = net_buf_user_data(buf);

		if (user_data->session == session) {
			sys_slist_find_and_remove(&avdtp_tx_list, &buf->node);
			net_buf_unref(buf);
		}
	}
}

void bt_avdtp_l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_avdtp *session = AVDTP_CHAN(chan);

	LOG_DBG("chan %p session %p", chan, session);
	session->br_chan.chan.conn = NULL;
	/* Clear the Pending req if set*/
	if (session->req) {
		struct bt_avdtp_req *req = session->req;

		req->status = BT_AVDTP_BAD_STATE;
		bt_avdtp_clear_req(session);

		if (req->func != NULL) {
			req->func(req, NULL);
		}
	}

	if (session->reasm_buf != NULL) {
		net_buf_unref(session->reasm_buf);
		session->reasm_buf = NULL;
	}

	k_sem_take(&avdtp_sem_lock, K_FOREVER);
	bt_avdtp_clear_tx(session);
	k_sem_give(&avdtp_sem_lock);

	/* notify a2dp disconnect */
	session->ops->disconnected(session);
}

void (*cmd_handler[])(struct bt_avdtp *session, struct net_buf *buf, uint8_t tid) = {
	avdtp_discover_cmd,              /* BT_AVDTP_DISCOVER */
	avdtp_get_capabilities_cmd,      /* BT_AVDTP_GET_CAPABILITIES */
	avdtp_set_configuration_cmd,     /* BT_AVDTP_SET_CONFIGURATION */
	NULL,                            /* BT_AVDTP_GET_CONFIGURATION */
	avdtp_re_configure_cmd,          /* BT_AVDTP_RECONFIGURE */
	avdtp_open_cmd,                  /* BT_AVDTP_OPEN */
	avdtp_start_cmd,                 /* BT_AVDTP_START */
	avdtp_close_cmd,                 /* BT_AVDTP_CLOSE */
	avdtp_suspend_cmd,               /* BT_AVDTP_SUSPEND */
	avdtp_abort_cmd,                 /* BT_AVDTP_ABORT */
	NULL,                            /* BT_AVDTP_SECURITY_CONTROL */
	avdtp_get_all_capabilities_cmd,  /* BT_AVDTP_GET_ALL_CAPABILITIES */
	NULL,                            /* BT_AVDTP_DELAYREPORT */
};

void (*rsp_handler[])(struct bt_avdtp *session, struct net_buf *buf, uint8_t msg_type) = {
	avdtp_discover_rsp,          /* BT_AVDTP_DISCOVER */
	avdtp_get_capabilities_rsp,  /* BT_AVDTP_GET_CAPABILITIES */
	avdtp_set_configuration_rsp, /* BT_AVDTP_SET_CONFIGURATION */
	NULL,                        /* BT_AVDTP_GET_CONFIGURATION */
	avdtp_re_configure_rsp,      /* BT_AVDTP_RECONFIGURE */
	avdtp_open_rsp,              /* BT_AVDTP_OPEN */
	avdtp_start_rsp,             /* BT_AVDTP_START */
	avdtp_close_rsp,             /* BT_AVDTP_CLOSE */
	avdtp_suspend_rsp,           /* BT_AVDTP_SUSPEND */
	avdtp_abort_rsp,             /* BT_AVDTP_ABORT */
	NULL,                        /* BT_AVDTP_SECURITY_CONTROL */
	avdtp_get_capabilities_rsp,  /* BT_AVDTP_GET_ALL_CAPABILITIES */
	NULL,                        /* BT_AVDTP_DELAYREPORT */
};

static int avdtp_rel_and_return(struct bt_avdtp *session)
{
	if (session->reasm_buf != NULL) {
		net_buf_unref(session->reasm_buf);
		session->reasm_buf = NULL;
	}

	return 0;
}

static bool avdtp_rsp_expected(struct bt_avdtp *session, uint8_t tid, uint8_t sigid)
{
	/* validate if the response is expected*/
	if (session->req == NULL) {
		LOG_DBG("Drop unexpected peer response");
		return false;
	}

	if (session->req->sig != sigid || session->req->tid != tid) {
		LOG_DBG("Peer mismatch resp, expected sig[0x%02x] tid[0x%02x], dropped",
			session->req->sig, session->req->tid);
		return false;
	}

	return true;
}

static int bt_avdtp_l2cap_frags_recv(struct bt_avdtp *session, struct net_buf *buf,
				     uint8_t pack_type, bool *finish)
{
	struct net_buf *sdu_buf;
	struct bt_avdtp_single_sig_hdr *single_hdr;
	struct bt_avdtp_continue_end_sig_hdr *cont_hdr;

	*finish = false;

	if (pack_type == BT_AVDTP_PACKET_TYPE_START) {
		struct bt_avdtp_start_sig_hdr *start_hdr;
		uint32_t total_len;
		uint32_t start_len;

		if (buf->len < sizeof(*start_hdr)) {
			LOG_ERR("Recvd Wrong AVDTP Header");
			return 0;
		}

		start_len = buf->len;
		start_hdr = net_buf_pull_mem(buf, sizeof(*start_hdr));

		if (session->reasm_buf != NULL) {
			LOG_ERR("get start packet during reassembly");
			net_buf_unref(session->reasm_buf);
			session->reasm_buf = NULL;
		}

		if ((AVDTP_MSG_GET(start_hdr->hdr) != BT_AVDTP_CMD) &&
		    (!avdtp_rsp_expected(session, AVDTP_TID_GET(start_hdr->hdr),
					 AVDTP_SIGID_GET(start_hdr->signal_id)))) {
			return avdtp_rel_and_return(session);
		}

		session->num_of_signal_pkts = start_hdr->num_of_signal_pkts;

		if (session->num_of_signal_pkts < 2) {
			LOG_ERR("Unexpected number of signal packets: %d",
				session->num_of_signal_pkts);
			return avdtp_rel_and_return(session);
		}

		session->reasm_buf = net_buf_alloc(&avdtp_pool, K_FOREVER);
		if (session->reasm_buf == NULL) {
			LOG_ERR("fail to alloc reasm buf");
			return 0;
		}

		sdu_buf = session->reasm_buf;

		/* 1. The L2CAP payload of the start and all continue packets of a fragmented
		 * message shall have the same length.
		 * 2. The L2CAP payload of end packets shall not be larger than the start and any
		 * possible continue packet that belong to the same message.
		 */
		/* Since the end packet length is unknown,
		 * calculate the length by assuming end packet has one byte data.
		 */
		total_len = sizeof(*single_hdr) + (start_len - sizeof(*start_hdr)) +
			    (start_len - sizeof(*cont_hdr)) * (session->num_of_signal_pkts - 2) + 1;
		if (total_len > net_buf_tailroom(sdu_buf)) {
			LOG_ERR("Not enough buffer space");
			return avdtp_rel_and_return(session);
		}

		single_hdr = net_buf_add(sdu_buf, sizeof(*single_hdr));
		/* unnecessary to change the packet type as single, not used later. */
		single_hdr->hdr = start_hdr->hdr;
		single_hdr->signal_id = start_hdr->signal_id;

		session->num_of_signal_pkts--;

		/* The total length is less than sdu_buf, so the start packet len is less than
		 * sdu_buf too.
		 */
		net_buf_add_mem(sdu_buf, buf->data, buf->len);

		return 0; /* wait other packets */
	}

	/* continue packet or end packet */
	sdu_buf = session->reasm_buf;
	if (sdu_buf == NULL) {
		LOG_ERR("Discard unexpected continue or end packet");
		return 0;
	}

	if (buf->len < sizeof(*cont_hdr)) {
		LOG_ERR("Recvd Wrong AVDTP Header");
		return 0;
	}
	cont_hdr = net_buf_pull_mem(buf, sizeof(*cont_hdr));

	single_hdr = (struct bt_avdtp_single_sig_hdr *)&sdu_buf->data[0];

	/* compare message type with start packet */
	if (AVDTP_MSG_GET(cont_hdr->hdr) != AVDTP_MSG_GET(single_hdr->hdr) ||
	    AVDTP_TID_GET(cont_hdr->hdr) != AVDTP_TID_GET(single_hdr->hdr)) {
		LOG_ERR("Recvd Wrong msg type or transaction label");
		return avdtp_rel_and_return(session);
	}

	if (session->num_of_signal_pkts == 0) {
		LOG_ERR("num_of_signal_pkts is already 0");
		return avdtp_rel_and_return(session);
	}

	if (buf->len < net_buf_tailroom(sdu_buf)) {
		net_buf_add_mem(sdu_buf, buf->data, buf->len);
	} else {
		LOG_ERR("Not enough buffer space");
		return avdtp_rel_and_return(session);
	}

	session->num_of_signal_pkts--;
	if (pack_type == BT_AVDTP_PACKET_TYPE_END) {
		if (session->num_of_signal_pkts == 0) {
			/* all frags are received */
			*finish = true;
		} else {
			LOG_ERR("Unexpected packet end");
			return avdtp_rel_and_return(session);
		}
	}

	return 0;
}

static int avdtp_rel_and_rej(struct bt_avdtp *session, uint8_t sigid, uint8_t tid)
{
	struct net_buf *rsp_buf;
	int err;

	if (session->reasm_buf != NULL) {
		net_buf_unref(session->reasm_buf);
		session->reasm_buf = NULL;
	}

	rsp_buf = avdtp_create_pdu(BT_AVDTP_GEN_REJECT, sigid, tid);
	if (!rsp_buf) {
		LOG_ERR("Error: No Buff available");
		return 0;
	}

	err = bt_l2cap_chan_send(&session->br_chan.chan, rsp_buf);
	if (err < 0) {
		net_buf_unref(rsp_buf);
		LOG_ERR("Error:L2CAP send fail - result = %d", err);
	}

	return 0;
}

int bt_avdtp_l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_avdtp_single_sig_hdr *single_hdr;
	struct bt_avdtp *session = AVDTP_CHAN(chan);
	uint8_t pack_type, sigid;
	int err = 0;
	uint8_t hdr;

	if (buf->len < sizeof(hdr)) {
		LOG_ERR("Recvd Wrong AVDTP Header");
		return -EINVAL;
	}

	hdr = buf->data[0];
	pack_type = AVDTP_PKT_GET(hdr);

	LOG_DBG("pack_type[%d] msg_type[%d] tid[%d]", pack_type, (uint8_t)AVDTP_MSG_GET(hdr),
		(uint8_t)AVDTP_TID_GET(hdr));

	if (pack_type != BT_AVDTP_PACKET_TYPE_SINGLE) {
		bool finish;

		err = bt_avdtp_l2cap_frags_recv(session, buf, pack_type, &finish);

		if (err != 0 || !finish) {
			return err;
		}

		buf = session->reasm_buf;
	} else {
		if (session->reasm_buf != NULL) {
			LOG_DBG("get single packet during reassembly");

			net_buf_unref(session->reasm_buf);
			session->reasm_buf = NULL;
		}

		if (buf->len < sizeof(*single_hdr)) {
			LOG_ERR("Recvd Wrong AVDTP Header");
			return -EINVAL;
		}
	}

	/* process the buf as single packet no matter it is reassembly or not */
	single_hdr = net_buf_pull_mem(buf, sizeof(*single_hdr));
	sigid = AVDTP_SIGID_GET(single_hdr->signal_id);

	if (AVDTP_MSG_GET(single_hdr->hdr) == BT_AVDTP_CMD) {
		if (sigid != 0U && sigid <= BT_AVDTP_DELAYREPORT &&
		    cmd_handler[sigid - 1U] != NULL) {
			cmd_handler[sigid - 1U](session, buf, AVDTP_TID_GET(single_hdr->hdr));

			return avdtp_rel_and_return(session);
		}

		return avdtp_rel_and_rej(session, sigid, AVDTP_TID_GET(single_hdr->hdr));
	}

	if (avdtp_rsp_expected(session, AVDTP_TID_GET(single_hdr->hdr), sigid) &&
	    sigid != 0U && sigid <= BT_AVDTP_DELAYREPORT &&
	    rsp_handler[sigid - 1U] != NULL) {
		rsp_handler[sigid - 1U](session, buf, AVDTP_MSG_GET(single_hdr->hdr));
	}

	return avdtp_rel_and_return(session);
}

static const struct bt_l2cap_chan_ops signal_chan_ops = {
	.connected = bt_avdtp_l2cap_connected,
	.disconnected = bt_avdtp_l2cap_disconnected,
	.recv = bt_avdtp_l2cap_recv,
};

/*A2DP Layer interface */
int bt_avdtp_connect(struct bt_conn *conn, struct bt_avdtp *session)
{
	if (!session) {
		return -EINVAL;
	}

	/* there are headsets that initiate the AVDTP signal l2cap connection
	 * at the same time when DUT initiates the same l2cap connection.
	 * Use the `conn` to check whether the l2cap creation is already started.
	 * The whole `session` is cleared by upper layer if it is new l2cap connection.
	 */
	k_sem_take(&avdtp_sem_lock, K_FOREVER);

	if (session->br_chan.chan.conn != NULL) {
		k_sem_give(&avdtp_sem_lock);
		return -ENOMEM;
	}

	session->br_chan.chan.conn = conn;
	bt_avdtp_clear_tx(session);
	k_sem_give(&avdtp_sem_lock);

	/* Locking semaphore initialized to 1 (unlocked) */
	k_sem_init(&session->sem_lock, 1, 1);
	session->br_chan.rx.mtu = BT_L2CAP_RX_MTU;
	session->br_chan.chan.ops = &signal_chan_ops;
	session->br_chan.required_sec_level = BT_SECURITY_L2;

	return bt_l2cap_chan_connect(conn, &session->br_chan.chan, BT_L2CAP_PSM_AVDTP);
}

int bt_avdtp_disconnect(struct bt_avdtp *session)
{
	if (!session) {
		return -EINVAL;
	}

	LOG_DBG("session %p", session);

	return bt_l2cap_chan_disconnect(&session->br_chan.chan);
}

int bt_avdtp_l2cap_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
			  struct bt_l2cap_chan **chan)
{
	struct bt_avdtp *session = NULL;
	int result;

	LOG_DBG("conn %p", conn);
	/* Get the AVDTP session from upper layer */
	result = event_cb->accept(conn, &session);
	if (result < 0) {
		return result;
	}

	/* there are headsets that initiate the AVDTP signal l2cap connection
	 * at the same time when DUT initiates the same l2cap connection.
	 * Use the `conn` to check whether the l2cap creation is already started.
	 * The whole `session` is cleared by upper layer if it is new l2cap connection.
	 */
	k_sem_take(&avdtp_sem_lock, K_FOREVER);

	if (session->br_chan.chan.conn == NULL) {
		session->br_chan.chan.conn = conn;
		bt_avdtp_clear_tx(session);
		k_sem_give(&avdtp_sem_lock);
		/* Locking semaphore initialized to 1 (unlocked) */
		k_sem_init(&session->sem_lock, 1, 1);
		session->br_chan.chan.ops = &signal_chan_ops;
		session->br_chan.rx.mtu = BT_L2CAP_RX_MTU;
		*chan = &session->br_chan.chan;
	} else {
		k_sem_give(&avdtp_sem_lock);

		/* get the current opening endpoint */
		if (session->current_sep != NULL) {
			session->current_sep->session = session;
			session->current_sep->chan.chan.ops = &stream_chan_ops;
			session->current_sep->chan.rx.mtu = BT_L2CAP_RX_MTU;
			session->current_sep->chan.required_sec_level = BT_SECURITY_L2;
			*chan = &session->current_sep->chan.chan;
			session->current_sep = NULL;
		} else {
			return -ENOMEM;
		}
	}

	return 0;
}

/* Application will register its callback */
int bt_avdtp_register(struct bt_avdtp_event_cb *cb)
{
	LOG_DBG("");

	if (event_cb) {
		return -EALREADY;
	}

	event_cb = cb;

	return 0;
}

int bt_avdtp_register_sep(uint8_t media_type, uint8_t sep_type, struct bt_avdtp_sep *sep)
{
	LOG_DBG("");

	static uint8_t bt_avdtp_sep = BT_AVDTP_MIN_SEID;

	if (!sep) {
		return -EIO;
	}

	if (bt_avdtp_sep == BT_AVDTP_MAX_SEID) {
		return -EIO;
	}

	k_sem_take(&avdtp_sem_lock, K_FOREVER);
	if (sys_slist_find(&seps, &sep->_node, NULL)) {
		k_sem_give(&avdtp_sem_lock);
		LOG_ERR("Endpoint is already registered");
		return -EEXIST;
	}

	/* the id allocation need be locked to protect it */
	sep->sep_info.id = bt_avdtp_sep++;
	sep->sep_info.inuse = 0U;
	sep->sep_info.media_type = media_type;
	sep->sep_info.tsep = sep_type;
	/* Locking semaphore initialized to 1 (unlocked) */
	k_sem_init(&sep->sem_lock, 1, 1);
	bt_avdtp_set_state_lock(sep, AVDTP_IDLE);

	sys_slist_append(&seps, &sep->_node);
	k_sem_give(&avdtp_sem_lock);

	return 0;
}

/* init function */
int bt_avdtp_init(void)
{
	int err;
	static struct bt_l2cap_server avdtp_l2cap = {
		.psm = BT_L2CAP_PSM_AVDTP,
		.sec_level = BT_SECURITY_L2,
		.accept = bt_avdtp_l2cap_accept,
	};

	LOG_DBG("");

	/* Register AVDTP PSM with L2CAP */
	err = bt_l2cap_br_server_register(&avdtp_l2cap);
	if (err < 0) {
		LOG_ERR("AVDTP L2CAP Registration failed %d", err);
	}

	return err;
}

/* AVDTP Discover Request */
int bt_avdtp_discover(struct bt_avdtp *session, struct bt_avdtp_discover_params *param)
{
	struct net_buf *buf;

	LOG_DBG("");
	if (!param || !session) {
		LOG_DBG("Error: Callback/Session not valid");
		return -EINVAL;
	}

	buf = avdtp_create_pdu(BT_AVDTP_CMD, BT_AVDTP_DISCOVER, avdtp_get_tid(session));
	if (!buf) {
		LOG_ERR("Error: No Buff available");
		return -ENOMEM;
	}

	return avdtp_send_cmd(session, buf, &param->req);
}

int bt_avdtp_parse_sep(struct net_buf *buf, struct bt_avdtp_sep_info *sep_info)
{
	struct bt_avdtp_sep_data *sep_data;

	if ((sep_info != NULL) && (buf != NULL)) {
		if (buf->len >= sizeof(*sep_data)) {
			sep_data = net_buf_pull_mem(buf, sizeof(*sep_data));
			sep_info->inuse = sep_data->inuse;
			sep_info->id = sep_data->id;
			sep_info->tsep = sep_data->tsep;
			sep_info->media_type = sep_data->media_type;
			return 0;
		}
	}

	return -EINVAL;
}

/* AVDTP Get Capabilities Request */
int bt_avdtp_get_capabilities(struct bt_avdtp *session,
			      struct bt_avdtp_get_capabilities_params *param)
{
	struct net_buf *buf;

	LOG_DBG("");
	if (!param || !session) {
		LOG_DBG("Error: Callback/Session not valid");
		return -EINVAL;
	}

	buf = avdtp_create_pdu(BT_AVDTP_CMD, param->get_all_caps ? BT_AVDTP_GET_ALL_CAPABILITIES :
			       BT_AVDTP_GET_CAPABILITIES, avdtp_get_tid(session));
	if (!buf) {
		LOG_ERR("Error: No Buff available");
		return -ENOMEM;
	}

	/* Body of the message */
	net_buf_add_u8(buf, (param->stream_endpoint_id << 2U));

	return avdtp_send_cmd(session, buf, &param->req);
}

int bt_avdtp_parse_capability_codec(struct net_buf *buf, uint8_t *codec_type,
				    uint8_t **codec_info_element, uint16_t *codec_info_element_len)
{
	uint8_t data;
	uint8_t length;

	if (!buf) {
		LOG_DBG("Error: buf not valid");
		return -EINVAL;
	}

	while (buf->len) {
		data = net_buf_pull_u8(buf);
		switch (data) {
		case BT_AVDTP_SERVICE_MEDIA_TRANSPORT:
		case BT_AVDTP_SERVICE_REPORTING:
		case BT_AVDTP_SERVICE_MEDIA_RECOVERY:
		case BT_AVDTP_SERVICE_CONTENT_PROTECTION:
		case BT_AVDTP_SERVICE_HEADER_COMPRESSION:
		case BT_AVDTP_SERVICE_MULTIPLEXING:
		case BT_AVDTP_SERVICE_DELAY_REPORTING:
			if (buf->len < 1U) {
				return -EINVAL;
			}

			length = net_buf_pull_u8(buf);
			if (length > 0) {
				if (buf->len < length) {
					return -EINVAL;
				}

				net_buf_pull_mem(buf, length);
			}
			break;

		case BT_AVDTP_SERVICE_MEDIA_CODEC:
			if (buf->len < 1U) {
				return -EINVAL;
			}

			length = net_buf_pull_u8(buf);
			if (buf->len < length) {
				return -EINVAL;
			}

			if (length > 3) {
				data = net_buf_pull_u8(buf);
				if (data == BT_AVDTP_AUDIO) {
					data = net_buf_pull_u8(buf);
					*codec_type = data;
					*codec_info_element_len = (length - 2);
					*codec_info_element =
						net_buf_pull_mem(buf, (*codec_info_element_len));
					return 0;
				}
			}
			break;

		default:
			break;
		}
	}
	return -EINVAL;
}

static int avdtp_process_configure_command(struct bt_avdtp *session, uint8_t cmd,
					   struct bt_avdtp_set_configuration_params *param)
{
	struct net_buf *buf;

	LOG_DBG("");
	if (!param || !session) {
		LOG_DBG("Error: Callback/Session not valid");
		return -EINVAL;
	}

	buf = avdtp_create_pdu(BT_AVDTP_CMD, cmd, avdtp_get_tid(session));
	if (!buf) {
		LOG_ERR("Error: No Buff available");
		return -ENOMEM;
	}

	/* Body of the message */
	/* ACP Stream Endpoint ID */
	net_buf_add_u8(buf, (param->acp_stream_ep_id << 2U));
	if (cmd == BT_AVDTP_SET_CONFIGURATION) {
		/* INT Stream Endpoint ID */
		net_buf_add_u8(buf, (param->int_stream_endpoint_id << 2U));
		/* Service Category: Media Transport */
		net_buf_add_u8(buf, BT_AVDTP_SERVICE_MEDIA_TRANSPORT);
		/* LOSC */
		net_buf_add_u8(buf, 0);
	}
	/* Service Category: Media Codec */
	net_buf_add_u8(buf, BT_AVDTP_SERVICE_MEDIA_CODEC);
	/* LOSC */
	net_buf_add_u8(buf, param->codec_specific_ie_len + 2);
	/* Media Type */
	net_buf_add_u8(buf, param->media_type << 4U);
	/* Media Codec Type */
	net_buf_add_u8(buf, param->media_codec_type);
	/* Codec Info Element */
	net_buf_add_mem(buf, param->codec_specific_ie, param->codec_specific_ie_len);

	return avdtp_send_cmd(session, buf, &param->req);
}

int bt_avdtp_set_configuration(struct bt_avdtp *session,
			       struct bt_avdtp_set_configuration_params *param)
{
	if (!param || !session || !param->sep) {
		LOG_DBG("Error: parameters not valid");
		return -EINVAL;
	}

	if (param->sep->state != AVDTP_IDLE) {
		return -EINVAL;
	}

	return avdtp_process_configure_command(session, BT_AVDTP_SET_CONFIGURATION, param);
}

int bt_avdtp_reconfigure(struct bt_avdtp *session, struct bt_avdtp_set_configuration_params *param)
{
	if (!param || !session || !param->sep) {
		LOG_DBG("Error: parameters not valid");
		return -EINVAL;
	}

	if (param->sep->state != AVDTP_OPEN) {
		return -EINVAL;
	}

	return avdtp_process_configure_command(session, BT_AVDTP_RECONFIGURE, param);
}

static int bt_avdtp_ctrl(struct bt_avdtp *session, struct bt_avdtp_ctrl_params *param, uint8_t ctrl,
			 uint8_t check_state)
{
	struct net_buf *buf;

	LOG_DBG("");
	if (!param || !session || !param->sep) {
		LOG_DBG("Error: parameters not valid");
		return -EINVAL;
	}

	if (!(param->sep->state & check_state)) {
		return -EINVAL;
	}

	buf = avdtp_create_pdu(BT_AVDTP_CMD, ctrl, avdtp_get_tid(session));
	if (!buf) {
		LOG_ERR("Error: No Buff available");
		return -ENOMEM;
	}

	/* Body of the message */
	/* ACP Stream Endpoint ID */
	net_buf_add_u8(buf, (param->acp_stream_ep_id << 2U));

	return avdtp_send_cmd(session, buf, &param->req);
}

int bt_avdtp_open(struct bt_avdtp *session, struct bt_avdtp_ctrl_params *param)
{
	return bt_avdtp_ctrl(session, param, BT_AVDTP_OPEN, AVDTP_CONFIGURED);
}

int bt_avdtp_close(struct bt_avdtp *session, struct bt_avdtp_ctrl_params *param)
{
	return bt_avdtp_ctrl(session, param, BT_AVDTP_CLOSE, AVDTP_OPEN | AVDTP_STREAMING);
}

int bt_avdtp_start(struct bt_avdtp *session, struct bt_avdtp_ctrl_params *param)
{
	int err;

	err = bt_avdtp_ctrl(session, param, BT_AVDTP_START, AVDTP_OPEN);
	if (!err && param->sep->sep_info.tsep == BT_AVDTP_SINK) {
		bt_avdtp_set_state_lock(param->sep, AVDTP_STREAMING);
	}

	return err;
}

int bt_avdtp_suspend(struct bt_avdtp *session, struct bt_avdtp_ctrl_params *param)
{
	return bt_avdtp_ctrl(session, param, BT_AVDTP_SUSPEND, AVDTP_STREAMING);
}

int bt_avdtp_abort(struct bt_avdtp *session, struct bt_avdtp_ctrl_params *param)
{
	return bt_avdtp_ctrl(session, param, BT_AVDTP_ABORT,
			     AVDTP_CONFIGURED | AVDTP_OPENING | AVDTP_OPEN | AVDTP_STREAMING |
				     AVDTP_CLOSING);
}

int bt_avdtp_send_media_data(struct bt_avdtp_sep *sep, struct net_buf *buf)
{
	int err;

	if (sep->state != AVDTP_STREAMING || sep->sep_info.tsep != BT_AVDTP_SOURCE) {
		return -EIO;
	}

	err = bt_l2cap_chan_send(&sep->chan.chan, buf);
	if (err < 0) {
		LOG_ERR("Error:L2CAP send fail - err = %d", err);
		return err;
	}

	return err;
}

uint32_t bt_avdtp_get_media_mtu(struct bt_avdtp_sep *sep)
{
	return sep->chan.tx.mtu;
}
