/** @file
 * @brief Advance Audio Distribution Profile.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright 2021,2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/avdtp.h>
#include <zephyr/bluetooth/classic/a2dp_codec_sbc.h>
#include <zephyr/bluetooth/classic/a2dp.h>

#include "common/assert.h"

#define A2DP_SBC_PAYLOAD_TYPE (0x60u)

#define A2DP_AVDTP(_avdtp) CONTAINER_OF(_avdtp, struct bt_a2dp, session)
#define DISCOVER_REQ(_req) CONTAINER_OF(_req, struct bt_avdtp_discover_params, req)
#define DISCOVER_PARAM(_discover_param) CONTAINER_OF(_discover_param, struct bt_a2dp,\
						discover_param)
#define GET_CAP_REQ(_req) CONTAINER_OF(_req, struct bt_avdtp_get_capabilities_params, req)
#define GET_CAP_PARAM(_get_cap_param) CONTAINER_OF(_get_cap_param, struct bt_a2dp,\
						get_capabilities_param)
#define SET_CONF_REQ(_req) CONTAINER_OF(_req, struct bt_avdtp_set_configuration_params, req)
#define SET_CONF_PARAM(_set_conf_param) CONTAINER_OF(_set_conf_param, struct bt_a2dp,\
						set_config_param)
#define CTRL_REQ(_req) CONTAINER_OF(_req, struct bt_avdtp_ctrl_params, req)
#define CTRL_PARAM(_ctrl_param) CONTAINER_OF(_ctrl_param, struct bt_a2dp, ctrl_param)

#include "host/hci_core.h"
#include "host/conn_internal.h"
#include "host/l2cap_internal.h"
#include "avdtp_internal.h"
#include "a2dp_internal.h"

#define LOG_LEVEL CONFIG_BT_A2DP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_a2dp);

#define A2DP_NO_SPACE (-1)

enum bt_a2dp_internal_state {
	INTERNAL_STATE_IDLE = 0,
	INTERNAL_STATE_AVDTP_CONNECTED,
};

struct bt_a2dp {
	struct bt_avdtp session;
	struct bt_avdtp_discover_params discover_param;
	struct bt_a2dp_discover_param *discover_cb_param;
	struct bt_avdtp_get_capabilities_params get_capabilities_param;
	struct bt_avdtp_set_configuration_params set_config_param;
	struct bt_avdtp_ctrl_params ctrl_param;
	uint8_t get_cap_index;
	enum bt_a2dp_internal_state a2dp_state;
	uint8_t peer_seps_count;
};

static struct bt_a2dp_cb *a2dp_cb;
K_MUTEX_DEFINE(a2dp_mutex);

#define A2DP_LOCK() k_mutex_lock(&a2dp_mutex, K_FOREVER)
#define A2DP_UNLOCK() k_mutex_unlock(&a2dp_mutex)

/* Connections */
static struct bt_a2dp connection[CONFIG_BT_MAX_CONN];

static int bt_a2dp_get_sep_caps(struct bt_a2dp *a2dp);

static void a2dp_reset(struct bt_a2dp *a2dp)
{
	(void)memset(a2dp, 0, sizeof(struct bt_a2dp));
}

static struct bt_a2dp *get_new_connection(struct bt_conn *conn)
{
	int8_t i, free;

	free = A2DP_NO_SPACE;

	if (!conn) {
		LOG_ERR("Invalid Input (err: %d)", -EINVAL);
		return NULL;
	}

	/* Find a space */
	for (i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (connection[i].session.br_chan.chan.conn == conn) {
			LOG_DBG("Conn already exists");
			return &connection[i];
		}

		if (!connection[i].session.br_chan.chan.conn &&
			free == A2DP_NO_SPACE) {
			free = i;
			break;
		}
	}

	if (free == A2DP_NO_SPACE) {
		LOG_DBG("More connection cannot be supported");
		return NULL;
	}

	/* Clean the memory area before returning */
	a2dp_reset(&connection[free]);

	return &connection[free];
}

/* The AVDTP L2CAP signal channel established */
static void a2dp_connected(struct bt_avdtp *session)
{
	struct bt_a2dp *a2dp = A2DP_AVDTP(session);

	a2dp->a2dp_state = INTERNAL_STATE_AVDTP_CONNECTED;
	/* notify a2dp app the connection. */
	if ((a2dp_cb != NULL) && (a2dp_cb->connected != NULL)) {
		a2dp_cb->connected(a2dp, 0);
	}
}

/* The AVDTP L2CAP signal channel released */
static void a2dp_disconnected(struct bt_avdtp *session)
{
	struct bt_a2dp *a2dp = A2DP_AVDTP(session);

	/* notify a2dp app the disconnection. */
	if ((a2dp_cb != NULL) && (a2dp_cb->disconnected != NULL)) {
		a2dp_cb->disconnected(a2dp);
	}
}

/* avdtp alloc buf from upper layer */
static struct net_buf *a2dp_alloc_buf(struct bt_avdtp *session)
{
	return NULL;
}

