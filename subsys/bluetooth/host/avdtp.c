/*
 * Audio Video Distribution Protocol
 *
 * Copyright (c) 2021 NXP
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <sys/atomic.h>
#include <sys/byteorder.h>
#include <sys/util.h>

#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/avdtp.h>
#include <bluetooth/a2dp_codec_sbc.h>
#include <bluetooth/a2dp.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_AVDTP)
#define LOG_MODULE_NAME bt_avdtp
#include "common/log.h"

#include "hci_core.h"
#include "conn_internal.h"
#include "l2cap_internal.h"
#include "avdtp_internal.h"

#define AVDTP_MSG_POISTION 0x00
#define AVDTP_PKT_POSITION 0x02
#define AVDTP_TID_POSITION 0x04
#define AVDTP_SIGID_MASK 0x3f

#define AVDTP_GET_TR_ID(hdr) ((hdr & 0xf0) >> AVDTP_TID_POSITION)
#define AVDTP_GET_MSG_TYPE(hdr) (hdr & 0x03)
#define AVDTP_GET_PKT_TYPE(hdr) ((hdr & 0x0c) >> AVDTP_PKT_POSITION)
#define AVDTP_GET_SIG_ID(s) (s & AVDTP_SIGID_MASK)

static struct bt_avdtp_event_cb *event_cb;

static struct bt_avdtp_seid_lsep *lseps;

#define AVDTP_CHAN(_ch) CONTAINER_OF(_ch, struct bt_avdtp, br_chan.chan)

#define AVDTP_KWORK(_work) CONTAINER_OF(_work, struct bt_avdtp_req,\
					timeout_work)

#define DISCOVER_REQ(_req) CONTAINER_OF(_req, struct bt_avdtp_discover_params, req)
#define GET_CAP_REQ(_req) CONTAINER_OF(_req, struct bt_avdtp_get_capabilities_params, req)
#define SET_CONF_REQ(_req) CONTAINER_OF(_req, struct bt_avdtp_set_configuration_params, req)
#define OPEN_REQ(_req) CONTAINER_OF(_req, struct bt_avdtp_open_params, req)
#define START_REQ(_req) CONTAINER_OF(_req, struct bt_avdtp_start_params, req)

#define AVDTP_TIMEOUT K_SECONDS(6)

K_MUTEX_DEFINE(avdtp_mutex);
#define AVDTP_LOCK() k_mutex_lock(&avdtp_mutex, K_FOREVER)
#define AVDTP_UNLOCK() k_mutex_unlock(&avdtp_mutex)

static void avdtp_discover_handler(struct bt_avdtp *session,
			struct net_buf *buf, uint8_t msg_type,
			uint8_t tid);
static void avdtp_get_capabilities_handler(struct bt_avdtp *session,
			struct net_buf *buf, uint8_t msg_type, uint8_t tid);
static void avdtp_set_configuration_handler(struct bt_avdtp *session,
			struct net_buf *buf, uint8_t msg_type, uint8_t tid);
static void avdtp_get_configuration_handler(struct bt_avdtp *session,
			struct net_buf *buf, uint8_t msg_type, uint8_t tid);
static void avdtp_re_configure_handler(struct bt_avdtp *session,
			struct net_buf *buf, uint8_t msg_type, uint8_t tid);
static void avdtp_open_handler(struct bt_avdtp *session,
			struct net_buf *buf, uint8_t msg_type, uint8_t tid);
static void avdtp_start_handler(struct bt_avdtp *session,
			struct net_buf *buf, uint8_t msg_type, uint8_t tid);
static void avdtp_close_handler(struct bt_avdtp *session,
			struct net_buf *buf, uint8_t msg_type, uint8_t tid);
static void avdtp_suspend_handler(struct bt_avdtp *session,
			struct net_buf *buf, uint8_t msg_type, uint8_t tid);
static void avdtp_abort_handler(struct bt_avdtp *session,
			struct net_buf *buf, uint8_t msg_type, uint8_t tid);
static void avdtp_timeout(struct k_work *work);
static struct net_buf *avdtp_create_pdu(uint8_t msg_type,
					uint8_t pkt_type,
					uint8_t sig_id);

enum seid_state {
	AVDTP_IDLE = 0,
	AVDTP_CONFIGURED,
	/* establishing the transport sessions. */
	AVDTP_OPENING,
	AVDTP_OPEN,
	AVDTP_STREAMING,
	AVDTP_CLOSING,
	AVDTP_ABORTING,
};

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


/* L2CAP Interface callbacks */
void bt_avdtp_media_l2cap_connected(struct bt_l2cap_chan *chan)
{
	struct bt_avdtp *session;
	struct bt_avdtp_seid_lsep *lsep =
		CONTAINER_OF(chan, struct bt_avdtp_seid_lsep, media_br_chan.chan);

	session = lsep->session;
	if (session == NULL) {
		return;
	}

	BT_DBG("chan %p session %p", chan, session);
	lsep->seid_state = AVDTP_OPEN;
	if ((session->req != NULL) && (session->req->func != NULL)) {
		struct bt_avdtp_req *req = session->req;

		if (!chan) {
			BT_ERR("Invalid AVDTP chan");
			OPEN_REQ(session->req)->status = -EIO;
			session->req = NULL;
			req->func(req);
			return;
		}

		OPEN_REQ(session->req)->status = 0;
		session->req = NULL;
		req->func(req);
	}
}

