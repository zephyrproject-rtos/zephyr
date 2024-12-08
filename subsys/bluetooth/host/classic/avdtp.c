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
#define AVDTP_SIGID_MASK 0x3f

#define AVDTP_GET_TR_ID(hdr) ((hdr & 0xf0) >> AVDTP_TID_POSITION)
#define AVDTP_GET_MSG_TYPE(hdr) (hdr & 0x03)
#define AVDTP_GET_PKT_TYPE(hdr) ((hdr & 0x0c) >> AVDTP_PKT_POSITION)
#define AVDTP_GET_SIG_ID(s) (s & AVDTP_SIGID_MASK)

static struct bt_avdtp_event_cb *event_cb;
static sys_slist_t seps;

#define AVDTP_CHAN(_ch) CONTAINER_OF(_ch, struct bt_avdtp, br_chan.chan)

#define AVDTP_KWORK(_work) CONTAINER_OF(CONTAINER_OF(_work, struct k_work_delayable, work),\
					struct bt_avdtp, timeout_work)

#define DISCOVER_REQ(_req) CONTAINER_OF(_req, struct bt_avdtp_discover_params, req)
#define GET_CAP_REQ(_req) CONTAINER_OF(_req, struct bt_avdtp_get_capabilities_params, req)
#define SET_CONF_REQ(_req) CONTAINER_OF(_req, struct bt_avdtp_set_configuration_params, req)
#define OPEN_REQ(_req) CONTAINER_OF(_req, struct bt_avdtp_open_params, req)
#define START_REQ(_req) CONTAINER_OF(_req, struct bt_avdtp_start_params, req)

#define AVDTP_TIMEOUT K_SECONDS(6)

K_MUTEX_DEFINE(avdtp_mutex);
#define AVDTP_LOCK() k_mutex_lock(&avdtp_mutex, K_FOREVER)
#define AVDTP_UNLOCK() k_mutex_unlock(&avdtp_mutex)

enum sep_state {
	AVDTP_IDLE = 0,
	AVDTP_CONFIGURED,
	/* establishing the transport sessions. */
	AVDTP_OPENING,
	AVDTP_OPEN,
	AVDTP_STREAMING,
	AVDTP_CLOSING,
	AVDTP_ABORTING,
};

/* L2CAP Interface callbacks */
void bt_avdtp_media_l2cap_connected(struct bt_l2cap_chan *chan)
{
	struct bt_avdtp *session;
	struct bt_avdtp_sep *sep =
		CONTAINER_OF(chan, struct bt_avdtp_sep, chan.chan);

	if (!chan) {
		LOG_ERR("Invalid AVDTP chan");
		return;
	}

	session = sep->session;
	if (session == NULL) {
		return;
	}

	LOG_DBG("chan %p session %p", chan, session);
	sep->state = AVDTP_OPEN;
	if (session->req != NULL) {
		struct bt_avdtp_req *req = session->req;

		OPEN_REQ(req)->status = 0;
		AVDTP_LOCK();
		session->req = NULL;
		AVDTP_UNLOCK();
		if (req->func != NULL) {
			req->func(req);
		}
	}
}

void bt_avdtp_media_l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_avdtp_sep *sep =
		CONTAINER_OF(chan, struct bt_avdtp_sep, chan.chan);

	LOG_DBG("chan %p", chan);
	chan->conn = NULL;
	if (sep->state > AVDTP_OPENING) {
		sep->state = AVDTP_OPENING;
	}
}

int bt_avdtp_media_l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	/* media data is received */
	struct bt_avdtp_sep *sep =
		CONTAINER_OF(chan, struct bt_avdtp_sep, chan.chan);

	if (sep->media_data_cb != NULL) {
		sep->media_data_cb(sep, buf);
	}
	return 0;
}

static int avdtp_media_connect(struct bt_avdtp *session, struct bt_avdtp_sep *sep)
{
	static const struct bt_l2cap_chan_ops ops = {
		.connected = bt_avdtp_media_l2cap_connected,
		.disconnected = bt_avdtp_media_l2cap_disconnected,
		.recv = bt_avdtp_media_l2cap_recv
	};

	if (!session) {
		return -EINVAL;
	}

	sep->session = session;
	sep->chan.rx.mtu = BT_L2CAP_RX_MTU;
	sep->chan.chan.ops = &ops;
	sep->chan.required_sec_level = BT_SECURITY_L2;

	return bt_l2cap_chan_connect(session->br_chan.chan.conn, &sep->chan.chan,
					 BT_L2CAP_PSM_AVDTP);
}