static int a2dp_discovery_ind(struct bt_avdtp *session, uint8_t *errcode)
{
	*errcode = 0;
	return 0;
}

static int a2dp_get_capabilities_ind(struct bt_avdtp *session, struct bt_avdtp_sep *sep,
		struct net_buf *rsp_buf, uint8_t *errcode)
{
	struct bt_a2dp_ep *ep;

	BT_ASSERT_MSG(sep, "Invalid sep");
	*errcode = 0;
	/* Service Category: Media Transport */
	net_buf_add_u8(rsp_buf, BT_AVDTP_SERVICE_MEDIA_TRANSPORT);
	net_buf_add_u8(rsp_buf, 0);
	/* Service Category: Media Codec */
	net_buf_add_u8(rsp_buf, BT_AVDTP_SERVICE_MEDIA_CODEC);
	ep = CONTAINER_OF(sep, struct bt_a2dp_ep, sep);
	/* Length Of Service Capability */
	net_buf_add_u8(rsp_buf, ep->codec_cap->len + 2u);
	/* Media Type */
	net_buf_add_u8(rsp_buf, sep->sep_info.media_type << 4u);
	/* Media Codec Type */
	net_buf_add_u8(rsp_buf, ep->codec_type);
	/* Codec Info Element */
	net_buf_add_mem(rsp_buf, &ep->codec_cap->codec_ie[0], ep->codec_cap->len);
	return 0;
}

static int a2dp_process_config_ind(struct bt_avdtp *session, struct bt_avdtp_sep *sep,
		uint8_t int_seid, struct net_buf *buf, uint8_t *errcode, bool reconfig)
{
	struct bt_a2dp *a2dp = A2DP_AVDTP(session);
	struct bt_a2dp_ep *ep;
	struct bt_a2dp_stream *stream = NULL;
	struct bt_a2dp_stream_ops *ops;
	uint8_t codec_type;
	uint8_t *codec_info_element;
	uint16_t codec_info_element_len;
	int err;

	*errcode = 0;

	BT_ASSERT_MSG(sep, "Invalid sep");

	ep = CONTAINER_OF(sep, struct bt_a2dp_ep, sep);

	/* parse the configuration */
	codec_info_element_len = 4U;
	err = bt_avdtp_parse_capability_codec(buf,
				&codec_type, &codec_info_element,
				&codec_info_element_len);
	if (err) {
		*errcode = BT_AVDTP_BAD_ACP_SEID;
		return -EINVAL;
	}

	if (codec_type == BT_A2DP_SBC) {
		struct bt_a2dp_codec_sbc_params *sbc_set;
		struct bt_a2dp_codec_sbc_params *sbc;

		if (codec_info_element_len != 4U) {
			*errcode = BT_AVDTP_BAD_ACP_SEID;
			return -EINVAL;
		}

		sbc_set = (struct bt_a2dp_codec_sbc_params *)codec_info_element;
		sbc = (struct bt_a2dp_codec_sbc_params *)&ep->codec_cap->codec_ie[0];
		if (((BT_A2DP_SBC_SAMP_FREQ(sbc_set) & BT_A2DP_SBC_SAMP_FREQ(sbc)) == 0) ||
			((BT_A2DP_SBC_CHAN_MODE(sbc_set) & BT_A2DP_SBC_CHAN_MODE(sbc)) == 0) ||
			((BT_A2DP_SBC_BLK_LEN(sbc_set) & BT_A2DP_SBC_BLK_LEN(sbc)) == 0) ||
			((BT_A2DP_SBC_SUB_BAND(sbc_set) & BT_A2DP_SBC_SUB_BAND(sbc)) == 0) ||
			((BT_A2DP_SBC_ALLOC_MTHD(sbc_set) & BT_A2DP_SBC_ALLOC_MTHD(sbc)) == 0)) {
			*errcode = BT_AVDTP_BAD_ACP_SEID;
			return -EINVAL;
		}
	}

	/* For reconfig, ep->stream must already be valid.
	 * For !reconfig, config_req must be set to get stream from upper layer
	 */
	if (reconfig) {
		stream = ep->stream;
		if (stream == NULL) {
			*errcode = BT_AVDTP_BAD_ACP_SEID;
			return -EINVAL;
		}
	} else if (!reconfig && (a2dp_cb == NULL || a2dp_cb->config_req == NULL)) {
		*errcode = BT_AVDTP_BAD_ACP_SEID;
		return -EINVAL;
	}

	if ((a2dp_cb != NULL) && (!reconfig || (reconfig && a2dp_cb->reconfig_req != NULL))) {
		struct bt_a2dp_codec_cfg cfg;
		struct bt_a2dp_codec_ie codec_config;
		uint8_t rsp_err_code;

		cfg.codec_config = &codec_config;
		cfg.codec_config->len = codec_info_element_len;
		memcpy(&cfg.codec_config->codec_ie[0], codec_info_element,
			(codec_info_element_len > A2DP_MAX_IE_LENGTH ?
			A2DP_MAX_IE_LENGTH : codec_info_element_len));
		if (!reconfig) {
			err = a2dp_cb->config_req(a2dp, ep, &cfg, &stream, &rsp_err_code);
			if (!err && stream) {
				stream->a2dp = a2dp;
				stream->local_ep = ep;
				stream->remote_ep_id = int_seid;
				stream->remote_ep = NULL;
				stream->codec_config = *cfg.codec_config;
				ep->stream = stream;
			} else {
				*errcode = rsp_err_code != 0 ? rsp_err_code : BT_AVDTP_BAD_ACP_SEID;
			}
		} else {
			err = a2dp_cb->reconfig_req(stream, &cfg, &rsp_err_code);
			if (err) {
				*errcode = rsp_err_code;
			}
		}
	}

	if (*errcode == 0) {
		ops = stream->ops;
		if ((ops != NULL) && (ops->configured != NULL)) {
			ops->configured(stream);
		}
	}

	return (*errcode == 0) ? 0 : -EINVAL;
}