void bt_avdtp_media_l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_avdtp_seid_lsep *lsep =
		CONTAINER_OF(chan, struct bt_avdtp_seid_lsep, media_br_chan.chan);

	BT_DBG("chan %p", chan);
	chan->conn = NULL;
	if (lsep->seid_state > AVDTP_OPENING) {
		lsep->seid_state = AVDTP_OPENING;
	}
}

void bt_avdtp_media_l2cap_encrypt_changed(struct bt_l2cap_chan *chan, uint8_t status)
{
}

int bt_avdtp_media_l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	/* media data is received */
	struct bt_avdtp_seid_lsep *lsep =
		CONTAINER_OF(chan, struct bt_avdtp_seid_lsep, media_br_chan.chan);

	if (lsep->media_data_cb != NULL) {
		lsep->media_data_cb(lsep, buf);
	}
	return 0;
}

static int avdtp_media_connect(struct bt_avdtp *session, struct bt_avdtp_seid_lsep *lsep)
{
	static const struct bt_l2cap_chan_ops ops = {
		.connected = bt_avdtp_media_l2cap_connected,
		.disconnected = bt_avdtp_media_l2cap_disconnected,
		.encrypt_change = bt_avdtp_media_l2cap_encrypt_changed,
		.recv = bt_avdtp_media_l2cap_recv
	};

	if (!session) {
		return -EINVAL;
	}

	lsep->session = session;
	lsep->media_br_chan.rx.mtu	= BT_L2CAP_RX_MTU;
	lsep->media_br_chan.chan.ops = &ops;
	lsep->media_br_chan.chan.required_sec_level = BT_SECURITY_L2;

	return bt_l2cap_chan_connect(session->br_chan.chan.conn, &lsep->media_br_chan.chan,
					 BT_L2CAP_PSM_AVDTP);
}

static struct net_buf *avdtp_create_reply_pdu(uint8_t msg_type,
					uint8_t pkt_type,
					uint8_t sig_id,
					uint8_t tid)
{
	struct net_buf *buf;
	struct bt_avdtp_single_sig_hdr *hdr;

	BT_DBG("");

	buf = bt_l2cap_create_pdu(NULL, 0);

	hdr = net_buf_add(buf, sizeof(*hdr));

	hdr->hdr = (msg_type | pkt_type << AVDTP_PKT_POSITION |
			tid << AVDTP_TID_POSITION);
	hdr->signal_id = sig_id & AVDTP_SIGID_MASK;

	BT_DBG("hdr = 0x%02X, Signal_ID = 0x%02X", hdr->hdr, hdr->signal_id);
	return buf;
}

static void avdtp_discover_handler(struct bt_avdtp *session,
			struct net_buf *buf, uint8_t msg_type,
			uint8_t tid)
{
	if (msg_type == BT_AVDTP_CMD) {
		int err;
		struct bt_avdtp_seid_lsep *lsep;
		struct bt_avdtp_seid_info *dest;
		struct net_buf *rsp_buf;
		uint8_t error_code;

		if (!session) {
			BT_DBG("Error: Session not valid");
			return;
		}

		error_code = 0;
		err = session->ops->discovery_ind(session, &error_code);
		if ((err < 0) || (error_code != 0)) {
			if (error_code == 0) {
				error_code = BAD_HEADER_FORMAT;
			}
		}

		if (error_code != 0) {
			rsp_buf = avdtp_create_reply_pdu(BT_AVDTP_REJECT,
					BT_AVDTP_PACKET_TYPE_SINGLE,
					BT_AVDTP_DISCOVER, tid);
			if (!rsp_buf) {
				BT_ERR("Error: No Buff available");
				return;
			}
			/* ERROR CODE */
			net_buf_add_u8(rsp_buf, error_code);
		} else {
			rsp_buf = avdtp_create_reply_pdu(BT_AVDTP_ACCEPT,
					BT_AVDTP_PACKET_TYPE_SINGLE,
					BT_AVDTP_DISCOVER, tid);
			if (!rsp_buf) {
				BT_ERR("Error: No Buff available");
				return;
			}

			lsep = lseps;
			while (lsep != NULL) {
				dest = (struct bt_avdtp_seid_info *)
					net_buf_add(rsp_buf, sizeof(struct bt_avdtp_seid_info));
				*dest = lsep->sep;
				lsep = lsep->next;
			}
		}

		err = bt_l2cap_chan_send(&session->br_chan.chan, rsp_buf);
		if (err < 0) {
			net_buf_unref(rsp_buf);
			BT_ERR("Error:L2CAP send fail - result = %d", err);
			return;
		}
	} else {
		struct bt_avdtp_req *req = session->req;

		if ((session->req == NULL) || (session->req->func == NULL)) {
			return;
		}
		k_work_cancel_delayable(&session->req->timeout_work);
		if (msg_type == BT_AVDTP_ACCEPT) {
			DISCOVER_REQ(session->req)->status = 0;
			if (session->req != NULL) {
				DISCOVER_REQ(session->req)->buf = buf;
			}
		} else {
			/* BT_AVDTP_GEN_REJECT, BT_AVDTP_REJECT */
			DISCOVER_REQ(session->req)->status = -EIO;
		}
		session->req = NULL;
		req->func(req);
		net_buf_unref(buf);
	}
}

