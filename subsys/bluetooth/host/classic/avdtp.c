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

#define AVDTP_MSG_POISTION 0x00
#define AVDTP_PKT_POSITION 0x02
#define AVDTP_TID_POSITION 0x04
#define AVDTP_SIGID_MASK   0x3f

#define AVDTP_GET_TR_ID(hdr)    ((hdr & 0xf0) >> AVDTP_TID_POSITION)
#define AVDTP_GET_MSG_TYPE(hdr) (hdr & 0x03)
#define AVDTP_GET_PKT_TYPE(hdr) ((hdr & 0x0c) >> AVDTP_PKT_POSITION)
#define AVDTP_GET_SIG_ID(s)     (s & AVDTP_SIGID_MASK)

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

static struct net_buf *avdtp_create_reply_pdu(uint8_t msg_type, uint8_t pkt_type, uint8_t sig_id,
					      uint8_t tid)
{
	struct net_buf *buf;
	struct bt_avdtp_single_sig_hdr *hdr;

	LOG_DBG("");

	buf = bt_l2cap_create_pdu(NULL, 0);
	if (!buf) {
		LOG_ERR("Error: No Buff available");
		return NULL;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));

	hdr->hdr = (msg_type | pkt_type << AVDTP_PKT_POSITION | tid << AVDTP_TID_POSITION);
	hdr->signal_id = sig_id & AVDTP_SIGID_MASK;

	LOG_DBG("hdr = 0x%02X, Signal_ID = 0x%02X", hdr->hdr, hdr->signal_id);
	return buf;
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

	rsp_buf = avdtp_create_reply_pdu(err ? BT_AVDTP_REJECT : BT_AVDTP_ACCEPT,
					 BT_AVDTP_PACKET_TYPE_SINGLE, BT_AVDTP_DISCOVER, tid);
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

	err = bt_l2cap_chan_send(&session->br_chan.chan, rsp_buf);
	if (err < 0) {
		net_buf_unref(rsp_buf);
		LOG_ERR("Error:L2CAP send fail - result = %d", err);
		return;
	}
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

static struct bt_avdtp_sep *avdtp_get_cmd_sep(struct net_buf *buf, uint8_t *error_code)
{
	struct bt_avdtp_sep *sep;

	if (buf->len < 1U) {
		*error_code = BT_AVDTP_BAD_LENGTH;
		LOG_WRN("Invalid ACP SEID");
		return NULL;
	}

	sep = avdtp_get_sep(net_buf_pull_u8(buf) >> 2);
	return sep;
}