static int a2dp_set_config_ind(struct bt_avdtp *session, struct bt_avdtp_sep *sep,
		uint8_t int_seid, struct net_buf *buf, uint8_t *errcode)
{
	BT_ASSERT_MSG(sep, "Invalid sep");
	return a2dp_process_config_ind(session, sep, int_seid, buf, errcode, false);
}

static int a2dp_re_config_ind(struct bt_avdtp *session, struct bt_avdtp_sep *sep,
		uint8_t int_seid, struct net_buf *buf, uint8_t *errcode)
{
	BT_ASSERT_MSG(sep, "Invalid sep");
	return a2dp_process_config_ind(session, sep, int_seid, buf, errcode, true);
}

#if defined(CONFIG_BT_A2DP_SINK)
static void bt_a2dp_media_data_callback(
		struct bt_avdtp_sep *sep,
		struct net_buf *buf)
{
	struct bt_avdtp_media_hdr *media_hdr;
	struct bt_a2dp_ep *ep;
	struct bt_a2dp_stream *stream;

	BT_ASSERT_MSG(sep, "Invalid sep");
	ep = CONTAINER_OF(sep, struct bt_a2dp_ep, sep);
	if (ep->stream == NULL) {
		return;
	}
	stream = ep->stream;

	media_hdr = net_buf_pull_mem(buf, sizeof(*media_hdr));

	stream->ops->recv(stream, buf,
		sys_get_be16((uint8_t *)&media_hdr->sequence_number),
		sys_get_be32((uint8_t *)&media_hdr->time_stamp));
}
#endif

static int a2dp_ctrl_ind(struct bt_avdtp *session,
	struct bt_avdtp_sep *sep, uint8_t *errcode,
	int (*req_fun)(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code),
	void (*done_fun)(struct bt_a2dp_stream *stream),
	bool clear_stream)
{
	struct bt_a2dp_ep *ep;
	struct bt_a2dp_stream *stream;

	*errcode = 0;
	ep = CONTAINER_OF(sep, struct bt_a2dp_ep, sep);
	if (ep->stream == NULL) {
		*errcode = BT_AVDTP_BAD_ACP_SEID;
		return -EINVAL;
	}
	stream = ep->stream;

	if (req_fun != NULL) {
		uint8_t rsp_err_code;
		int err;

		err = req_fun(stream, &rsp_err_code);
		if (err) {
			*errcode = rsp_err_code;
		}
	}

	if (*errcode == 0) {
		if (clear_stream) {
			ep->stream = NULL;
		}

		if (done_fun != NULL) {
			done_fun(stream);
		}
	}

	return (*errcode == 0) ? 0 : -EINVAL;
}

static int a2dp_open_ind(struct bt_avdtp *session, struct bt_avdtp_sep *sep, uint8_t *errcode)
{
	struct bt_a2dp_ep *ep = CONTAINER_OF(sep, struct bt_a2dp_ep, sep);

	BT_ASSERT_MSG(sep, "Invalid sep");
	return a2dp_ctrl_ind(session, sep, errcode,
		a2dp_cb != NULL ? a2dp_cb->establish_req : NULL,
		(ep->stream != NULL && ep->stream->ops != NULL) ?
		ep->stream->ops->established : NULL,
		false);
}

static int a2dp_start_ind(struct bt_avdtp *session, struct bt_avdtp_sep *sep, uint8_t *errcode)
{
	struct bt_a2dp_ep *ep = CONTAINER_OF(sep, struct bt_a2dp_ep, sep);

	BT_ASSERT_MSG(sep, "Invalid sep");
	return a2dp_ctrl_ind(session, sep, errcode,
		a2dp_cb != NULL ? a2dp_cb->start_req : NULL,
		(ep->stream != NULL && ep->stream->ops != NULL) ?
		ep->stream->ops->started : NULL,
		false);
}