static void avdtp_get_capabilities_handler(struct bt_avdtp *session,
			struct net_buf *buf, uint8_t msg_type, uint8_t tid)
{
	if (msg_type == BT_AVDTP_CMD) {
		int err;
		struct net_buf *rsp_buf = NULL;
		struct bt_avdtp_seid_lsep *lsep;
		uint8_t stream_endpoint_id;
		uint8_t error_code;

		if (!session) {
			BT_DBG("Error: Session not valid");
			return;
		}

		stream_endpoint_id = net_buf_pull_u8(buf);
		stream_endpoint_id = stream_endpoint_id >> 2U;
		lsep = lseps;
		while (lsep != NULL) {
			if (lsep->sep.id == stream_endpoint_id) {
				break;
			}
			lsep = lsep->next;
		}

		error_code = 0;
		if (lsep == NULL) {
			error_code = BAD_ACP_SEID;
		} else {
			rsp_buf = avdtp_create_reply_pdu(BT_AVDTP_ACCEPT,
					BT_AVDTP_PACKET_TYPE_SINGLE,
					BT_AVDTP_GET_CAPABILITIES,
					tid);
			if (!rsp_buf) {
				BT_ERR("Error: No Buff available");
				return;
			}

			err = session->ops->get_capabilities_ind(session,
				lsep, rsp_buf, &error_code);
			if ((err < 0) || (error_code != 0)) {
				net_buf_unref(rsp_buf);
				if (error_code == 0) {
					error_code = BAD_ACP_SEID;
				}
			}
		}

		if (error_code) {
			rsp_buf = avdtp_create_reply_pdu(BT_AVDTP_REJECT,
			BT_AVDTP_PACKET_TYPE_SINGLE,
			BT_AVDTP_GET_CAPABILITIES, tid);
			if (!rsp_buf) {
				BT_ERR("Error: No Buff available");
				return;
			}

			net_buf_add_u8(rsp_buf, error_code);
		}

		err = bt_l2cap_chan_send(&session->br_chan.chan, rsp_buf);
		if (err < 0) {
			net_buf_unref(rsp_buf);
			BT_ERR("Error:L2CAP send fail - result = %d", err);
			return;
		}
	} else {
		struct bt_avdtp_req *req = session->req;

		if ((session->req == NULL) || (session->req->func == NULL)) {
			return;
		}
		k_work_cancel_delayable(&session->req->timeout_work);
		GET_CAP_REQ(session->req)->buf = NULL;

		if (msg_type == BT_AVDTP_ACCEPT) {
			GET_CAP_REQ(session->req)->status = 0;
			if (session->req != NULL) {
				GET_CAP_REQ(session->req)->buf = buf;
			}
		} else {
			/* BT_AVDTP_GEN_REJECT, BT_AVDTP_REJECT */
			GET_CAP_REQ(session->req)->status = -EIO;
		}
		session->req = NULL;
		req->func(req);
		net_buf_unref(buf);
	}
}