static struct net_buf *avdtp_create_reply_pdu(uint8_t msg_type,
					uint8_t pkt_type,
					uint8_t sig_id,
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

	hdr->hdr = (msg_type | pkt_type << AVDTP_PKT_POSITION |
			tid << AVDTP_TID_POSITION);
	hdr->signal_id = sig_id & AVDTP_SIGID_MASK;

	LOG_DBG("hdr = 0x%02X, Signal_ID = 0x%02X", hdr->hdr, hdr->signal_id);
	return buf;
}

static void avdtp_discover_handler(struct bt_avdtp *session,
			struct net_buf *buf, uint8_t msg_type,
			uint8_t tid)
{
	if (msg_type == BT_AVDTP_CMD) {
		int err;
		struct bt_avdtp_sep *sep;
		struct net_buf *rsp_buf;
		uint8_t error_code = 0;

		if (session->ops->discovery_ind == NULL) {
			err = -ENOTSUP;
		} else {
			err = session->ops->discovery_ind(session, &error_code);
		}

		rsp_buf = avdtp_create_reply_pdu(err ?
					BT_AVDTP_REJECT : BT_AVDTP_ACCEPT,
					BT_AVDTP_PACKET_TYPE_SINGLE,
					BT_AVDTP_DISCOVER, tid);
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
	} else {
		struct bt_avdtp_req *req = session->req;

		if (session->req == NULL) {
			return;
		}
		k_work_cancel_delayable(&session->timeout_work);
		if (msg_type == BT_AVDTP_ACCEPT) {
			DISCOVER_REQ(session->req)->status = 0;
			DISCOVER_REQ(session->req)->buf = buf;
		} else if (msg_type == BT_AVDTP_REJECT) {
			DISCOVER_REQ(session->req)->status = net_buf_pull_u8(buf);
		} else if (msg_type == BT_AVDTP_GEN_REJECT) {
			DISCOVER_REQ(session->req)->status = BT_AVDTP_NOT_SUPPORTED_COMMAND;
		}
		AVDTP_LOCK();
		session->req = NULL;
		AVDTP_UNLOCK();
		if (req->func != NULL) {
			req->func(req);
		}
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

static void avdtp_get_capabilities_handler(struct bt_avdtp *session,
			struct net_buf *buf, uint8_t msg_type, uint8_t tid)
{
	if (msg_type == BT_AVDTP_CMD) {
		int err = 0;
		struct net_buf *rsp_buf;
		struct bt_avdtp_sep *sep;
		uint8_t error_code = 0;

		sep = avdtp_get_sep(net_buf_pull_u8(buf) >> 2);
		if ((sep == NULL) || (session->ops->get_capabilities_ind == NULL)) {
			err = -ENOTSUP;
		} else {
			rsp_buf = avdtp_create_reply_pdu(BT_AVDTP_ACCEPT,
					BT_AVDTP_PACKET_TYPE_SINGLE,
					BT_AVDTP_GET_CAPABILITIES,
					tid);
			if (!rsp_buf) {
				return;
			}
			err = session->ops->get_capabilities_ind(session,
				sep, rsp_buf, &error_code);
			if (err) {
				net_buf_unref(rsp_buf);
			}
		}

		if (err) {
			rsp_buf = avdtp_create_reply_pdu(BT_AVDTP_REJECT,
					BT_AVDTP_PACKET_TYPE_SINGLE,
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
	} else {
		struct bt_avdtp_req *req = session->req;

		if (session->req == NULL) {
			return;
		}
		k_work_cancel_delayable(&session->timeout_work);
		GET_CAP_REQ(session->req)->buf = NULL;

		if (msg_type == BT_AVDTP_ACCEPT) {
			GET_CAP_REQ(session->req)->status = 0;
			if (session->req != NULL) {
				GET_CAP_REQ(session->req)->buf = buf;
			}
		} else if (msg_type == BT_AVDTP_REJECT) {
			GET_CAP_REQ(session->req)->status = net_buf_pull_u8(buf);
		} else if (msg_type == BT_AVDTP_GEN_REJECT) {
			GET_CAP_REQ(session->req)->status = BT_AVDTP_NOT_SUPPORTED_COMMAND;
		}
		AVDTP_LOCK();
		session->req = NULL;
		AVDTP_UNLOCK();
		if (req->func != NULL) {
			req->func(req);
		}
	}
}

static void avdtp_process_configuration(struct bt_avdtp *session,
				struct net_buf *buf, uint8_t msg_type, uint8_t tid)
{
	if (msg_type == BT_AVDTP_CMD) {
		int err = 0;
		struct bt_avdtp_sep *sep;
		struct net_buf *rsp_buf;
		uint8_t error_code = 0;

		sep = avdtp_get_sep(net_buf_pull_u8(buf) >> 2);
		if ((sep == NULL) || (session->ops->set_configuration_ind == NULL)) {
			err = -ENOTSUP;
		} else {
			if (sep->state == AVDTP_STREAMING) {
				err = -ENOTSUP;
				error_code = BT_AVDTP_BAD_STATE;
			} else {
				uint8_t int_seid;

				/* INT Stream Endpoint ID */
				int_seid = net_buf_pull_u8(buf);
				err = session->ops->set_configuration_ind(session,
						sep, int_seid, buf, &error_code);
			}
		}

		rsp_buf = avdtp_create_reply_pdu(err ?
					BT_AVDTP_REJECT : BT_AVDTP_ACCEPT,
					BT_AVDTP_PACKET_TYPE_SINGLE,
					BT_AVDTP_SET_CONFIGURATION, tid);
		if (!rsp_buf) {
			return;
		}

		if (err) {
			if (error_code == 0) {
				error_code = BT_AVDTP_BAD_ACP_SEID;
			}
			LOG_DBG("set configuration err code:%d", error_code);
			/* Service Category: Media Codec */
			net_buf_add_u8(rsp_buf, BT_AVDTP_SERVICE_MEDIA_CODEC);
			/* Length Of Service Capability */
			net_buf_add_u8(rsp_buf, 0);
			/* ERROR CODE */
			net_buf_add_u8(rsp_buf, error_code);
		} else {
			sep->state = AVDTP_CONFIGURED;
		}

		err = bt_l2cap_chan_send(&session->br_chan.chan, rsp_buf);
		if (err < 0) {
			net_buf_unref(rsp_buf);
			LOG_ERR("Error:L2CAP send fail - result = %d", err);
			return;
		}
	} else {
		struct bt_avdtp_req *req = session->req;

		if (session->req == NULL) {
			return;
		}
		k_work_cancel_delayable(&session->timeout_work);
		if (msg_type == BT_AVDTP_ACCEPT) {
			SET_CONF_REQ(req)->status = 0;
			SET_CONF_REQ(req)->sep->state = AVDTP_CONFIGURED;
		} else if (msg_type == BT_AVDTP_REJECT) {
			/* Service Category */
			net_buf_pull_u8(buf);
			SET_CONF_REQ(req)->status = net_buf_pull_u8(buf);
		} else if (msg_type == BT_AVDTP_GEN_REJECT) {
			SET_CONF_REQ(req)->status = BT_AVDTP_NOT_SUPPORTED_COMMAND;
		}
		AVDTP_LOCK();
		session->req = NULL;
		AVDTP_UNLOCK();
		if (req->func != NULL) {
			req->func(req);
		}
	}
}

static void avdtp_set_configuration_handler(struct bt_avdtp *session,
			struct net_buf *buf, uint8_t msg_type, uint8_t tid)
{
	avdtp_process_configuration(session, buf, msg_type, tid);
}

static void avdtp_get_configuration_handler(struct bt_avdtp *session,
			struct net_buf *buf, uint8_t msg_type, uint8_t tid)
{
	/* todo: is not supported now, reply reject */
	struct net_buf *rsp_buf;
	int err;

	rsp_buf = avdtp_create_reply_pdu(BT_AVDTP_REJECT,
			BT_AVDTP_PACKET_TYPE_SINGLE,
			BT_AVDTP_GET_CONFIGURATION, tid);
	if (!rsp_buf) {
		LOG_ERR("Error: No Buff available");
		return;
	}

	err = bt_l2cap_chan_send(&session->br_chan.chan, rsp_buf);
	if (err < 0) {
		net_buf_unref(rsp_buf);
		LOG_ERR("Error:L2CAP send fail - result = %d", err);
		return;
	}
}

static void avdtp_re_configure_handler(struct bt_avdtp *session,
			struct net_buf *buf, uint8_t msg_type, uint8_t tid)
{
	avdtp_process_configuration(session, buf, msg_type, tid);
}

static void avdtp_open_handler(struct bt_avdtp *session,
			struct net_buf *buf, uint8_t msg_type, uint8_t tid)
{
	if (msg_type == BT_AVDTP_CMD) {
		int err = 0;
		struct bt_avdtp_sep *sep;
		struct net_buf *rsp_buf;
		uint8_t error_code = 0;

		sep = avdtp_get_sep(net_buf_pull_u8(buf) >> 2);
		if ((sep == NULL) || (session->ops->open_ind == NULL)) {
			err = -ENOTSUP;
		} else {
			if (sep->state != AVDTP_CONFIGURED) {
				err = -ENOTSUP;
				error_code = BT_AVDTP_BAD_STATE;
			} else {
				err = session->ops->open_ind(session, sep, &error_code);
			}
		}

		rsp_buf = avdtp_create_reply_pdu(err ?
					BT_AVDTP_REJECT : BT_AVDTP_ACCEPT,
					BT_AVDTP_PACKET_TYPE_SINGLE,
					BT_AVDTP_OPEN, tid);
		if (!rsp_buf) {
			return;
		}

		if (err) {
			if (error_code == 0) {
				error_code = BT_AVDTP_BAD_ACP_SEID;
			}
			LOG_DBG("open_ind err code:%d", error_code);
			net_buf_add_u8(rsp_buf, error_code);
		} else {
			session->current_sep = sep;
			sep->state = AVDTP_OPENING;
			sep->sep_info.inuse = 1u;
		}

		err = bt_l2cap_chan_send(&session->br_chan.chan, rsp_buf);
		if (err < 0) {
			net_buf_unref(rsp_buf);
			LOG_ERR("Error:L2CAP send fail - result = %d", err);
			return;
		}
	} else {
		struct bt_avdtp_req *req = session->req;

		if (session->req == NULL) {
			return;
		}
		k_work_cancel_delayable(&session->timeout_work);
		if (msg_type == BT_AVDTP_ACCEPT) {
			OPEN_REQ(req)->status = 0;
			OPEN_REQ(req)->sep->state = AVDTP_OPENING;
			if (!avdtp_media_connect(session, OPEN_REQ(req)->sep)) {
				return;
			}
		} else if (msg_type == BT_AVDTP_REJECT) {
			OPEN_REQ(req)->status = net_buf_pull_u8(buf);
		} else if (msg_type == BT_AVDTP_GEN_REJECT) {
			OPEN_REQ(req)->status = BT_AVDTP_NOT_SUPPORTED_COMMAND;
		}
		if (OPEN_REQ(req)->status) {
			/* wait the media l2cap is established */
			AVDTP_LOCK();
			session->req = NULL;
			AVDTP_UNLOCK();
			if (req->func != NULL) {
				req->func(req);
			}
		}
	}
}

static void avdtp_start_handler(struct bt_avdtp *session,
			struct net_buf *buf, uint8_t msg_type, uint8_t tid)
{
	if (msg_type == BT_AVDTP_CMD) {
		int err = 0;
		struct bt_avdtp_sep *sep;
		struct net_buf *rsp_buf;
		uint8_t error_code = 0;

		sep = avdtp_get_sep(net_buf_pull_u8(buf) >> 2);
		if ((sep == NULL) || (session->ops->start_ind == NULL)) {
			err = -ENOTSUP;
		} else {
			if (sep->state != AVDTP_OPEN) {
				err = -ENOTSUP;
				error_code = BT_AVDTP_BAD_STATE;
			} else {
				err = session->ops->start_ind(session, sep, &error_code);
			}
		}

		rsp_buf = avdtp_create_reply_pdu(err ?
					BT_AVDTP_REJECT : BT_AVDTP_ACCEPT,
					BT_AVDTP_PACKET_TYPE_SINGLE,
					BT_AVDTP_START, tid);
		if (!rsp_buf) {
			return;
		}

		if (err) {
			if (error_code == 0) {
				error_code = BT_AVDTP_BAD_ACP_SEID;
			}
			LOG_DBG("start err code:%d", error_code);
			net_buf_add_u8(rsp_buf, error_code);
		} else {
			sep->state = AVDTP_STREAMING;
		}

		err = bt_l2cap_chan_send(&session->br_chan.chan, rsp_buf);
		if (err < 0) {
			net_buf_unref(rsp_buf);
			LOG_ERR("Error:L2CAP send fail - result = %d", err);
			return;
		}
	} else {
		struct bt_avdtp_req *req = session->req;

		if (session->req == NULL) {
			return;
		}
		k_work_cancel_delayable(&session->timeout_work);
		if (msg_type == BT_AVDTP_ACCEPT) {
			START_REQ(req)->status = 0;
			START_REQ(req)->sep->state = AVDTP_STREAMING;
		} else if (msg_type == BT_AVDTP_REJECT) {
			uint8_t acp_seid;

			acp_seid = net_buf_pull_u8(buf);
			if (acp_seid != START_REQ(req)->acp_stream_ep_id) {
				return;
			}

			START_REQ(req)->status = net_buf_pull_u8(buf);
		} else if (msg_type == BT_AVDTP_GEN_REJECT) {
			START_REQ(req)->status = BT_AVDTP_NOT_SUPPORTED_COMMAND;
		}
		AVDTP_LOCK();
		session->req = NULL;
		AVDTP_UNLOCK();
		if (req->func != NULL) {
			req->func(req);
		}
	}
}

static void avdtp_close_handler(struct bt_avdtp *session,
			struct net_buf *buf, uint8_t msg_type, uint8_t tid)
{
	if (msg_type == BT_AVDTP_CMD) {
		int err = 0;
		struct bt_avdtp_sep *sep;
		struct net_buf *rsp_buf;
		uint8_t error_code = 0;

		sep = avdtp_get_sep(net_buf_pull_u8(buf) >> 2);
		if ((sep == NULL) || (session->ops->close_ind == NULL)) {
			err = -ENOTSUP;
		} else {
			if (sep->state != AVDTP_OPEN) {
				err = -ENOTSUP;
				error_code = BT_AVDTP_BAD_STATE;
			} else {
				err = session->ops->close_ind(session, sep, &error_code);
			}
		}

		rsp_buf = avdtp_create_reply_pdu(err ?
					BT_AVDTP_REJECT : BT_AVDTP_ACCEPT,
					BT_AVDTP_PACKET_TYPE_SINGLE,
					BT_AVDTP_CLOSE, tid);
		if (!rsp_buf) {
			return;
		}

		if (err) {
			if (error_code == 0) {
				error_code = BT_AVDTP_BAD_ACP_SEID;
			}
			LOG_DBG("close err code:%d", error_code);
			net_buf_add_u8(rsp_buf, error_code);
		} else {
			sep->state = AVDTP_CONFIGURED;
			sep->sep_info.inuse = 0u;
		}

		err = bt_l2cap_chan_send(&session->br_chan.chan, rsp_buf);
		if (err < 0) {
			net_buf_unref(rsp_buf);
			LOG_ERR("Error:L2CAP send fail - result = %d", err);
			return;
		}
	}
}

static void avdtp_suspend_handler(struct bt_avdtp *session,
			struct net_buf *buf, uint8_t msg_type, uint8_t tid)
{
	if (msg_type == BT_AVDTP_CMD) {
		int err = 0;
		struct bt_avdtp_sep *sep;
		struct net_buf *rsp_buf;
		uint8_t error_code = 0;

		sep = avdtp_get_sep(net_buf_pull_u8(buf) >> 2);
		if ((sep == NULL) || (session->ops->suspend_ind == NULL)) {
			err = -ENOTSUP;
		} else {
			if (sep->state != AVDTP_STREAMING) {
				err = -ENOTSUP;
				error_code = BT_AVDTP_BAD_STATE;
			} else {
				err = session->ops->suspend_ind(session, sep, &error_code);
			}
		}

		rsp_buf = avdtp_create_reply_pdu(err ?
					BT_AVDTP_REJECT : BT_AVDTP_ACCEPT,
					BT_AVDTP_PACKET_TYPE_SINGLE,
					BT_AVDTP_SUSPEND, tid);
		if (!rsp_buf) {
			return;
		}

		if (err) {
			if (error_code == 0) {
				error_code = BT_AVDTP_BAD_ACP_SEID;
			}
			LOG_DBG("suspend err code:%d", error_code);
			net_buf_add_u8(rsp_buf, error_code);
		} else {
			sep->state = AVDTP_OPEN;
		}

		err = bt_l2cap_chan_send(&session->br_chan.chan, rsp_buf);
		if (err < 0) {
			net_buf_unref(rsp_buf);
			LOG_ERR("Error:L2CAP send fail - result = %d", err);
			return;
		}
	}
}

static void avdtp_abort_handler(struct bt_avdtp *session,
			struct net_buf *buf, uint8_t msg_type, uint8_t tid)
{
	if (msg_type == BT_AVDTP_CMD) {
		int err = 0;
		struct bt_avdtp_sep *sep;
		struct net_buf *rsp_buf;
		uint8_t error_code = 0;

		sep = avdtp_get_sep(net_buf_pull_u8(buf) >> 2);
		if ((sep == NULL) || (session->ops->abort_ind == NULL)) {
			err = -ENOTSUP;
		} else {
			err = session->ops->abort_ind(session, sep, &error_code);
		}

		rsp_buf = avdtp_create_reply_pdu(err ?
					BT_AVDTP_REJECT : BT_AVDTP_ACCEPT,
					BT_AVDTP_PACKET_TYPE_SINGLE,
					BT_AVDTP_ABORT, tid);
		if (!rsp_buf) {
			return;
		}

		if (err) {
			if (error_code == 0) {
				error_code = BT_AVDTP_BAD_ACP_SEID;
			}
			LOG_DBG("abort err code:%d", error_code);
			net_buf_add_u8(rsp_buf, error_code);
		} else {
			sep->state = AVDTP_IDLE;
		}

		err = bt_l2cap_chan_send(&session->br_chan.chan, rsp_buf);
		if (err < 0) {
			net_buf_unref(rsp_buf);
			LOG_ERR("Error:L2CAP send fail - result = %d", err);
			return;
		}
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
			DISCOVER_REQ(req)->status = BT_AVDTP_TIME_OUT;
			req->func(req);
			break;
		case BT_AVDTP_GET_CAPABILITIES:
			GET_CAP_REQ(req)->status = BT_AVDTP_TIME_OUT;
			req->func(req);
			break;
		case BT_AVDTP_SET_CONFIGURATION:
			SET_CONF_REQ(req)->status = BT_AVDTP_TIME_OUT;
			req->func(req);
			break;
		case BT_AVDTP_OPEN:
			OPEN_REQ(req)->status = BT_AVDTP_TIME_OUT;
			req->func(req);
			break;
		case BT_AVDTP_START:
			START_REQ(req)->status = BT_AVDTP_TIME_OUT;
			req->func(req);
			break;
		default:
			break;
		}

		AVDTP_KWORK(work)->req = NULL;
	}
}

static int avdtp_send(struct bt_avdtp *session,
			  struct net_buf *buf, struct bt_avdtp_req *req)
{
	int result;
	struct bt_avdtp_single_sig_hdr *hdr;

	AVDTP_LOCK();
	if (session->req != NULL) {
		AVDTP_UNLOCK();
		return -EBUSY;
	}
	session->req = req;
	AVDTP_UNLOCK();
	hdr = (struct bt_avdtp_single_sig_hdr *)buf->data;

	result = bt_l2cap_chan_send(&session->br_chan.chan, buf);
	if (result < 0) {
		LOG_ERR("Error:L2CAP send fail - result = %d", result);
		net_buf_unref(buf);
		AVDTP_LOCK();
		session->req = NULL;
		AVDTP_UNLOCK();
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

static struct net_buf *avdtp_create_pdu(uint8_t msg_type,
					uint8_t pkt_type,
					uint8_t sig_id)
{
	struct net_buf *buf;
	static uint8_t tid;
	struct bt_avdtp_single_sig_hdr *hdr;

	LOG_DBG("");

	buf = bt_l2cap_create_pdu(NULL, 0);

	hdr = net_buf_add(buf, sizeof(*hdr));

	hdr->hdr = (msg_type | pkt_type << AVDTP_PKT_POSITION |
			tid++ << AVDTP_TID_POSITION);
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
	session->signalling_l2cap_connected = 0;
	/* todo: Clear the Pending req if set*/

	/* notify a2dp disconnect */
	session->ops->disconnected(session);
}

void bt_avdtp_l2cap_encrypt_changed(struct bt_l2cap_chan *chan, uint8_t status)
{
	LOG_DBG("");
}

static const struct {
	uint8_t sig_id;
	void (*func)(struct bt_avdtp *session, struct net_buf *buf,
			 uint8_t msg_type, uint8_t tid);
} handler[] = {
	{BT_AVDTP_DISCOVER, avdtp_discover_handler},
	{BT_AVDTP_GET_CAPABILITIES, avdtp_get_capabilities_handler},
	{BT_AVDTP_SET_CONFIGURATION, avdtp_set_configuration_handler},
	{BT_AVDTP_GET_CONFIGURATION, avdtp_get_configuration_handler},
	{BT_AVDTP_RECONFIGURE, avdtp_re_configure_handler},
	{BT_AVDTP_OPEN, avdtp_open_handler},
	{BT_AVDTP_START, avdtp_start_handler},
	{BT_AVDTP_CLOSE, avdtp_close_handler},
	{BT_AVDTP_SUSPEND, avdtp_suspend_handler},
	{BT_AVDTP_ABORT, avdtp_abort_handler},
};

int bt_avdtp_l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_avdtp_single_sig_hdr *hdr;
	struct bt_avdtp *session = AVDTP_CHAN(chan);
	uint8_t i, msgtype, pack_type, sigid, tid;

	if (buf->len < sizeof(*hdr)) {
		LOG_ERR("Recvd Wrong AVDTP Header");
		return 0;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	pack_type = AVDTP_GET_PKT_TYPE(hdr->hdr);
	msgtype = AVDTP_GET_MSG_TYPE(hdr->hdr);
	sigid = AVDTP_GET_SIG_ID(hdr->signal_id);
	tid = AVDTP_GET_TR_ID(hdr->hdr);

	LOG_DBG("pack_type[0x%02x] msg_type[0x%02x] sig_id[0x%02x] tid[0x%02x]",
		pack_type, msgtype, sigid, tid);

	/* TODO: only support single packet now */
	if (pack_type != BT_AVDTP_PACKET_TYPE_SINGLE) {
		if (pack_type == BT_AVDTP_PACKET_TYPE_START) {
			struct net_buf *rsp_buf;
			int err;

			if (buf->len < sizeof(sigid)) {
				LOG_ERR("Invalid AVDTP Header");
				return 0;
			}

			sigid = net_buf_pull_u8(buf);
			rsp_buf = avdtp_create_reply_pdu(BT_AVDTP_REJECT,
					BT_AVDTP_PACKET_TYPE_SINGLE,
					sigid, tid);
			if (!rsp_buf) {
				LOG_ERR("Error: No Buff available");
				return 0;
			}
			err = bt_l2cap_chan_send(&session->br_chan.chan, rsp_buf);
			if (err < 0) {
				net_buf_unref(rsp_buf);
				LOG_ERR("Error:L2CAP send fail - result = %d", err);
			}
		}
		return 0;
	}

	/* validate if there is an outstanding resp expected*/
	if (msgtype != BT_AVDTP_CMD) {
		if (session->req == NULL) {
			LOG_DBG("Unexpected peer response");
			return 0;
		}

		if (session->req->sig != sigid ||
			session->req->tid != tid) {
			LOG_DBG("Peer mismatch resp, expected sig[0x%02x]"
				"tid[0x%02x]", session->req->sig,
				session->req->tid);
			return 0;
		}
	}

	if (!session) {
		LOG_DBG("Error: Session not valid");
		return 0;
	}

	for (i = 0U; i < ARRAY_SIZE(handler); i++) {
		if (sigid == handler[i].sig_id) {
			handler[i].func(session, buf, msgtype, tid);
			return 0;
		}
	}

	return 0;
}

/*A2DP Layer interface */
int bt_avdtp_connect(struct bt_conn *conn, struct bt_avdtp *session)
{
	static const struct bt_l2cap_chan_ops ops = {
		.connected = bt_avdtp_l2cap_connected,
		.disconnected = bt_avdtp_l2cap_disconnected,
		.encrypt_change = bt_avdtp_l2cap_encrypt_changed,
		.recv = bt_avdtp_l2cap_recv
	};

	if (!session) {
		return -EINVAL;
	}

	session->signalling_l2cap_connected = 1;
	session->br_chan.rx.mtu	= BT_L2CAP_RX_MTU;
	session->br_chan.chan.ops = &ops;
	session->br_chan.required_sec_level = BT_SECURITY_L2;

	return bt_l2cap_chan_connect(conn, &session->br_chan.chan,
					 BT_L2CAP_PSM_AVDTP);
}

int bt_avdtp_disconnect(struct bt_avdtp *session)
{
	if (!session) {
		return -EINVAL;
	}

	LOG_DBG("session %p", session);

	session->signalling_l2cap_connected = 0;
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

	if (session->signalling_l2cap_connected == 0) {
		static const struct bt_l2cap_chan_ops ops = {
			.connected = bt_avdtp_l2cap_connected,
			.disconnected = bt_avdtp_l2cap_disconnected,
			.recv = bt_avdtp_l2cap_recv,
		};
		session->signalling_l2cap_connected = 1;
		session->br_chan.chan.ops = &ops;
		session->br_chan.rx.mtu = BT_L2CAP_RX_MTU;
		*chan = &session->br_chan.chan;
	} else {
		/* get the current opening endpoint */
		if (session->current_sep != NULL) {
			static const struct bt_l2cap_chan_ops ops = {
				.connected = bt_avdtp_media_l2cap_connected,
				.disconnected = bt_avdtp_media_l2cap_disconnected,
				.recv = bt_avdtp_media_l2cap_recv
			};
			session->current_sep->session = session;
			session->current_sep->chan.chan.ops = &ops;
			session->current_sep->chan.rx.mtu = BT_L2CAP_RX_MTU;
			session->current_sep->chan.required_sec_level =
					BT_SECURITY_L2;
			*chan = &session->current_sep->chan.chan;
			session->current_sep = NULL;
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

int bt_avdtp_register_sep(uint8_t media_type, uint8_t role,
			  struct bt_avdtp_sep *sep)
{
	LOG_DBG("");

	static uint8_t bt_avdtp_sep = BT_AVDTP_MIN_SEID;

	if (!sep) {
		return -EIO;
	}

	if (bt_avdtp_sep == BT_AVDTP_MAX_SEID) {
		return -EIO;
	}

	sep->sep_info.id = bt_avdtp_sep++;
	sep->sep_info.inuse = 0U;
	sep->sep_info.media_type = media_type;
	sep->sep_info.tsep = role;
	sep->state = AVDTP_IDLE;

	sys_slist_append(&seps, &sep->_node);

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
int bt_avdtp_discover(struct bt_avdtp *session,
			  struct bt_avdtp_discover_params *param)
{
	struct net_buf *buf;

	LOG_DBG("");
	if (!param || !session) {
		LOG_DBG("Error: Callback/Session not valid");
		return -EINVAL;
	}

	buf = avdtp_create_pdu(BT_AVDTP_CMD,
				   BT_AVDTP_PACKET_TYPE_SINGLE,
				   BT_AVDTP_DISCOVER);
	if (!buf) {
		LOG_ERR("Error: No Buff available");
		return -ENOMEM;
	}

	return avdtp_send(session, buf, &param->req);
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

	buf = avdtp_create_pdu(BT_AVDTP_CMD,
				   BT_AVDTP_PACKET_TYPE_SINGLE,
				   BT_AVDTP_GET_CAPABILITIES);
	if (!buf) {
		LOG_ERR("Error: No Buff available");
		return -ENOMEM;
	}

	/* Body of the message */
	net_buf_add_u8(buf, (param->stream_endpoint_id << 2u));

	return avdtp_send(session, buf, &param->req);
}

int bt_avdtp_parse_capability_codec(struct net_buf *buf,
	uint8_t *codec_type, uint8_t **codec_info_element,
	uint16_t *codec_info_element_len)
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
			length = net_buf_pull_u8(buf);
			if (length > 0) {
				net_buf_pull_mem(buf, length);
			}
			break;

		case BT_AVDTP_SERVICE_MEDIA_CODEC:
			length = net_buf_pull_u8(buf);
			if (length > 3) {
				data = net_buf_pull_u8(buf);
				if (net_buf_tailroom(buf) < (length - 1)) {
					return -EINVAL;
				}
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
	return -EIO;
}

static int avdtp_process_configure_command(struct bt_avdtp *session,
			uint8_t cmd,
			struct bt_avdtp_set_configuration_params *param)
{
	struct net_buf *buf;

	LOG_DBG("");
	if (!param || !session) {
		LOG_DBG("Error: Callback/Session not valid");
		return -EINVAL;
	}

	buf = avdtp_create_pdu(BT_AVDTP_CMD,
				   BT_AVDTP_PACKET_TYPE_SINGLE,
				   cmd);
	if (!buf) {
		LOG_ERR("Error: No Buff available");
		return -ENOMEM;
	}

	/* Body of the message */
	/* ACP Stream Endpoint ID */
	net_buf_add_u8(buf, (param->acp_stream_ep_id << 2u));
	/* INT Stream Endpoint ID */
	net_buf_add_u8(buf, (param->int_stream_endpoint_id << 2u));
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

	return avdtp_send(session, buf, &param->req);
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

int bt_avdtp_reconfigure(struct bt_avdtp *session,
			  struct bt_avdtp_set_configuration_params *param)
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

int bt_avdtp_open(struct bt_avdtp *session,
			  struct bt_avdtp_open_params *param)
{
	struct net_buf *buf;

	LOG_DBG("");
	if (!param || !session || !param->sep) {
		LOG_DBG("Error: parameters not valid");
		return -EINVAL;
	}

	if (param->sep->state != AVDTP_CONFIGURED) {
		return -EINVAL;
	}

	buf = avdtp_create_pdu(BT_AVDTP_CMD,
				   BT_AVDTP_PACKET_TYPE_SINGLE,
				   BT_AVDTP_OPEN);
	if (!buf) {
		LOG_ERR("Error: No Buff available");
		return -ENOMEM;
	}

	/* Body of the message */
	/* ACP Stream Endpoint ID */
	net_buf_add_u8(buf, (param->acp_stream_ep_id << 2u));

	return avdtp_send(session, buf, &param->req);
}

int bt_avdtp_start(struct bt_avdtp *session,
			  struct bt_avdtp_start_params *param)
{
	struct net_buf *buf;

	LOG_DBG("");
	if (!param || !session || !param->sep) {
		LOG_DBG("Error: parameters not valid");
		return -EINVAL;
	}

	if (param->sep->state != AVDTP_OPEN) {
		return -EINVAL;
	}

	buf = avdtp_create_pdu(BT_AVDTP_CMD,
				   BT_AVDTP_PACKET_TYPE_SINGLE,
				   BT_AVDTP_START);
	if (!buf) {
		LOG_ERR("Error: No Buff available");
		return -ENOMEM;
	}

	/* Body of the message */
	/* ACP Stream Endpoint ID */
	net_buf_add_u8(buf, (param->acp_stream_ep_id << 2u));

	return avdtp_send(session, buf, &param->req);
}

int bt_avdtp_send_media_data(struct bt_avdtp_sep *sep, struct net_buf *buf)
{
	int err;

	if (sep->state != AVDTP_STREAMING) {
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