static int a2dp_suspend_ind(struct bt_avdtp *session, struct bt_avdtp_sep *sep, uint8_t *errcode)
{
	struct bt_a2dp_ep *ep = CONTAINER_OF(sep, struct bt_a2dp_ep, sep);

	BT_ASSERT_MSG(sep, "Invalid sep");
	return a2dp_ctrl_ind(session, sep, errcode,
		a2dp_cb != NULL ? a2dp_cb->suspend_req : NULL,
		(ep->stream != NULL && ep->stream->ops != NULL) ?
		ep->stream->ops->suspended : NULL,
		false);
}

static int a2dp_close_ind(struct bt_avdtp *session, struct bt_avdtp_sep *sep, uint8_t *errcode)
{
	struct bt_a2dp_ep *ep = CONTAINER_OF(sep, struct bt_a2dp_ep, sep);

	BT_ASSERT_MSG(sep, "Invalid sep");
	return a2dp_ctrl_ind(session, sep, errcode,
		a2dp_cb != NULL ? a2dp_cb->release_req : NULL,
		(ep->stream != NULL && ep->stream->ops != NULL) ?
		ep->stream->ops->released : NULL,
		true);
}

static int a2dp_abort_ind(struct bt_avdtp *session, struct bt_avdtp_sep *sep, uint8_t *errcode)
{
	struct bt_a2dp_ep *ep = CONTAINER_OF(sep, struct bt_a2dp_ep, sep);

	BT_ASSERT_MSG(sep, "Invalid sep");
	return a2dp_ctrl_ind(session, sep, errcode,
		a2dp_cb != NULL ? a2dp_cb->abort_req : NULL,
		(ep->stream != NULL && ep->stream->ops != NULL) ?
		ep->stream->ops->aborted : NULL,
		true);
}

static int bt_a2dp_set_config_cb(struct bt_avdtp_req *req)
{
	struct bt_a2dp *a2dp = SET_CONF_PARAM(SET_CONF_REQ(req));
	struct bt_a2dp_ep *ep;
	struct bt_a2dp_stream *stream;
	struct bt_a2dp_stream_ops *ops;

	ep = CONTAINER_OF(a2dp->set_config_param.sep, struct bt_a2dp_ep, sep);
	if (ep->stream == NULL) {
		return -EINVAL;
	}
	if ((ep->stream == NULL) || (SET_CONF_REQ(req) != &a2dp->set_config_param)) {
		return -EINVAL;
	}
	stream = ep->stream;

	LOG_DBG("SET CONFIGURATION result:%d", req->status);

	if ((a2dp_cb != NULL) && (a2dp_cb->config_rsp != NULL)) {
		a2dp_cb->config_rsp(stream, req->status);
	}

	ops = stream->ops;
	if ((!req->status) && (ops->configured != NULL)) {
		ops->configured(stream);
	}
	return 0;
}

static int bt_a2dp_get_capabilities_cb(struct bt_avdtp_req *req)
{
	int err;
	uint8_t *codec_info_element;
	struct bt_a2dp *a2dp = GET_CAP_PARAM(GET_CAP_REQ(req));
	uint16_t codec_info_element_len;
	uint8_t codec_type;
	uint8_t user_ret;

	if (GET_CAP_REQ(req) != &a2dp->get_capabilities_param) {
		return -EINVAL;
	}

	LOG_DBG("GET CAPABILITIES result:%d", req->status);
	if (req->status) {
		if ((a2dp->discover_cb_param != NULL) &&
		(a2dp->discover_cb_param->cb != NULL)) {
			a2dp->discover_cb_param->cb(a2dp, NULL, NULL);
			a2dp->discover_cb_param = NULL;
		}
		return 0;
	}

	err = bt_avdtp_parse_capability_codec(a2dp->get_capabilities_param.buf,
			&codec_type, &codec_info_element, &codec_info_element_len);
	if (err) {
		LOG_DBG("codec capability parsing fail");
		return 0;
	}

	if (codec_info_element_len > A2DP_MAX_IE_LENGTH) {
		codec_info_element_len = A2DP_MAX_IE_LENGTH;
	}

	if ((a2dp->discover_cb_param != NULL) && (a2dp->discover_cb_param->cb != NULL)) {
		struct bt_a2dp_ep *ep;
		struct bt_a2dp_ep_info *info = &a2dp->discover_cb_param->info;

		info->codec_type = codec_type;
		info->sep_info = a2dp->discover_cb_param->seps_info[a2dp->get_cap_index];
		memcpy(&info->codec_cap.codec_ie,
			codec_info_element, codec_info_element_len);
		info->codec_cap.len = codec_info_element_len;
		user_ret = a2dp->discover_cb_param->cb(a2dp, info, &ep);
		if (ep != NULL) {
			ep->codec_type = info->codec_type;
			ep->sep.sep_info = info->sep_info;
			*ep->codec_cap = info->codec_cap;
			ep->stream = NULL;
		}

		if (user_ret == BT_A2DP_DISCOVER_EP_CONTINUE) {
			if (bt_a2dp_get_sep_caps(a2dp) != 0) {
				a2dp->discover_cb_param->cb(a2dp, NULL, NULL);
				a2dp->discover_cb_param = NULL;
			}
		} else {
			a2dp->discover_cb_param = NULL;
		}
	}

	return 0;
}