static void avdtp_process_configuration(struct bt_avdtp *session,
				struct net_buf *buf, uint8_t msg_type, uint8_t tid)
{
	if (msg_type == BT_AVDTP_CMD) {
		int err = -1;
		uint8_t stream_endpoint_id;
		struct bt_avdtp_seid_lsep *lsep;
		struct net_buf *rsp_buf;
		uint8_t error_code;

		if (!session) {
			BT_DBG("Error: Session not valid");
			return;
		}

		/* ACP Stream Endpoint ID */
		stream_endpoint_id = net_buf_pull_u8(buf) >> 2U;
		lsep = lseps;
		while (lsep != NULL) {
			if (lsep->sep.id == stream_endpoint_id) {
				break;
			}
			lsep = lsep->next;
		}

		error_code = 0;
		if (lsep == NULL) {
			error_code = BAD_ACP_SEID;
		} else {
			if (lsep->seid_state == AVDTP_STREAMING) {
				error_code = BAD_STATE;
			} else {
				/* INT Stream Endpoint ID */
				net_buf_pull_u8(buf);
				err = session->ops->set_configuration_ind(session,
						lsep, buf, &error_code);
				if (err < 0) {
					BT_DBG("set configuration err code:%d", error_code);
				}
				if ((err < 0) && (error_code == 0)) {
					error_code = BAD_ACP_SEID;
				}
			}
		}

		if (error_code != 0) {
			rsp_buf = avdtp_create_reply_pdu(BT_AVDTP_REJECT,
					BT_AVDTP_PACKET_TYPE_SINGLE,
					BT_AVDTP_SET_CONFIGURATION, tid);
			if (!rsp_buf) {
				BT_ERR("Error: No Buff available");
				return;
			}
			/* Service Category: Media Codec */
			net_buf_add_u8(rsp_buf, BT_AVDTP_SERVICE_MEDIA_CODEC);
			/* Length Of Service Capability */
			net_buf_add_u8(rsp_buf, 0);
			/* ERROR CODE */
			net_buf_add_u8(rsp_buf, error_code);
		} else {
			lsep->seid_state = AVDTP_CONFIGURED;
			rsp_buf = avdtp_create_reply_pdu(BT_AVDTP_ACCEPT,
					BT_AVDTP_PACKET_TYPE_SINGLE,
					BT_AVDTP_SET_CONFIGURATION, tid);
			if (!rsp_buf) {
				BT_ERR("Error: No Buff available");
				return;
			}
		}

		err = bt_l2cap_chan_send(&session->br_chan.chan, rsp_buf);
		if (err < 0) {
			net_buf_unref(rsp_buf);
			BT_ERR("Error:L2CAP send fail - result = %d", err);
			return;
		}
	} else {
		struct bt_avdtp_req *avdtp_req = session->req;
		struct bt_avdtp_set_configuration_params *set_config;

		if ((session->req == NULL) || (session->req->func == NULL)) {
			return;
		}
		set_config = CONTAINER_OF(avdtp_req, struct bt_avdtp_set_configuration_params, req);
		k_work_cancel_delayable(&avdtp_req->timeout_work);
		if (msg_type == BT_AVDTP_ACCEPT) {
			SET_CONF_REQ(avdtp_req)->status = 0;
			set_config->lsep->seid_state = AVDTP_CONFIGURED;
		} else {
			/* BT_AVDTP_GEN_REJECT, BT_AVDTP_REJECT */
			SET_CONF_REQ(avdtp_req)->status = -EIO;
		}
		net_buf_unref(buf);
		session->req = NULL;
		avdtp_req->func(avdtp_req);
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
	/* todo */
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
		int err = -1;
		uint8_t stream_endpoint_id;
		struct bt_avdtp_seid_lsep *lsep;
		struct net_buf *rsp_buf;
		uint8_t error_code;

		if (!session) {
			BT_DBG("Error: Session not valid");
			return;
		}

		/* ACP Stream Endpoint ID */
		stream_endpoint_id = net_buf_pull_u8(buf) >> 2;
		lsep = lseps;
		while (lsep != NULL) {
			if (lsep->sep.id == stream_endpoint_id) {
				break;
			}
			lsep = lsep->next;
		}

		error_code = 0;
		if (lsep == NULL) {
			error_code = BAD_ACP_SEID;
		} else {
			if (lsep->seid_state != AVDTP_CONFIGURED) {
				error_code = BAD_STATE;
			} else {
				err = session->ops->open_ind(session, lsep, &error_code);
				if (err < 0) {
					BT_DBG("open_ind err code:%d", error_code);
				}
				if ((err < 0) && (error_code == 0)) {
					error_code = BAD_ACP_SEID;
				}
			}
		}

		if (error_code != 0) {
			rsp_buf = avdtp_create_reply_pdu(BT_AVDTP_REJECT,
					BT_AVDTP_PACKET_TYPE_SINGLE,
					BT_AVDTP_OPEN, tid);
			if (!rsp_buf) {
				BT_ERR("Error: No Buff available");
				return;
			}
			/* ERROR CODE */
			net_buf_add_u8(rsp_buf, error_code);
		} else {
			session->current_lsep = lsep;
			lsep->seid_state = AVDTP_OPENING;
			lsep->sep.inuse = 1u;
			rsp_buf = avdtp_create_reply_pdu(BT_AVDTP_ACCEPT,
					BT_AVDTP_PACKET_TYPE_SINGLE,
					BT_AVDTP_OPEN, tid);
			if (!rsp_buf) {
				BT_ERR("Error: No Buff available");
				return;
			}
		}

		err = bt_l2cap_chan_send(&session->br_chan.chan, rsp_buf);
		if (err < 0) {
			net_buf_unref(rsp_buf);
			BT_ERR("Error:L2CAP send fail - result = %d", err);
			return;
		}
	} else {
		struct bt_avdtp_req *avdtp_req = session->req;
		struct bt_avdtp_open_params *open_param;

		if ((session->req == NULL) || (session->req->func == NULL)) {
			return;
		}
		open_param = CONTAINER_OF(avdtp_req, struct bt_avdtp_open_params, req);

		k_work_cancel_delayable(&avdtp_req->timeout_work);
		if (msg_type == BT_AVDTP_ACCEPT) {
			OPEN_REQ(avdtp_req)->status = 0;
			open_param->lsep->seid_state = AVDTP_OPENING;
			if (!avdtp_media_connect(session, open_param->lsep)) {
				net_buf_unref(buf);
				return;
			}

			OPEN_REQ(avdtp_req)->status = -EIO;
		} else {
			/* BT_AVDTP_GEN_REJECT, BT_AVDTP_REJECT */
			OPEN_REQ(avdtp_req)->status = -EIO;
		}
		net_buf_unref(buf);
		if (!(OPEN_REQ(avdtp_req)->status)) {
			/* wait the media l2cap is established */
			session->req = NULL;
			avdtp_req->func(avdtp_req);
		}
	}
}

static void avdtp_start_handler(struct bt_avdtp *session,
			struct net_buf *buf, uint8_t msg_type, uint8_t tid)
{
	if (msg_type == BT_AVDTP_CMD) {
		int err = -1;
		uint8_t stream_endpoint_id;
		struct bt_avdtp_seid_lsep *lsep;
		struct net_buf *rsp_buf;
		uint8_t error_code;

		if (!session) {
			BT_DBG("Error: Session not valid");
			return;
		}

		/* ACP Stream Endpoint ID */
		stream_endpoint_id = net_buf_pull_u8(buf) >> 2;
		lsep = lseps;
		while (lsep != NULL) {
			if (lsep->sep.id == stream_endpoint_id) {
				break;
			}
			lsep = lsep->next;
		}

		error_code = 0;
		if (lsep == NULL) {
			error_code = BAD_ACP_SEID;
		} else {
			if (lsep->seid_state != AVDTP_OPEN) {
				error_code = BAD_STATE;
			} else {
				err = session->ops->start_ind(session, lsep, &error_code);
				if (err < 0) {
					BT_DBG("start err code:%d", error_code);
				}
				if ((err < 0) && (error_code == 0)) {
					error_code = BAD_ACP_SEID;
				}
			}
		}

		if (error_code != 0) {
			lsep->seid_state = AVDTP_STREAMING;
			rsp_buf = avdtp_create_reply_pdu(BT_AVDTP_REJECT,
					BT_AVDTP_PACKET_TYPE_SINGLE,
					BT_AVDTP_START, tid);
			if (!rsp_buf) {
				BT_ERR("Error: No Buff available");
				return;
			}
			/* ERROR CODE */
			net_buf_add_u8(rsp_buf, error_code);
		} else {
			rsp_buf = avdtp_create_reply_pdu(BT_AVDTP_ACCEPT,
					BT_AVDTP_PACKET_TYPE_SINGLE,
					BT_AVDTP_START, tid);
			if (!rsp_buf) {
				BT_ERR("Error: No Buff available");
				return;
			}
		}

		err = bt_l2cap_chan_send(&session->br_chan.chan, rsp_buf);
		if (err < 0) {
			net_buf_unref(rsp_buf);
			BT_ERR("Error:L2CAP send fail - result = %d", err);
			return;
		}
	} else {
		struct bt_avdtp_req *req = session->req;

		if ((session->req == NULL) || (session->req->func == NULL)) {
			return;
		}
		k_work_cancel_delayable(&session->req->timeout_work);
		if (msg_type == BT_AVDTP_ACCEPT) {
			START_REQ(session->req)->status = 0;
		} else {
			/* BT_AVDTP_GEN_REJECT, BT_AVDTP_REJECT */
			START_REQ(session->req)->status = -EIO;
		}
		net_buf_unref(buf);
		session->req = NULL;
		req->func(req);
	}
}