static void avdtp_get_capabilities_cmd(struct bt_avdtp *session, struct net_buf *buf, uint8_t tid)
{
	int err = 0;
	struct net_buf *rsp_buf;
	struct bt_avdtp_sep *sep;
	uint8_t error_code = 0;

	sep = avdtp_get_cmd_sep(buf, &error_code);

	if ((sep == NULL) || (session->ops->get_capabilities_ind == NULL)) {
		err = -ENOTSUP;
	} else {
		rsp_buf = avdtp_create_reply_pdu(BT_AVDTP_ACCEPT, BT_AVDTP_PACKET_TYPE_SINGLE,
						 BT_AVDTP_GET_CAPABILITIES, tid);
		if (!rsp_buf) {
			return;
		}

		err = session->ops->get_capabilities_ind(session, sep, rsp_buf, &error_code);
		if (err) {
			net_buf_unref(rsp_buf);
		}
	}

	if (err) {
		rsp_buf = avdtp_create_reply_pdu(BT_AVDTP_REJECT, BT_AVDTP_PACKET_TYPE_SINGLE,
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

	err = bt_l2cap_chan_send(&session->br_chan.chan, rsp_buf);
	if (err < 0) {
		net_buf_unref(rsp_buf);
		LOG_ERR("Error:L2CAP send fail - result = %d", err);
		return;
	}
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

static void avdtp_process_configuration_cmd(struct bt_avdtp *session, struct net_buf *buf,
					    uint8_t tid, bool reconfig)
{
	int err = 0;
	struct bt_avdtp_sep *sep;
	struct net_buf *rsp_buf;
	uint8_t avdtp_err_code = 0;

	sep = avdtp_get_cmd_sep(buf, &avdtp_err_code);
	avdtp_sep_lock(sep);

	if (sep == NULL) {
		err = -ENOTSUP;
	} else if (!reconfig && session->ops->set_configuration_ind == NULL) {
		err = -ENOTSUP;
	} else if (reconfig && session->ops->re_configuration_ind == NULL) {
		err = -ENOTSUP;
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
			uint8_t int_seid;

			/* INT Stream Endpoint ID */
			int_seid = net_buf_pull_u8(buf) >> 2;

			if (!reconfig) {
				err = session->ops->set_configuration_ind(session, sep, int_seid,
									  buf, &avdtp_err_code);
			} else {
				err = session->ops->re_configuration_ind(session, sep, int_seid,
									 buf, &avdtp_err_code);
			}
		} else {
			LOG_WRN("Invalid INT SEID");
			err = -ENOTSUP;
			avdtp_err_code = BT_AVDTP_BAD_LENGTH;
		}
	}

	rsp_buf = avdtp_create_reply_pdu(
		err ? BT_AVDTP_REJECT : BT_AVDTP_ACCEPT, BT_AVDTP_PACKET_TYPE_SINGLE,
		reconfig ? BT_AVDTP_RECONFIGURE : BT_AVDTP_SET_CONFIGURATION, tid);
	if (!rsp_buf) {
		avdtp_sep_unlock(sep);
		return;
	}

	if (err) {
		if (avdtp_err_code == 0) {
			avdtp_err_code = BT_AVDTP_BAD_ACP_SEID;
		}

		LOG_DBG("set configuration err code:%d", avdtp_err_code);
		/* Service Category: Media Codec */
		net_buf_add_u8(rsp_buf, BT_AVDTP_SERVICE_MEDIA_CODEC);
		/* Length Of Service Capability */
		net_buf_add_u8(rsp_buf, 0);
		/* ERROR CODE */
		net_buf_add_u8(rsp_buf, avdtp_err_code);
	}

	err = bt_l2cap_chan_send(&session->br_chan.chan, rsp_buf);
	if (err) {
		net_buf_unref(rsp_buf);
		LOG_ERR("Error:L2CAP send fail - result = %d", err);
	}

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

	sep = avdtp_get_cmd_sep(buf, &avdtp_err_code);
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

	rsp_buf = avdtp_create_reply_pdu(err ? BT_AVDTP_REJECT : BT_AVDTP_ACCEPT,
					 BT_AVDTP_PACKET_TYPE_SINGLE, BT_AVDTP_OPEN, tid);
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

	err = bt_l2cap_chan_send(&session->br_chan.chan, rsp_buf);
	if (err) {
		net_buf_unref(rsp_buf);
		LOG_ERR("Error:L2CAP send fail - result = %d", err);
	}

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

	sep = avdtp_get_cmd_sep(buf, &avdtp_err_code);
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

	rsp_buf = avdtp_create_reply_pdu(err ? BT_AVDTP_REJECT : BT_AVDTP_ACCEPT,
					 BT_AVDTP_PACKET_TYPE_SINGLE, BT_AVDTP_START, tid);
	if (!rsp_buf) {
		avdtp_sep_unlock(sep);
		return;
	}

	if (err) {
		if (avdtp_err_code == 0) {
			avdtp_err_code = BT_AVDTP_BAD_ACP_SEID;
		}

		LOG_DBG("start err code:%d", avdtp_err_code);
		net_buf_add_u8(rsp_buf, avdtp_err_code);
	}

	err = bt_l2cap_chan_send(&session->br_chan.chan, rsp_buf);
	if (err) {
		net_buf_unref(rsp_buf);
		LOG_ERR("Error:L2CAP send fail - result = %d", err);
	}

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

	sep = avdtp_get_cmd_sep(buf, &avdtp_err_code);
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

	rsp_buf = avdtp_create_reply_pdu(err ? BT_AVDTP_REJECT : BT_AVDTP_ACCEPT,
					 BT_AVDTP_PACKET_TYPE_SINGLE, BT_AVDTP_CLOSE, tid);
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

	err = bt_l2cap_chan_send(&session->br_chan.chan, rsp_buf);
	if (err) {
		net_buf_unref(rsp_buf);
		LOG_ERR("Error:L2CAP send fail - result = %d", err);
	}

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

	sep = avdtp_get_cmd_sep(buf, &avdtp_err_code);
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

	rsp_buf = avdtp_create_reply_pdu(err ? BT_AVDTP_REJECT : BT_AVDTP_ACCEPT,
					 BT_AVDTP_PACKET_TYPE_SINGLE, BT_AVDTP_SUSPEND, tid);
	if (!rsp_buf) {
		avdtp_sep_unlock(sep);
		return;
	}

	if (err) {
		if (avdtp_err_code == 0) {
			avdtp_err_code = BT_AVDTP_BAD_ACP_SEID;
		}

		LOG_DBG("suspend err code:%d", avdtp_err_code);
		net_buf_add_u8(rsp_buf, avdtp_err_code);
	}

	err = bt_l2cap_chan_send(&session->br_chan.chan, rsp_buf);
	if (err) {
		net_buf_unref(rsp_buf);
		LOG_ERR("Error:L2CAP send fail - result = %d", err);
	}

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

	sep = avdtp_get_cmd_sep(buf, &avdtp_err_code);
	avdtp_sep_lock(sep);

	if ((sep == NULL) || (session->ops->abort_ind == NULL)) {
		err = -ENOTSUP;
	} else {
		/* all current sep state is OK for abort operation */
		err = session->ops->abort_ind(session, sep, &avdtp_err_code);
	}

	rsp_buf = avdtp_create_reply_pdu(err ? BT_AVDTP_REJECT : BT_AVDTP_ACCEPT,
					 BT_AVDTP_PACKET_TYPE_SINGLE, BT_AVDTP_ABORT, tid);
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

	err = bt_l2cap_chan_send(&session->br_chan.chan, rsp_buf);
	if (err) {
		net_buf_unref(rsp_buf);
		LOG_ERR("Error:L2CAP send fail - result = %d", err);
	}

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

static int avdtp_send(struct bt_avdtp *session, struct net_buf *buf, struct bt_avdtp_req *req)
{
	int result;
	struct bt_avdtp_single_sig_hdr *hdr;

	avdtp_lock(session);

	if (session->req != NULL) {
		avdtp_unlock(session);
		return -EBUSY;
	}

	session->req = req;
	avdtp_unlock(session);
	hdr = (struct bt_avdtp_single_sig_hdr *)buf->data;

	result = bt_l2cap_chan_send(&session->br_chan.chan, buf);
	if (result < 0) {
		LOG_ERR("Error:L2CAP send fail - result = %d", result);
		bt_avdtp_clear_req(session);
		return result;
	}

	/*Save the sent request*/
	req->sig = AVDTP_GET_SIG_ID(hdr->signal_id);
	req->tid = AVDTP_GET_TR_ID(hdr->hdr);
	LOG_DBG("sig 0x%02X, tid 0x%02X", req->sig, req->tid);

	/* Init the timer */
	k_work_init_delayable(&session->timeout_work, avdtp_timeout);
	/* Start timeout work */
	k_work_reschedule(&session->timeout_work, AVDTP_TIMEOUT);
	return result;
}

static struct net_buf *avdtp_create_pdu(uint8_t msg_type, uint8_t pkt_type, uint8_t sig_id)
{
	struct net_buf *buf;
	static uint8_t tid;
	struct bt_avdtp_single_sig_hdr *hdr;

	LOG_DBG("");

	buf = bt_l2cap_create_pdu(NULL, 0);

	hdr = net_buf_add(buf, sizeof(*hdr));

	hdr->hdr = (msg_type | pkt_type << AVDTP_PKT_POSITION | tid++ << AVDTP_TID_POSITION);
	tid %= 16; /* Loop for 16*/
	hdr->signal_id = sig_id & AVDTP_SIGID_MASK;

	LOG_DBG("hdr = 0x%02X, Signal_ID = 0x%02X", hdr->hdr, hdr->signal_id);
	return buf;
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

	/* notify a2dp disconnect */
	session->ops->disconnected(session);
}

void (*cmd_handler[])(struct bt_avdtp *session, struct net_buf *buf, uint8_t tid) = {
	avdtp_discover_cmd,          /* BT_AVDTP_DISCOVER */
	avdtp_get_capabilities_cmd,  /* BT_AVDTP_GET_CAPABILITIES */
	avdtp_set_configuration_cmd, /* BT_AVDTP_SET_CONFIGURATION */
	NULL,                        /* BT_AVDTP_GET_CONFIGURATION */
	avdtp_re_configure_cmd,      /* BT_AVDTP_RECONFIGURE */
	avdtp_open_cmd,              /* BT_AVDTP_OPEN */
	avdtp_start_cmd,             /* BT_AVDTP_START */
	avdtp_close_cmd,             /* BT_AVDTP_CLOSE */
	avdtp_suspend_cmd,           /* BT_AVDTP_SUSPEND */
	avdtp_abort_cmd,             /* BT_AVDTP_ABORT */
	NULL,                        /* BT_AVDTP_SECURITY_CONTROL */
	NULL,                        /* BT_AVDTP_GET_ALL_CAPABILITIES */
	NULL,                        /* BT_AVDTP_DELAYREPORT */
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
	NULL,                        /* BT_AVDTP_GET_ALL_CAPABILITIES */
	NULL,                        /* BT_AVDTP_DELAYREPORT */
};

int bt_avdtp_l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_avdtp_single_sig_hdr *hdr;
	struct bt_avdtp *session = AVDTP_CHAN(chan);
	uint8_t msgtype, pack_type, sigid, tid;
	struct net_buf *rsp_buf;
	int err;

	if (buf->len < sizeof(*hdr)) {
		LOG_ERR("Recvd Wrong AVDTP Header");
		return -EINVAL;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	pack_type = AVDTP_GET_PKT_TYPE(hdr->hdr);
	msgtype = AVDTP_GET_MSG_TYPE(hdr->hdr);
	sigid = AVDTP_GET_SIG_ID(hdr->signal_id);
	tid = AVDTP_GET_TR_ID(hdr->hdr);

	LOG_DBG("pack_type[0x%02x] msg_type[0x%02x] sig_id[0x%02x] tid[0x%02x]", pack_type, msgtype,
		sigid, tid);

	/* TODO: only support single packet now */
	if (pack_type != BT_AVDTP_PACKET_TYPE_SINGLE) {
		if (pack_type == BT_AVDTP_PACKET_TYPE_START) {
			if (buf->len < 1U) {
				return -EINVAL;
			}

			sigid = net_buf_pull_u8(buf);
			goto send_reject;
		}

		/* discard the continue packet and end packet */
		return -EINVAL;
	}

	if (msgtype == BT_AVDTP_CMD) {
		if (sigid != 0U && sigid <= BT_AVDTP_DELAYREPORT &&
		    cmd_handler[sigid - 1U] != NULL) {
			cmd_handler[sigid - 1U](session, buf, tid);
			return 0;
		}
		/* goto send_reject; */
	} else {
		/* validate if the response is expected*/
		if (session->req == NULL) {
			LOG_DBG("Unexpected peer response");
			return -EINVAL;
		}

		if (session->req->sig != sigid || session->req->tid != tid) {
			LOG_DBG("Peer mismatch resp, expected sig[0x%02x]"
				"tid[0x%02x]",
				session->req->sig, session->req->tid);
			return -EINVAL;
		}

		if (sigid != 0U && sigid <= BT_AVDTP_DELAYREPORT &&
		    cmd_handler[sigid - 1U] != NULL) {
			rsp_handler[sigid - 1U](session, buf, msgtype);
			return 0;
		}

		/* discard unsupported response packet */
		return -EINVAL;
	}

send_reject:
	rsp_buf = avdtp_create_reply_pdu(BT_AVDTP_GEN_REJECT, BT_AVDTP_PACKET_TYPE_SINGLE, sigid,
					 tid);
	if (!rsp_buf) {
		LOG_ERR("Error: No Buff available");
		return -EINVAL;
	}

	err = bt_l2cap_chan_send(&session->br_chan.chan, rsp_buf);
	if (err < 0) {
		net_buf_unref(rsp_buf);
		LOG_ERR("Error:L2CAP send fail - result = %d", err);
	}

	return err;
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
	int err;

	LOG_DBG("");
	if (!param || !session) {
		LOG_DBG("Error: Callback/Session not valid");
		return -EINVAL;
	}

	buf = avdtp_create_pdu(BT_AVDTP_CMD, BT_AVDTP_PACKET_TYPE_SINGLE, BT_AVDTP_DISCOVER);
	if (!buf) {
		LOG_ERR("Error: No Buff available");
		return -ENOMEM;
	}

	err = avdtp_send(session, buf, &param->req);
	if (err) {
		net_buf_unref(buf);
	}

	return err;
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
	int err;

	LOG_DBG("");
	if (!param || !session) {
		LOG_DBG("Error: Callback/Session not valid");
		return -EINVAL;
	}

	buf = avdtp_create_pdu(BT_AVDTP_CMD, BT_AVDTP_PACKET_TYPE_SINGLE,
			       BT_AVDTP_GET_CAPABILITIES);
	if (!buf) {
		LOG_ERR("Error: No Buff available");
		return -ENOMEM;
	}

	/* Body of the message */
	net_buf_add_u8(buf, (param->stream_endpoint_id << 2U));

	err = avdtp_send(session, buf, &param->req);
	if (err) {
		net_buf_unref(buf);
	}

	return err;
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
	int err;

	LOG_DBG("");
	if (!param || !session) {
		LOG_DBG("Error: Callback/Session not valid");
		return -EINVAL;
	}

	buf = avdtp_create_pdu(BT_AVDTP_CMD, BT_AVDTP_PACKET_TYPE_SINGLE, cmd);
	if (!buf) {
		LOG_ERR("Error: No Buff available");
		return -ENOMEM;
	}

	/* Body of the message */
	/* ACP Stream Endpoint ID */
	net_buf_add_u8(buf, (param->acp_stream_ep_id << 2U));
	/* INT Stream Endpoint ID */
	net_buf_add_u8(buf, (param->int_stream_endpoint_id << 2U));
	/* Service Category: Media Transport */
	net_buf_add_u8(buf, BT_AVDTP_SERVICE_MEDIA_TRANSPORT);
	/* LOSC */
	net_buf_add_u8(buf, 0);
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

	err = avdtp_send(session, buf, &param->req);
	if (err) {
		net_buf_unref(buf);
	}

	return err;
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
	int err;

	LOG_DBG("");
	if (!param || !session || !param->sep) {
		LOG_DBG("Error: parameters not valid");
		return -EINVAL;
	}

	if (!(param->sep->state & check_state)) {
		return -EINVAL;
	}

	buf = avdtp_create_pdu(BT_AVDTP_CMD, BT_AVDTP_PACKET_TYPE_SINGLE, ctrl);
	if (!buf) {
		LOG_ERR("Error: No Buff available");
		return -ENOMEM;
	}

	/* Body of the message */
	/* ACP Stream Endpoint ID */
	net_buf_add_u8(buf, (param->acp_stream_ep_id << 2U));

	err = avdtp_send(session, buf, &param->req);
	if (err) {
		net_buf_unref(buf);
	}

	return err;
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