static int bt_a2dp_get_sep_caps(struct bt_a2dp *a2dp)
{
	int err;

	if ((a2dp->discover_cb_param == NULL) || (a2dp->discover_cb_param->cb == NULL)) {
		return -EINVAL;
	}

	if (a2dp->get_cap_index == 0xFFu) {
		a2dp->get_cap_index = 0U;
	} else {
		a2dp->get_cap_index++;
	}

	for (; a2dp->get_cap_index < a2dp->peer_seps_count;
			a2dp->get_cap_index++) {
		if (a2dp->discover_cb_param->seps_info[a2dp->get_cap_index].media_type ==
			BT_AVDTP_AUDIO) {
			a2dp->get_capabilities_param.req.func = bt_a2dp_get_capabilities_cb;
			a2dp->get_capabilities_param.buf = NULL;
			a2dp->get_capabilities_param.stream_endpoint_id =
				a2dp->discover_cb_param->seps_info[a2dp->get_cap_index].id;
			err = bt_avdtp_get_capabilities(&a2dp->session,
				&a2dp->get_capabilities_param);
			if (err) {
				LOG_DBG("AVDTP get codec_cap failed");
				a2dp->discover_cb_param->cb(a2dp, NULL, NULL);
				a2dp->discover_cb_param = NULL;
			}
			return 0;
		}
	}
	return -EINVAL;
}

static int bt_a2dp_discover_cb(struct bt_avdtp_req *req)
{
	struct bt_a2dp *a2dp = DISCOVER_PARAM(DISCOVER_REQ(req));
	struct bt_avdtp_sep_info *sep_info;
	int err;

	LOG_DBG("DISCOVER result:%d", req->status);
	if (a2dp->discover_cb_param == NULL) {
		return -EINVAL;
	}
	a2dp->peer_seps_count = 0U;
	if (!(req->status)) {
		if (a2dp->discover_cb_param->sep_count == 0) {
			if (a2dp->discover_cb_param->cb != NULL) {
				a2dp->discover_cb_param->cb(a2dp, NULL, NULL);
				a2dp->discover_cb_param = NULL;
			}
			return -EINVAL;
		}

		do {
			sep_info = &a2dp->discover_cb_param->seps_info[a2dp->peer_seps_count];
			err = bt_avdtp_parse_sep(DISCOVER_REQ(req)->buf, sep_info);
			if (err) {
				break;
			}
			a2dp->peer_seps_count++;
			LOG_DBG("id:%d, inuse:%d, media_type:%d, tsep:%d, ",
				sep_info->id,
				sep_info->inuse,
				sep_info->media_type,
				sep_info->tsep);
		} while (a2dp->peer_seps_count < a2dp->discover_cb_param->sep_count);

		/* trigger the getting capability */
		a2dp->get_cap_index = 0xFFu;
		if (bt_a2dp_get_sep_caps(a2dp) != 0) {
			/* the peer_seps' caps is done.*/
			if (a2dp->discover_cb_param->cb != NULL) {
				a2dp->discover_cb_param->cb(a2dp, NULL, NULL);
				a2dp->discover_cb_param = NULL;
			}
		}
	} else {
		if (a2dp->discover_cb_param->cb != NULL) {
			a2dp->discover_cb_param->cb(a2dp, NULL, NULL);
			a2dp->discover_cb_param = NULL;
		}
	}

	return 0;
}

int bt_a2dp_discover(struct bt_a2dp *a2dp, struct bt_a2dp_discover_param *param)
{
	int err;

	if ((a2dp == NULL) || (param == NULL)) {
		return -EINVAL;
	}

	if (a2dp->a2dp_state != INTERNAL_STATE_AVDTP_CONNECTED) {
		return -EIO;
	}

	if (a2dp->discover_cb_param != NULL) {
		return -EBUSY;
	}

	a2dp->discover_cb_param = param;
	a2dp->discover_param.req.func = bt_a2dp_discover_cb;
	a2dp->discover_param.buf = NULL;
	err = bt_avdtp_discover(&a2dp->session, &a2dp->discover_param);
	if (err) {
		if (a2dp->discover_cb_param->cb != NULL) {
			a2dp->discover_cb_param->cb(a2dp, NULL, NULL);
		}
		a2dp->discover_cb_param = NULL;
	}

	return err;
}

void bt_a2dp_stream_cb_register(struct bt_a2dp_stream *stream, struct bt_a2dp_stream_ops *ops)
{
	BT_ASSERT(stream);
	stream->ops = ops;
}