static void avdtp_close_handler(struct bt_avdtp *session,
			struct net_buf *buf, uint8_t msg_type, uint8_t tid)
{
	if (msg_type == BT_AVDTP_CMD) {
		int err = -1;
		uint8_t stream_endpoint_id;
		struct bt_avdtp_seid_lsep *lsep;
		struct net_buf *rsp_buf;
		uint8_t error_code;

		if (!session) {
			BT_DBG("Error: Session not valid");
			return;
		}

		/* ACP Stream Endpoint ID */
		stream_endpoint_id = net_buf_pull_u8(buf) >> 2;
		lsep = lseps;
		while (lsep != NULL) {
			if (lsep->sep.id == stream_endpoint_id) {
				break;
			}
			lsep = lsep->next;
		}

		error_code = 0;
		if (lsep == NULL) {
			error_code = BAD_ACP_SEID;
		} else {
			if (lsep->seid_state != AVDTP_OPEN) {
				error_code = BAD_STATE;
			} else {
				err = session->ops->close_ind(session, lsep, &error_code);
				if (err < 0) {
					BT_DBG("close err code:%d", error_code);
				}
				if ((err < 0) && (error_code == 0)) {
					error_code = BAD_ACP_SEID;
				}
			}
		}

		if (error_code != 0) {
			rsp_buf = avdtp_create_reply_pdu(BT_AVDTP_REJECT,
					BT_AVDTP_PACKET_TYPE_SINGLE,
					BT_AVDTP_OPEN, tid);
			if (!rsp_buf) {
				BT_ERR("Error: No Buff available");
				return;
			}
			/* ERROR CODE */
			net_buf_add_u8(rsp_buf, error_code);
		} else {
			lsep->seid_state = AVDTP_CONFIGURED;
			lsep->sep.inuse = 0u;
			rsp_buf = avdtp_create_reply_pdu(BT_AVDTP_ACCEPT,
					BT_AVDTP_PACKET_TYPE_SINGLE,
					BT_AVDTP_OPEN, tid);
			if (!rsp_buf) {
				BT_ERR("Error: No Buff available");
				return;
			}
		}

		err = bt_l2cap_chan_send(&session->br_chan.chan, rsp_buf);
		if (err < 0) {
			net_buf_unref(rsp_buf);
			BT_ERR("Error:L2CAP send fail - result = %d", err);
			return;
		}
	}
}

static void avdtp_suspend_handler(struct bt_avdtp *session,
			struct net_buf *buf, uint8_t msg_type, uint8_t tid)
{
	if (msg_type == BT_AVDTP_CMD) {
		int err = -1;
		uint8_t stream_endpoint_id;
		struct bt_avdtp_seid_lsep *lsep;
		struct net_buf *rsp_buf;
		uint8_t error_code;

		if (!session) {
			BT_DBG("Error: Session not valid");
			return;
		}

		/* ACP Stream Endpoint ID */
		stream_endpoint_id = net_buf_pull_u8(buf) >> 2;
		lsep = lseps;
		while (lsep != NULL) {
			if (lsep->sep.id == stream_endpoint_id) {
				break;
			}
			lsep = lsep->next;
		}

		error_code = 0;
		if (lsep == NULL) {
			error_code = BAD_ACP_SEID;
		} else {
			if (lsep->seid_state != AVDTP_STREAMING) {
				error_code = BAD_STATE;
			} else {
				err = session->ops->suspend_ind(session, lsep, &error_code);
				if (err < 0) {
					BT_DBG("suspend err code:%d", error_code);
				}
				if ((err < 0) && (error_code == 0)) {
					error_code = BAD_ACP_SEID;
				}
			}
		}

		if (error_code != 0) {
			rsp_buf = avdtp_create_reply_pdu(BT_AVDTP_REJECT,
					BT_AVDTP_PACKET_TYPE_SINGLE,
					BT_AVDTP_OPEN, tid);
			if (!rsp_buf) {
				BT_ERR("Error: No Buff available");
				return;
			}
			/* ERROR CODE */
			net_buf_add_u8(rsp_buf, error_code);
		} else {
			lsep->seid_state = AVDTP_OPEN;
			rsp_buf = avdtp_create_reply_pdu(BT_AVDTP_ACCEPT,
					BT_AVDTP_PACKET_TYPE_SINGLE,
					BT_AVDTP_OPEN, tid);
			if (!rsp_buf) {
				BT_ERR("Error: No Buff available");
				return;
			}
		}

		err = bt_l2cap_chan_send(&session->br_chan.chan, rsp_buf);
		if (err < 0) {
			net_buf_unref(rsp_buf);
			BT_ERR("Error:L2CAP send fail - result = %d", err);
			return;
		}
	}
}

static void avdtp_abort_handler(struct bt_avdtp *session,
			struct net_buf *buf, uint8_t msg_type, uint8_t tid)
{
	if (msg_type == BT_AVDTP_CMD) {
		int err = -1;
		uint8_t stream_endpoint_id;
		struct bt_avdtp_seid_lsep *lsep;
		struct net_buf *rsp_buf;
		uint8_t error_code;

		if (!session) {
			BT_DBG("Error: Session not valid");
			return;
		}

		/* ACP Stream Endpoint ID */
		stream_endpoint_id = net_buf_pull_u8(buf) >> 2;
		lsep = lseps;
		while (lsep != NULL) {
			if (lsep->sep.id == stream_endpoint_id) {
				break;
			}
			lsep = lsep->next;
		}

		error_code = 0;
		if (lsep == NULL) {
			error_code = BAD_ACP_SEID;
		} else {
			err = session->ops->abort_ind(session, lsep, &error_code);
			if (err < 0) {
				BT_DBG("abort err code:%d", error_code);
			}
			if ((err < 0) || (error_code != 0)) {
				if (error_code == 0) {
					error_code = BAD_ACP_SEID;
				}
			}
		}

		if (error_code != 0) {
			rsp_buf = avdtp_create_reply_pdu(BT_AVDTP_REJECT,
					BT_AVDTP_PACKET_TYPE_SINGLE,
					BT_AVDTP_OPEN, tid);
			if (!rsp_buf) {
				BT_ERR("Error: No Buff available");
				return;
			}
			/* ERROR CODE */
			net_buf_add_u8(rsp_buf, error_code);
		} else {
			lsep->seid_state = AVDTP_IDLE;
			rsp_buf = avdtp_create_reply_pdu(BT_AVDTP_ACCEPT,
					BT_AVDTP_PACKET_TYPE_SINGLE,
					BT_AVDTP_OPEN, tid);
			if (!rsp_buf) {
				BT_ERR("Error: No Buff available");
				return;
			}
		}

		err = bt_l2cap_chan_send(&session->br_chan.chan, rsp_buf);
		if (err < 0) {
			net_buf_unref(rsp_buf);
			BT_ERR("Error:L2CAP send fail - result = %d", err);
			return;
		}
	}
}

static int avdtp_send(struct bt_avdtp *session,
			  struct net_buf *buf, struct bt_avdtp_req *req)
{
	int result;
	struct bt_avdtp_single_sig_hdr *hdr;

	AVDTP_LOCK();
	if (session->req != NULL) {
		return -EBUSY;
	}
	session->req = req;
	AVDTP_UNLOCK();
	hdr = (struct bt_avdtp_single_sig_hdr *)buf->data;

	result = bt_l2cap_chan_send(&session->br_chan.chan, buf);
	if (result < 0) {
		BT_ERR("Error:L2CAP send fail - result = %d", result);
		net_buf_unref(buf);
		return result;
	}

	/*Save the sent request*/
	req->sig = AVDTP_GET_SIG_ID(hdr->signal_id);
	req->tid = AVDTP_GET_TR_ID(hdr->hdr);
	BT_DBG("sig 0x%02X, tid 0x%02X", req->sig, req->tid);

	/* Init the timer */
	k_work_init_delayable(&session->req->timeout_work, avdtp_timeout);
	/* Start timeout work */
	k_work_reschedule(&session->req->timeout_work, AVDTP_TIMEOUT);
	return result;
}

static struct net_buf *avdtp_create_pdu(uint8_t msg_type,
					uint8_t pkt_type,
					uint8_t sig_id)
{
	struct net_buf *buf;
	static uint8_t tid;
	struct bt_avdtp_single_sig_hdr *hdr;

	BT_DBG("");

	buf = bt_l2cap_create_pdu(NULL, 0);

	hdr = net_buf_add(buf, sizeof(*hdr));

	hdr->hdr = (msg_type | pkt_type << AVDTP_PKT_POSITION |
			tid++ << AVDTP_TID_POSITION);
	tid %= 16; /* Loop for 16*/
	hdr->signal_id = sig_id & AVDTP_SIGID_MASK;

	BT_DBG("hdr = 0x%02X, Signal_ID = 0x%02X", hdr->hdr, hdr->signal_id);
	return buf;
}

/* Timeout handler */
static void avdtp_timeout(struct k_work *work)
{
	BT_DBG("Failed Signal_id = %d", (AVDTP_KWORK(work))->sig);

	/* add process code */
	/* Gracefully Disconnect the Signalling and streaming L2cap chann*/
	if (AVDTP_KWORK(work)) {
		switch ((AVDTP_KWORK(work))->sig) {
		case BT_AVDTP_DISCOVER:
			DISCOVER_REQ(AVDTP_KWORK(work))->status = -EPERM;
			AVDTP_KWORK(work)->func(AVDTP_KWORK(work));
			break;
		case BT_AVDTP_GET_CAPABILITIES:
			GET_CAP_REQ(AVDTP_KWORK(work))->status = -EPERM;
			AVDTP_KWORK(work)->func(AVDTP_KWORK(work));
			break;
		case BT_AVDTP_SET_CONFIGURATION:
			SET_CONF_REQ(AVDTP_KWORK(work))->status = -EPERM;
			AVDTP_KWORK(work)->func(AVDTP_KWORK(work));
			break;
		case BT_AVDTP_OPEN:
			OPEN_REQ(AVDTP_KWORK(work))->status = -EPERM;
			AVDTP_KWORK(work)->func(AVDTP_KWORK(work));
			break;
		case BT_AVDTP_START:
			START_REQ(AVDTP_KWORK(work))->status = -EPERM;
			AVDTP_KWORK(work)->func(AVDTP_KWORK(work));
			break;
		default:
			break;
		}
	}
}