static inline void bt_a2dp_stream_config_set_param(struct bt_a2dp *a2dp,
		struct bt_a2dp_codec_cfg *config,
		bt_avdtp_func_t cb, uint8_t remote_id, uint8_t int_id,
		uint8_t codec_type, struct bt_avdtp_sep *sep)
{
	a2dp->set_config_param.req.func = cb;
	a2dp->set_config_param.acp_stream_ep_id = remote_id;
	a2dp->set_config_param.int_stream_endpoint_id = int_id;
	a2dp->set_config_param.media_type = BT_AVDTP_AUDIO;
	a2dp->set_config_param.media_codec_type = codec_type;
	a2dp->set_config_param.codec_specific_ie_len = config->codec_config->len;
	a2dp->set_config_param.codec_specific_ie = &config->codec_config->codec_ie[0];
	a2dp->set_config_param.sep = sep;
}

int bt_a2dp_stream_config(struct bt_a2dp *a2dp, struct bt_a2dp_stream *stream,
		struct bt_a2dp_ep *local_ep, struct bt_a2dp_ep *remote_ep,
		struct bt_a2dp_codec_cfg *config)
{
	if ((a2dp == NULL) || (stream == NULL) || (local_ep == NULL) ||
		(remote_ep == NULL) || (config == NULL)) {
		return -EINVAL;
	}

	if ((local_ep->sep.sep_info.tsep == remote_ep->sep.sep_info.tsep) ||
	    (local_ep->codec_type != remote_ep->codec_type)) {
		return -EINVAL;
	}

	stream->local_ep = local_ep;
	stream->remote_ep = remote_ep;
	stream->remote_ep_id = remote_ep->sep.sep_info.id;
	stream->a2dp = a2dp;
	local_ep->stream = stream;
	remote_ep->stream = stream;
	bt_a2dp_stream_config_set_param(a2dp, config, bt_a2dp_set_config_cb,
			remote_ep->sep.sep_info.id, local_ep->sep.sep_info.id,
			local_ep->codec_type, &local_ep->sep);
	return bt_avdtp_set_configuration(&a2dp->session, &a2dp->set_config_param);
}

static int bt_a2dp_ctrl_cb(struct bt_avdtp_req *req,
	void (*rsp_fun)(struct bt_a2dp_stream *stream, uint8_t rsp_err_code),
	void (*done_fun)(struct bt_a2dp_stream *stream),
	bool cler_stream)
{
	struct bt_a2dp *a2dp = CTRL_PARAM(CTRL_REQ(req));
	struct bt_a2dp_ep *ep;
	struct bt_a2dp_stream *stream;

	ep = CONTAINER_OF(a2dp->ctrl_param.sep, struct bt_a2dp_ep, sep);
	if ((ep->stream == NULL) || (CTRL_REQ(req) != &a2dp->ctrl_param)) {
		return -EINVAL;
	}
	stream = ep->stream;
	if (cler_stream) {
		ep->stream = NULL;
	}

	LOG_DBG("ctrl result:%d", req->status);

	if (rsp_fun != NULL) {
		rsp_fun(stream, req->status);
	}

	if ((!req->status) && (done_fun != NULL)) {
		done_fun(stream);
	}
	return 0;
}

static int bt_a2dp_open_cb(struct bt_avdtp_req *req)
{
	struct bt_a2dp_ep *ep = CONTAINER_OF(CTRL_REQ(req)->sep, struct bt_a2dp_ep, sep);

	return bt_a2dp_ctrl_cb(req, a2dp_cb != NULL ? a2dp_cb->establish_rsp : NULL,
		(ep->stream != NULL && ep->stream->ops != NULL) ?
		ep->stream->ops->established : NULL,
		false);
}

static int bt_a2dp_start_cb(struct bt_avdtp_req *req)
{
	struct bt_a2dp_ep *ep = CONTAINER_OF(CTRL_REQ(req)->sep, struct bt_a2dp_ep, sep);

	return bt_a2dp_ctrl_cb(req, a2dp_cb != NULL ? a2dp_cb->start_rsp : NULL,
		(ep->stream != NULL && ep->stream->ops != NULL) ?
		ep->stream->ops->started : NULL,
		false);
}

static int bt_a2dp_suspend_cb(struct bt_avdtp_req *req)
{
	struct bt_a2dp_ep *ep = CONTAINER_OF(CTRL_REQ(req)->sep, struct bt_a2dp_ep, sep);

	return bt_a2dp_ctrl_cb(req, a2dp_cb != NULL ? a2dp_cb->suspend_rsp : NULL,
		(ep->stream != NULL && ep->stream->ops != NULL) ?
		ep->stream->ops->suspended : NULL,
		false);
}

static int bt_a2dp_close_cb(struct bt_avdtp_req *req)
{
	struct bt_a2dp_ep *ep = CONTAINER_OF(CTRL_REQ(req)->sep, struct bt_a2dp_ep, sep);

	return bt_a2dp_ctrl_cb(req, a2dp_cb != NULL ? a2dp_cb->release_rsp : NULL,
		(ep->stream != NULL && ep->stream->ops != NULL) ?
		ep->stream->ops->released : NULL,
		true);
}