/* L2CAP Interface callbacks */
void bt_avdtp_l2cap_connected(struct bt_l2cap_chan *chan)
{
	struct bt_avdtp *session;

	if (!chan) {
		BT_ERR("Invalid AVDTP chan");
		return;
	}

	session = AVDTP_CHAN(chan);
	BT_DBG("chan %p session %p", chan, session);

	/* notify a2dp connection result */
	session->ops->connected(session);
}

void bt_avdtp_l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_avdtp *session = AVDTP_CHAN(chan);

	BT_DBG("chan %p session %p", chan, session);
	session->br_chan.chan.conn = NULL;
	session->signalling_l2cap_connected = 0;
	/* todo: Clear the Pending req if set*/

	/* notify a2dp disconnect */
	session->ops->disconnected(session);
}

void bt_avdtp_l2cap_encrypt_changed(struct bt_l2cap_chan *chan, uint8_t status)
{
	BT_DBG("");
}

int bt_avdtp_l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_avdtp_single_sig_hdr *hdr;
	struct bt_avdtp *session = AVDTP_CHAN(chan);
	uint8_t i, msgtype, sigid, tid;

	if (buf->len < sizeof(*hdr)) {
		BT_ERR("Recvd Wrong AVDTP Header");
		return 0;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	msgtype = AVDTP_GET_MSG_TYPE(hdr->hdr);
	sigid = AVDTP_GET_SIG_ID(hdr->signal_id);
	tid = AVDTP_GET_TR_ID(hdr->hdr);

	BT_DBG("msg_type[0x%02x] sig_id[0x%02x] tid[0x%02x]",
		msgtype, sigid, tid);

	/* validate if there is an outstanding resp expected*/
	if (msgtype != BT_AVDTP_CMD) {
		if (session->req == NULL) {
			BT_DBG("Unexpected peer response");
			return 0;
		}

		if (session->req->sig != sigid ||
			session->req->tid != tid) {
			BT_DBG("Peer mismatch resp, expected sig[0x%02x]"
				"tid[0x%02x]", session->req->sig,
				session->req->tid);
			return 0;
		}
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
	session->br_chan.chan.required_sec_level = BT_SECURITY_L2;

	return bt_l2cap_chan_connect(conn, &session->br_chan.chan,
					 BT_L2CAP_PSM_AVDTP);
}

int bt_avdtp_disconnect(struct bt_avdtp *session)
{
	if (!session) {
		return -EINVAL;
	}

	BT_DBG("session %p", session);

	session->signalling_l2cap_connected = 0;
	return bt_l2cap_chan_disconnect(&session->br_chan.chan);
}

int bt_avdtp_l2cap_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	struct bt_avdtp *session = NULL;
	int result;

	BT_DBG("conn %p", conn);
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
		if (session->current_lsep != NULL) {
			static const struct bt_l2cap_chan_ops ops = {
				.connected = bt_avdtp_media_l2cap_connected,
				.disconnected = bt_avdtp_media_l2cap_disconnected,
				.encrypt_change = bt_avdtp_media_l2cap_encrypt_changed,
				.recv = bt_avdtp_media_l2cap_recv
			};
			session->current_lsep->session = session;
			session->current_lsep->media_br_chan.chan.ops = &ops;
			session->current_lsep->media_br_chan.rx.mtu	= BT_L2CAP_RX_MTU;
			session->current_lsep->media_br_chan.chan.required_sec_level =
					BT_SECURITY_L2;
			*chan = &session->current_lsep->media_br_chan.chan;
			session->current_lsep = NULL;
		}
	}

	return 0;
}

/* Application will register its callback */
int bt_avdtp_register(struct bt_avdtp_event_cb *cb)
{
	BT_DBG("");

	if (event_cb) {
		return -EALREADY;
	}

	event_cb = cb;

	return 0;
}

int bt_avdtp_register_sep(uint8_t media_type, uint8_t role,
			  struct bt_avdtp_seid_lsep *lsep)
{
	BT_DBG("");

	static uint8_t bt_avdtp_seid = BT_AVDTP_MIN_SEID;

	if (!lsep) {
		return -EIO;
	}

	if (bt_avdtp_seid == BT_AVDTP_MAX_SEID) {
		return -EIO;
	}

	lsep->sep.id = bt_avdtp_seid++;
	lsep->sep.inuse = 0U;
	lsep->sep.media_type = media_type;
	lsep->sep.tsep = role;
	lsep->seid_state = AVDTP_IDLE;

	lsep->next = lseps;
	lseps = lsep;

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

	BT_DBG("");

	/* Register AVDTP PSM with L2CAP */
	err = bt_l2cap_br_server_register(&avdtp_l2cap);
	if (err < 0) {
		BT_ERR("AVDTP L2CAP Registration failed %d", err);
	}

	return err;
}

/* AVDTP Discover Request */
int bt_avdtp_discover(struct bt_avdtp *session,
			  struct bt_avdtp_discover_params *param)
{
	struct net_buf *buf;