static int bt_a2dp_abort_cb(struct bt_avdtp_req *req)
{
	struct bt_a2dp_ep *ep = CONTAINER_OF(CTRL_REQ(req)->sep, struct bt_a2dp_ep, sep);

	return bt_a2dp_ctrl_cb(req, a2dp_cb != NULL ? a2dp_cb->abort_rsp : NULL,
		(ep->stream != NULL && ep->stream->ops != NULL) ?
		ep->stream->ops->aborted : NULL,
		true);
}

static int bt_a2dp_stream_ctrl_pre(struct bt_a2dp_stream *stream, bt_avdtp_func_t cb)
{
	struct bt_a2dp *a2dp;

	if ((stream == NULL) || (stream->local_ep == NULL) || (stream->a2dp == NULL)) {
		return -EINVAL;
	}

	a2dp = stream->a2dp;
	a2dp->ctrl_param.req.func = cb;
	a2dp->ctrl_param.acp_stream_ep_id = stream->remote_ep != NULL ?
		stream->remote_ep->sep.sep_info.id : stream->remote_ep_id;
	a2dp->ctrl_param.sep = &stream->local_ep->sep;
	return 0;
}

int bt_a2dp_stream_establish(struct bt_a2dp_stream *stream)
{
	int err;
	struct bt_a2dp *a2dp = stream->a2dp;

	err = bt_a2dp_stream_ctrl_pre(stream, bt_a2dp_open_cb);
	if (err) {
		return err;
	}
	return bt_avdtp_open(&a2dp->session, &a2dp->ctrl_param);
}

int bt_a2dp_stream_release(struct bt_a2dp_stream *stream)
{
	int err;
	struct bt_a2dp *a2dp = stream->a2dp;

	err = bt_a2dp_stream_ctrl_pre(stream, bt_a2dp_close_cb);
	if (err) {
		return err;
	}
	return bt_avdtp_close(&a2dp->session, &a2dp->ctrl_param);
}

int bt_a2dp_stream_start(struct bt_a2dp_stream *stream)
{
	int err;
	struct bt_a2dp *a2dp = stream->a2dp;

	err = bt_a2dp_stream_ctrl_pre(stream, bt_a2dp_start_cb);
	if (err) {
		return err;
	}
	return bt_avdtp_start(&a2dp->session, &a2dp->ctrl_param);
}

int bt_a2dp_stream_suspend(struct bt_a2dp_stream *stream)
{
	int err;
	struct bt_a2dp *a2dp = stream->a2dp;

	err = bt_a2dp_stream_ctrl_pre(stream, bt_a2dp_suspend_cb);
	if (err) {
		return err;
	}
	return bt_avdtp_suspend(&a2dp->session, &a2dp->ctrl_param);
}

int bt_a2dp_stream_abort(struct bt_a2dp_stream *stream)
{
	int err;
	struct bt_a2dp *a2dp = stream->a2dp;

	err = bt_a2dp_stream_ctrl_pre(stream, bt_a2dp_abort_cb);
	if (err) {
		return err;
	}
	return bt_avdtp_abort(&a2dp->session, &a2dp->ctrl_param);
}

int bt_a2dp_stream_reconfig(struct bt_a2dp_stream *stream, struct bt_a2dp_codec_cfg *config)
{
	if ((stream == NULL) || (config == NULL)) {
		return -EINVAL;
	}

	bt_a2dp_stream_config_set_param(stream->a2dp, config, bt_a2dp_set_config_cb,
			stream->remote_ep != NULL ? stream->remote_ep->sep.sep_info.id :
			stream->remote_ep_id,
			stream->local_ep->sep.sep_info.id,
			stream->local_ep->codec_type, &stream->local_ep->sep);
	return bt_avdtp_reconfigure(&stream->a2dp->session, &stream->a2dp->set_config_param);
}

uint32_t bt_a2dp_get_mtu(struct bt_a2dp_stream *stream)
{
	if ((stream == NULL) || (stream->local_ep == NULL)) {
		return -EINVAL;
	}

	return bt_avdtp_get_media_mtu(&stream->local_ep->sep);
}

#if defined(CONFIG_BT_A2DP_SOURCE)
int bt_a2dp_stream_send(struct bt_a2dp_stream *stream,  struct net_buf *buf,
			uint16_t seq_num, uint32_t ts)
{
	struct bt_avdtp_media_hdr *media_hdr;

	if (stream == NULL) {
		return -EINVAL;
	}

	media_hdr = net_buf_push(buf, sizeof(struct bt_avdtp_media_hdr));
	if (stream->local_ep->codec_type == BT_A2DP_SBC) {
		media_hdr->playload_type = A2DP_SBC_PAYLOAD_TYPE;
	}

	memset(media_hdr, 0, sizeof(struct bt_avdtp_media_hdr));
	media_hdr->synchronization_source = 0U;
	/* update time_stamp in the buf */
	sys_put_be32(ts, (uint8_t *)&media_hdr->time_stamp);
	/* update sequence_number in the buf */
	sys_put_be16(seq_num, (uint8_t *)&media_hdr->sequence_number);
	/* send the buf */
	return bt_avdtp_send_media_data(&stream->local_ep->sep, buf);
}
#endif

int a2dp_stream_l2cap_disconnected(struct bt_avdtp *session,
		struct bt_avdtp_sep *sep)
{
	struct bt_a2dp_ep *ep;

	BT_ASSERT_MSG(sep, "Invalid sep");
	ep = CONTAINER_OF(sep, struct bt_a2dp_ep, sep);
	if (ep->stream != NULL) {
		struct bt_a2dp_stream_ops *ops;
		struct bt_a2dp_stream *stream = ep->stream;

		ops = stream->ops;
		/* Many places set ep->stream as NULL like abort and close.
		 * it should be OK without lock protection (A2DP_LOCK) because
		 * all the related callbacks are in the same zephyr task context.
		 */
		ep->stream = NULL;
		if ((ops != NULL) && (ops->released != NULL)) {
			ops->released(stream);
		}
	}

	return 0;
}

static const struct bt_avdtp_ops_cb signaling_avdtp_ops = {
	.connected = a2dp_connected,
	.disconnected = a2dp_disconnected,
	.alloc_buf = a2dp_alloc_buf,
	.discovery_ind = a2dp_discovery_ind,
	.get_capabilities_ind = a2dp_get_capabilities_ind,
	.set_configuration_ind = a2dp_set_config_ind,
	.re_configuration_ind = a2dp_re_config_ind,
	.open_ind = a2dp_open_ind,
	.start_ind = a2dp_start_ind,
	.close_ind = a2dp_close_ind,
	.suspend_ind = a2dp_suspend_ind,
	.abort_ind = a2dp_abort_ind,
	.stream_l2cap_disconnected = a2dp_stream_l2cap_disconnected,
};

int a2dp_accept(struct bt_conn *conn, struct bt_avdtp **session)
{
	struct bt_a2dp *a2dp;

	a2dp = get_new_connection(conn);
	if (!a2dp) {
		return -ENOMEM;
	}

	*session = &(a2dp->session);
	a2dp->session.ops = &signaling_avdtp_ops;
	LOG_DBG("session: %p", &(a2dp->session));

	return 0;
}

/* The above callback structures need to be packed and passed to AVDTP */
static struct bt_avdtp_event_cb avdtp_cb = {
	.accept = a2dp_accept,
};

int bt_a2dp_init(void)
{
	int err;

	/* Register event handlers with AVDTP */
	err = bt_avdtp_register(&avdtp_cb);
	if (err < 0) {
		LOG_ERR("A2DP registration failed");
		return err;
	}

	for (uint8_t i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		memset(&connection[i], 0, sizeof(struct bt_a2dp));
	}

	LOG_DBG("A2DP Initialized successfully.");
	return 0;
}

struct bt_a2dp *bt_a2dp_connect(struct bt_conn *conn)
{
	struct bt_a2dp *a2dp;
	int err;

	A2DP_LOCK();
	a2dp = get_new_connection(conn);
	if (!a2dp) {
		A2DP_UNLOCK();
		LOG_ERR("Cannot allocate memory");
		return NULL;
	}

	a2dp->a2dp_state = INTERNAL_STATE_IDLE;

	a2dp->session.ops = &signaling_avdtp_ops;
	err = bt_avdtp_connect(conn, &(a2dp->session));
	if (err < 0) {
		/* If error occurs, undo the saving and return the error */
		a2dp_reset(a2dp);
		A2DP_UNLOCK();
		LOG_DBG("AVDTP Connect failed");
		return NULL;
	}

	A2DP_UNLOCK();
	LOG_DBG("Connect request sent");
	return a2dp;
}

int bt_a2dp_disconnect(struct bt_a2dp *a2dp)
{
	BT_ASSERT(a2dp);
	return bt_avdtp_disconnect(&a2dp->session);
}

int bt_a2dp_register_ep(struct bt_a2dp_ep *ep, uint8_t media_type, uint8_t role)
{
	int err;

	BT_ASSERT(ep);

#if defined(CONFIG_BT_A2DP_SINK)
	if (role == BT_AVDTP_SINK) {
		ep->sep.media_data_cb = bt_a2dp_media_data_callback;
	} else {
		ep->sep.media_data_cb = NULL;
	}
#else
	ep->sep.media_data_cb = NULL;
#endif
	A2DP_LOCK();
	err = bt_avdtp_register_sep(media_type, role, &(ep->sep));
	A2DP_UNLOCK();
	if (err < 0) {
		return err;
	}

	return 0;
}

int bt_a2dp_register_cb(struct bt_a2dp_cb *cb)
{
	a2dp_cb = cb;
	return 0;
}