	BT_DBG("");
	if (!param || !session) {
		BT_DBG("Error: Callback/Session not valid");
		return -EINVAL;
	}

	buf = avdtp_create_pdu(BT_AVDTP_CMD,
				   BT_AVDTP_PACKET_TYPE_SINGLE,
				   BT_AVDTP_DISCOVER);
	if (!buf) {
		BT_ERR("Error: No Buff available");
		return -ENOMEM;
	}

	return avdtp_send(session, buf, &param->req);
}

struct bt_avdtp_seid_info *bt_avdtp_parse_seids(struct net_buf *buf)
{
	struct bt_avdtp_seid_info *seid = NULL;

	if (buf != NULL) {
		if (buf->len >= sizeof(*seid)) {
			seid = net_buf_pull_mem(buf, sizeof(*seid));
		}
	}

	return seid;
}

/* AVDTP Get Capabilities Request */
int bt_avdtp_get_capabilities(struct bt_avdtp *session,
			  struct bt_avdtp_get_capabilities_params *param)
{
	struct net_buf *buf;

	BT_DBG("");
	if (!param || !session) {
		BT_DBG("Error: Callback/Session not valid");
		return -EINVAL;
	}

	buf = avdtp_create_pdu(BT_AVDTP_CMD,
				   BT_AVDTP_PACKET_TYPE_SINGLE,
				   BT_AVDTP_GET_CAPABILITIES);
	if (!buf) {
		BT_ERR("Error: No Buff available");
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
		BT_DBG("Error: buf not valid");
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
				if (data == BT_A2DP_AUDIO) {
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

	BT_DBG("");
	if (!param || !session) {
		BT_DBG("Error: Callback/Session not valid");
		return -EINVAL;
	}

	buf = avdtp_create_pdu(BT_AVDTP_CMD,
				   BT_AVDTP_PACKET_TYPE_SINGLE,
				   cmd);
	if (!buf) {
		BT_ERR("Error: No Buff available");
		return -ENOMEM;
	}

	/* Body of the message */
	/* ACP Stream Endpoint ID */
	net_buf_add_u8(buf, (param->acp_stream_endpoint_id << 2u));
	/* INT Stream Endpoint ID */
	net_buf_add_u8(buf, (param->int_stream_endpoint_id << 2u));
	/* Service Category: Media Transport */
	net_buf_add_u8(buf, BT_AVDTP_SERVICE_MEDIA_TRANSPORT);
	/* LOSC */
	net_buf_add_u8(buf, 0);
	/* Service Category: Media Codec */
	net_buf_add_u8(buf, BT_AVDTP_SERVICE_MEDIA_CODEC);
	/* LOSC */
	net_buf_add_u8(buf, param->codec_ie->len + 2);
	/* Media Type */
	net_buf_add_u8(buf, param->media_type << 4U);
	/* Media Codec Type */
	net_buf_add_u8(buf, param->media_codec_type);
	/* Codec Info Element */
	net_buf_add_mem(buf, param->codec_ie->codec_ie, param->codec_ie->len);

	return avdtp_send(session, buf, &param->req);
}

int bt_avdtp_set_configuration(struct bt_avdtp *session,
			  struct bt_avdtp_set_configuration_params *param)
{
	return avdtp_process_configure_command(session, BT_AVDTP_SET_CONFIGURATION, param);
}

int bt_avdtp_reconfigure(struct bt_avdtp *session,
			  struct bt_avdtp_set_configuration_params *param)
{
	return avdtp_process_configure_command(session, BT_AVDTP_RECONFIGURE, param);
}

int bt_avdtp_open(struct bt_avdtp *session,
			  struct bt_avdtp_open_params *param)
{
	struct net_buf *buf;

	BT_DBG("");
	if (!param || !session) {
		BT_DBG("Error: Callback/Session not valid");
		return -EINVAL;
	}

	buf = avdtp_create_pdu(BT_AVDTP_CMD,
				   BT_AVDTP_PACKET_TYPE_SINGLE,
				   BT_AVDTP_OPEN);
	if (!buf) {
		BT_ERR("Error: No Buff available");
		return -ENOMEM;
	}

	/* Body of the message */
	/* ACP Stream Endpoint ID */
	net_buf_add_u8(buf, (param->acp_stream_endpoint_id << 2u));

	return avdtp_send(session, buf, &param->req);
}

int bt_avdtp_start(struct bt_avdtp *session,
			  struct bt_avdtp_start_params *param)
{
	struct net_buf *buf;

	BT_DBG("");
	if (!param || !session) {
		BT_DBG("Error: Callback/Session not valid");
		return -EINVAL;
	}

	buf = avdtp_create_pdu(BT_AVDTP_CMD,
				   BT_AVDTP_PACKET_TYPE_SINGLE,
				   BT_AVDTP_START);
	if (!buf) {
		BT_ERR("Error: No Buff available");
		return -ENOMEM;
	}

	/* Body of the message */
	/* ACP Stream Endpoint ID */
	net_buf_add_u8(buf, (param->acp_stream_endpoint_id << 2u));

	return avdtp_send(session, buf, &param->req);
}

int bt_avdtp_send_media_data(struct bt_avdtp_seid_lsep *lsep,
							struct net_buf *buf)
{
	int result;

	result = bt_l2cap_chan_send(&lsep->media_br_chan.chan, buf);
	if (result < 0) {
		BT_ERR("Error:L2CAP send fail - result = %d", result);
		return result;
	}

	return result;
}

uint32_t bt_avdtp_get_media_mtu(struct bt_avdtp_seid_lsep *lsep)
{
	return lsep->media_br_chan.tx.mtu;
}
