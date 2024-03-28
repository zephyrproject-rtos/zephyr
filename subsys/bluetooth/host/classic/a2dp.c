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

#define A2DP_AVDTP(_avdtp) CONTAINER_OF(_avdtp, struct bt_a2dp, session)
#define DISCOVER_REQ(_req) CONTAINER_OF(_req, struct bt_avdtp_discover_params, req)
#define DISCOVER_PARAM(_discover_param) CONTAINER_OF(_discover_param, struct bt_a2dp,\
						discover_param)
#define GET_CAP_REQ(_req) CONTAINER_OF(_req, struct bt_avdtp_get_capabilities_params, req)
#define GET_CAP_PARAM(_get_cap_param) CONTAINER_OF(_get_cap_param, struct bt_a2dp,\
						get_capabilities_param)
#define SET_CONF_REQ(_req) CONTAINER_OF(_req, struct bt_avdtp_set_configuration_params, req)
#define SET_CONF_PARAM(_set_conf_param) CONTAINER_OF(_set_conf_param, struct bt_a2dp,\
						set_configuration_param)
#define OPEN_REQ(_req) CONTAINER_OF(_req, struct bt_avdtp_open_params, req)
#define OPEN_PARAM(_open_param) CONTAINER_OF(_open_param, struct bt_a2dp, open_param)
#define START_REQ(_req) CONTAINER_OF(_req, struct bt_avdtp_start_params, req)
#define START_PARAM(_start_param) CONTAINER_OF(_start_param, struct bt_a2dp, start_param)

#include "host/hci_core.h"
#include "host/conn_internal.h"
#include "host/l2cap_internal.h"
#include "avdtp_internal.h"
#include "a2dp_internal.h"

#define LOG_LEVEL CONFIG_BT_A2DP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_a2dp);

#define A2DP_NO_SPACE (-1)
#define A2DP_MAX_IE_LENGTH (32U)
#define A2DP_MSG_MAX_COUNT (10)
#define A2DP_SBC_BLOCK_MAX (512U)

struct a2dp_task_msg {
	void *parameter;
	uint8_t event;
};

enum bt_a2dp_internal_state {
	INTERNAL_STATE_IDLE = 0,
	INTERNAL_STATE_AVDTP_CONNECTED,
};

struct bt_a2dp_codec_ie_internal {
	uint8_t len;
	uint8_t codec_ie[A2DP_MAX_IE_LENGTH];
};

struct bt_a2dp_endpoint_internal {
	/* the a2dp connection */
	struct bt_a2dp *a2dp;
	/* the corresponding endpoint */
	struct bt_a2dp_endpoint *endpoint;
	/* remote SEP Code Type */
	uint8_t remote_codec_type;
	/* the config result */
	struct bt_a2dp_codec_ie_internal config_internal;
	/* remote SEP info */
	struct bt_avdtp_seid_info remote_ep_info;
	/* remote SEP capability */
	struct bt_a2dp_codec_ie_internal remote_ep_ie;
};

struct bt_a2dp {
	struct bt_avdtp session;
	struct bt_avdtp_discover_params discover_param;
	struct bt_avdtp_get_capabilities_params get_capabilities_param;
	struct bt_avdtp_set_configuration_params set_configuration_param;
	struct bt_avdtp_open_params open_param;
	struct bt_avdtp_start_params start_param;
	void (*configure_cb)(int err);
	bt_a2dp_discover_peer_endpoint_cb_t discover_endpoint_cb;
	struct bt_avdtp_seid_info peer_seids[CONFIG_BT_A2DP_MAX_ENDPOINT_COUNT];
	uint8_t auto_select_endpoint_index;
	uint8_t get_capabilities_index;
	enum bt_a2dp_internal_state a2dp_state;
	uint8_t peer_seids_count;
	uint8_t auto_configure_enabled : 1;
};

struct bt_a2dp_connect_cb connect_cb;
static struct bt_a2dp_endpoint_internal endpoint_internals[CONFIG_BT_A2DP_MAX_ENDPOINT_COUNT];
K_MSGQ_DEFINE(a2dp_msgq, sizeof(struct a2dp_task_msg), A2DP_MSG_MAX_COUNT, 4);
K_MUTEX_DEFINE(a2dp_mutex);

#define A2DP_LOCK() k_mutex_lock(&a2dp_mutex, K_FOREVER)
#define A2DP_UNLOCK() k_mutex_unlock(&a2dp_mutex)

/* Connections */
static struct bt_a2dp connection[CONFIG_BT_MAX_CONN];

static int bt_a2dp_get_seid_caps(struct bt_a2dp *a2dp);

static struct bt_a2dp_endpoint_internal *bt_a2dp_get_endpoint_state(
	struct bt_a2dp_endpoint *endpoint)
{
	for (uint8_t index = 0; index < CONFIG_BT_A2DP_MAX_ENDPOINT_COUNT; ++index) {
		if (endpoint_internals[index].endpoint == endpoint) {
			return &endpoint_internals[index];
		}
	}
	return NULL;
}

static uint8_t bt_a2dp_get_endpoints_count(void)
{
	uint8_t index;

	for (index = 0; index < CONFIG_BT_A2DP_MAX_ENDPOINT_COUNT; index++) {
		if (endpoint_internals[index].endpoint == NULL) {
			break;
		}
	}
	return index;
}

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

void avdtp_connected(struct bt_avdtp *session)
{
	struct bt_a2dp *a2dp = A2DP_AVDTP(session);

	a2dp->a2dp_state = INTERNAL_STATE_AVDTP_CONNECTED;
	/* notify a2dp app the connection. */
	if (connect_cb.connected != NULL) {
		connect_cb.connected(a2dp, 0);
	}
}

void avdtp_disconnected(struct bt_avdtp *session)
{
	struct bt_a2dp *a2dp = A2DP_AVDTP(session);

	/* notify a2dp app the disconnection. */
	if (connect_cb.disconnected != NULL) {
		connect_cb.disconnected(a2dp);
	}
}

static struct net_buf *avdtp_alloc_buf(struct bt_avdtp *session)
{
	return NULL;
}

int a2dp_discovery_ind(struct bt_avdtp *session, uint8_t *errcode)
{
	*errcode = 0;
	return 0;
}

int a2dp_get_capabilities_ind(struct bt_avdtp *session, struct bt_avdtp_seid_lsep *lsep,
		struct net_buf *rsp_buf, uint8_t *errcode)
{
	struct bt_a2dp_endpoint *endpoint;

	*errcode = 0;
	/* Service Category: Media Transport */
	net_buf_add_u8(rsp_buf, BT_AVDTP_SERVICE_MEDIA_TRANSPORT);
	net_buf_add_u8(rsp_buf, 0);
	/* Service Category: Media Codec */
	net_buf_add_u8(rsp_buf, BT_AVDTP_SERVICE_MEDIA_CODEC);
	endpoint = CONTAINER_OF(lsep, struct bt_a2dp_endpoint, info);
	/* Length Of Service Capability */
	net_buf_add_u8(rsp_buf, endpoint->capabilities->len + 2u);
	/* Media Type */
	net_buf_add_u8(rsp_buf, lsep->sep.media_type << 4u);
	/* Media Codec Type */
	net_buf_add_u8(rsp_buf, endpoint->codec_id);
	/* Codec Info Element */
	net_buf_add_mem(rsp_buf, &endpoint->capabilities->codec_ie[0], endpoint->capabilities->len);
	return 0;
}

int a2dp_set_configuration_ind(struct bt_avdtp *session, struct bt_avdtp_seid_lsep *lsep,
		struct net_buf *buf, uint8_t *errcode)
{
	struct bt_a2dp *a2dp = A2DP_AVDTP(session);
	int err;
	uint8_t codec_type;
	uint8_t *codec_info_element;
	uint16_t codec_info_element_len;
	struct bt_a2dp_endpoint *endpoint;
	struct bt_a2dp_codec_sbc_params *sbc_set;
	struct bt_a2dp_codec_sbc_params *sbc;
	struct bt_a2dp_endpoint_internal *ep_internal;

	*errcode = 0;
	/* parse the configuration */
	codec_info_element_len = 4U;
	err = bt_avdtp_parse_capability_codec(buf,
				&codec_type, &codec_info_element,
				&codec_info_element_len);
	endpoint = CONTAINER_OF(lsep, struct bt_a2dp_endpoint, info);
	if (endpoint->codec_id != codec_type) {
		*errcode = BAD_ACP_SEID;
		return 0;
	}

	ep_internal = bt_a2dp_get_endpoint_state(endpoint);
	if (ep_internal == NULL) {
		*errcode = BAD_ACP_SEID;
		return 0;
	}
	ep_internal->a2dp = a2dp;

	/* only support SBC now */
	if (codec_type == BT_A2DP_SBC) {
		struct bt_a2dp_endpoint_configure_result result;

		if (codec_info_element_len != 4U) {
			*errcode = BAD_ACP_SEID;
			return 0;
		}

		sbc_set = (struct bt_a2dp_codec_sbc_params *)codec_info_element;
		sbc = (struct bt_a2dp_codec_sbc_params *)&endpoint->capabilities->codec_ie[0];
		if (((BT_A2DP_SBC_SAMP_FREQ(sbc_set) & BT_A2DP_SBC_SAMP_FREQ(sbc)) == 0) ||
			((BT_A2DP_SBC_CHAN_MODE(sbc_set) & BT_A2DP_SBC_CHAN_MODE(sbc)) == 0) ||
			((BT_A2DP_SBC_BLK_LEN(sbc_set) & BT_A2DP_SBC_BLK_LEN(sbc)) == 0) ||
			((BT_A2DP_SBC_SUB_BAND(sbc_set) & BT_A2DP_SBC_SUB_BAND(sbc)) == 0) ||
			((BT_A2DP_SBC_ALLOC_MTHD(sbc_set) & BT_A2DP_SBC_ALLOC_MTHD(sbc)) == 0)) {
			*errcode = BAD_ACP_SEID;
			return 0;
		}

		/* callback to app */
		ep_internal->config_internal.len = codec_info_element_len;
		memcpy(&ep_internal->config_internal.codec_ie[0], codec_info_element,
			(codec_info_element_len > 4U ? 4U : codec_info_element_len));

		result.config.media_config =
			(struct bt_a2dp_codec_ie *)&ep_internal->config_internal;
		result.err = 0;
		if (endpoint->control_cbs.configured != NULL) {
			endpoint->control_cbs.configured(&result);
		}
	} else {
		*errcode = BAD_ACP_SEID;
		return 0;
	}

	return 0;
}

#if defined(CONFIG_BT_A2DP_SINK)
static void bt_a2dp_media_data_callback(
		struct bt_avdtp_seid_lsep *lsep,
		struct net_buf *buf)
{
	struct bt_avdtp_media_hdr *media_hdr;
	struct bt_a2dp_endpoint *endpoint;

	endpoint = CONTAINER_OF(lsep, struct bt_a2dp_endpoint, info);
	media_hdr = net_buf_pull_mem(buf, sizeof(*media_hdr));

	endpoint->control_cbs.sink_streamer_data(buf,
		sys_get_be16((uint8_t *)&media_hdr->sequence_number),
		sys_get_be32((uint8_t *)&media_hdr->time_stamp));
}
#endif

int a2dp_open_ind(struct bt_avdtp *session, struct bt_avdtp_seid_lsep *lsep, uint8_t *errcode)
{
#if defined(CONFIG_BT_A2DP_SINK)
	if (lsep->sep.tsep == BT_A2DP_SINK) {
		lsep->media_data_cb = bt_a2dp_media_data_callback;
	} else {
		lsep->media_data_cb = NULL;
	}
#else
	lsep->media_data_cb = NULL;
#endif
	*errcode = 0;
	return 0;
}

int a2dp_start_ind(struct bt_avdtp *session, struct bt_avdtp_seid_lsep *lsep, uint8_t *errcode)
{
	struct bt_a2dp_endpoint *endpoint;

	*errcode = 0;
	endpoint = CONTAINER_OF(lsep, struct bt_a2dp_endpoint, info);

	/* callback to app */
	if (endpoint->control_cbs.start_play != NULL) {
		endpoint->control_cbs.start_play(0);
	}

	return 0;
}

int a2dp_close_ind(struct bt_avdtp *session, struct bt_avdtp_seid_lsep *lsep, uint8_t *errcode)
{
	struct bt_a2dp_endpoint *endpoint;

	*errcode = 0;
	endpoint = CONTAINER_OF(lsep, struct bt_a2dp_endpoint, info);

	/* callback to app */
	if (endpoint->control_cbs.stop_play != NULL) {
		endpoint->control_cbs.stop_play(0);
	}
	return 0;
}

int a2dp_suspend_ind(struct bt_avdtp *session, struct bt_avdtp_seid_lsep *lsep, uint8_t *errcode)
{
	struct bt_a2dp_endpoint *endpoint;

	*errcode = 0;
	endpoint = CONTAINER_OF(lsep, struct bt_a2dp_endpoint, info);

	/* callback to app */
	if (endpoint->control_cbs.stop_play != NULL) {
		endpoint->control_cbs.stop_play(0);
	}
	return 0;
}

static void bt_a2dp_auto_configure_callback(struct bt_a2dp *a2dp, uint8_t err)
{
	struct bt_a2dp_endpoint_configure_result config_result;
	struct bt_a2dp_endpoint_internal *ep_internal;

	ep_internal = &endpoint_internals[a2dp->auto_select_endpoint_index];

	/* success */
	if (!err) {
		config_result.err = 0;
		config_result.config.media_config =
			(struct bt_a2dp_codec_ie *)&ep_internal->config_internal;
		ep_internal->a2dp = a2dp;
		a2dp->configure_cb(0);
		ep_internal->endpoint->control_cbs.configured(&config_result);
	} else {
		ep_internal->a2dp = NULL;
		a2dp->configure_cb(err);
	}
}

static int bt_a2dp_open_cb(struct bt_avdtp_req *req)
{
	struct bt_a2dp *a2dp = OPEN_PARAM(OPEN_REQ(req));

	LOG_DBG("OPEN result:%d", a2dp->open_param.status);
	if (!a2dp->open_param.status) {
		if (a2dp->auto_configure_enabled) {
			bt_a2dp_auto_configure_callback(a2dp, 0);
		} else {
			/* todo */
		}
	} else {
		if (a2dp->auto_configure_enabled) {
			bt_a2dp_auto_configure_callback(a2dp, -1);
		} else {
			/* todo */
		}
	}
	return 0;
}

static int bt_a2dp_set_configuration_cb(struct bt_avdtp_req *req)
{
	struct bt_a2dp *a2dp = SET_CONF_PARAM(SET_CONF_REQ(req));
	struct bt_a2dp_endpoint *endpoint;
	struct bt_a2dp_endpoint_internal *ep_internal;

	endpoint = CONTAINER_OF(a2dp->set_configuration_param.lsep, struct bt_a2dp_endpoint, info);
	ep_internal = bt_a2dp_get_endpoint_state(endpoint);

	if (ep_internal == NULL) {
		LOG_DBG("error");
		return -EIO;
	}

	LOG_DBG("SET CONFIGURATION result:%d", a2dp->set_configuration_param.status);
	if (!a2dp->set_configuration_param.status) {
		if (a2dp->auto_configure_enabled) {
			a2dp->open_param.req.func = bt_a2dp_open_cb;
			a2dp->open_param.acp_stream_endpoint_id = ep_internal->remote_ep_info.id;
			a2dp->open_param.lsep = a2dp->set_configuration_param.lsep;
			ep_internal->config_internal.len =
				a2dp->set_configuration_param.codec_ie->len;
			memcpy(&ep_internal->config_internal.codec_ie[0],
				&a2dp->set_configuration_param.codec_ie->codec_ie[0],
				ep_internal->config_internal.len);
			if (bt_avdtp_open(&a2dp->session, &a2dp->open_param) != 0) {
				bt_a2dp_auto_configure_callback(a2dp, -1);
			}
		} else {
			/* todo:*/
		}

	} else {
		if (a2dp->auto_configure_enabled) {
			bt_a2dp_auto_configure_callback(a2dp, -1);
		} else {
			/* todo: */
		}
	}

	return 0;
}

static void bt_a2dp_select_and_configure(struct bt_a2dp *a2dp)
{
	struct bt_a2dp_endpoint *self_endpoint;

	if ((a2dp->auto_select_endpoint_index == 0xFFu) ||
		(a2dp->auto_select_endpoint_index >= CONFIG_BT_A2DP_MAX_ENDPOINT_COUNT)) {
		bt_a2dp_auto_configure_callback(a2dp, -1);
		return;
	}

	self_endpoint =
		endpoint_internals[a2dp->auto_select_endpoint_index].endpoint;
	if (self_endpoint->codec_id == BT_A2DP_SBC) {
		struct bt_a2dp_endpoint_internal *ep_internal;

		if (self_endpoint->config == NULL) {
			bt_a2dp_auto_configure_callback(a2dp, -1);
			return;
		}

		ep_internal = &endpoint_internals[a2dp->auto_select_endpoint_index];

		a2dp->set_configuration_param.req.func = bt_a2dp_set_configuration_cb;
		a2dp->set_configuration_param.acp_stream_endpoint_id =
			ep_internal->remote_ep_info.id;
		a2dp->set_configuration_param.int_stream_endpoint_id =
			self_endpoint->info.sep.id;
		a2dp->set_configuration_param.media_type = BT_A2DP_AUDIO;
		a2dp->set_configuration_param.media_codec_type = BT_A2DP_SBC;
		a2dp->set_configuration_param.codec_ie = self_endpoint->config;
		a2dp->set_configuration_param.lsep = &self_endpoint->info;

		if (bt_avdtp_set_configuration(&a2dp->session,
				&a2dp->set_configuration_param) != 0) {
			bt_a2dp_auto_configure_callback(a2dp, -1);
		}
	} else {
		/* todo: the follow codecs are not supported yet.
		 * BT_A2DP_MPEG1
		 * BT_A2DP_MPEG2
		 * BT_A2DP_ATRAC
		 * BT_A2DP_VENDOR
		 */
		bt_a2dp_auto_configure_callback(a2dp, -1);
	}
}

static int bt_a2dp_get_capabilities_cb(struct bt_avdtp_req *req)
{
	int err;
	uint8_t *codec_info_element;
	struct bt_a2dp *a2dp = GET_CAP_PARAM(GET_CAP_REQ(req));
	uint16_t codec_info_element_len;
	uint8_t codec_type;
	uint8_t index;

	LOG_DBG("GET CAPABILITIES result:%d", a2dp->get_capabilities_param.status);
	if (a2dp->get_capabilities_param.status) {
		if (a2dp->auto_configure_enabled) {
			bt_a2dp_select_and_configure(a2dp);
		} else {
			a2dp->discover_endpoint_cb(a2dp, NULL, 0);
			a2dp->discover_endpoint_cb = NULL;
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

	if (a2dp->auto_configure_enabled) {
		for (index = 0; index < bt_a2dp_get_endpoints_count(); index++) {
			if (endpoint_internals[index].endpoint->codec_id ==
					codec_type) {
				if (index < a2dp->auto_select_endpoint_index) {
					struct bt_a2dp_endpoint_internal *ep_internal =
						&endpoint_internals[index];
					a2dp->auto_select_endpoint_index = index;
					ep_internal->remote_ep_info =
						a2dp->peer_seids[a2dp->get_capabilities_index];
					ep_internal->remote_codec_type = codec_type;

					memcpy(ep_internal->remote_ep_ie.codec_ie,
						codec_info_element, codec_info_element_len);
					ep_internal->remote_ep_ie.len = codec_info_element_len;
				}
			}
		}
		if (bt_a2dp_get_seid_caps(a2dp) != 0) {
			bt_a2dp_select_and_configure(a2dp);
		}
	} else if (a2dp->discover_endpoint_cb != NULL) {
		struct bt_a2dp_endpoint peer_endpoint;
		struct bt_a2dp_codec_ie_internal peer_endpoint_internal_cap;

		peer_endpoint.codec_id = codec_type;
		peer_endpoint.info.sep =
			a2dp->peer_seids[a2dp->get_capabilities_index];
		peer_endpoint.info.next = NULL;
		peer_endpoint.config = NULL;
		memcpy(peer_endpoint_internal_cap.codec_ie,
			codec_info_element, codec_info_element_len);
		peer_endpoint_internal_cap.len = codec_info_element_len;
		peer_endpoint.capabilities =
			(struct bt_a2dp_codec_ie *)&peer_endpoint_internal_cap;
		if (a2dp->discover_endpoint_cb(a2dp, &peer_endpoint, 0) ==
			BT_A2DP_DISCOVER_ENDPOINT_CONTINUE) {
			if (bt_a2dp_get_seid_caps(a2dp) != 0) {
				a2dp->discover_endpoint_cb(a2dp, NULL, 0);
				a2dp->discover_endpoint_cb = NULL;
			}
		}
	}

	return 0;
}

static int bt_a2dp_get_seid_caps(struct bt_a2dp *a2dp)
{
	int err;

	if (a2dp->get_capabilities_index == 0xFFu) {
		a2dp->get_capabilities_index = 0U;
	} else {
		a2dp->get_capabilities_index++;
	}

	for (; a2dp->get_capabilities_index < a2dp->peer_seids_count;
			a2dp->get_capabilities_index++) {
		if ((a2dp->peer_seids[a2dp->get_capabilities_index].inuse == 0) &&
			(a2dp->peer_seids[a2dp->get_capabilities_index].media_type ==
			BT_A2DP_AUDIO)) {
			a2dp->get_capabilities_param.req.func = bt_a2dp_get_capabilities_cb;
			a2dp->get_capabilities_param.buf = NULL;
			a2dp->get_capabilities_param.stream_endpoint_id =
				a2dp->peer_seids[a2dp->get_capabilities_index].id;
			err = bt_avdtp_get_capabilities(&a2dp->session,
				&a2dp->get_capabilities_param);
			if (err) {
				LOG_DBG("AVDTP get capabilities failed");
			}
			return 0;
		}
	}
	return -1;
}

static int bt_a2dp_discover_cb(struct bt_avdtp_req *req)
{
	struct bt_a2dp *a2dp = DISCOVER_PARAM(DISCOVER_REQ(req));
	struct bt_avdtp_seid_info *seid;

	LOG_DBG("DISCOVER result:%d", DISCOVER_REQ(req)->status);
	a2dp->peer_seids_count = 0U;
	if (!(DISCOVER_REQ(req)->status)) {
		seid = bt_avdtp_parse_seids(DISCOVER_REQ(req)->buf);
		while ((seid != NULL) &&
			(a2dp->peer_seids_count < CONFIG_BT_A2DP_MAX_ENDPOINT_COUNT)) {
			LOG_DBG("id:%d, inuse:%d, media_type:%d, tsep:%d, ",
				seid->id,
				seid->inuse,
				seid->media_type,
				seid->tsep);
			a2dp->peer_seids[a2dp->peer_seids_count] = *seid;
			a2dp->peer_seids_count++;
			seid = bt_avdtp_parse_seids(DISCOVER_REQ(req)->buf);
		}

		/* trigger the getting capability */
		a2dp->get_capabilities_index = 0xFFu;
		if (bt_a2dp_get_seid_caps(a2dp) != 0) {
			/* the peer_seids' caps is done.*/
			if (a2dp->auto_configure_enabled) {
				bt_a2dp_select_and_configure(a2dp);
			} else if (a2dp->discover_endpoint_cb != NULL) {
				a2dp->discover_endpoint_cb(a2dp, NULL, 0);
			}
		}
	} else {
		if (a2dp->auto_configure_enabled) {
			bt_a2dp_select_and_configure(a2dp);
		} else if (a2dp->discover_endpoint_cb != NULL) {
			a2dp->discover_endpoint_cb(a2dp, NULL, -1);
		}
	}

	return 0;
}

int bt_a2dp_configure(struct bt_a2dp *a2dp, void (*result_cb)(int err))
{
	int err;

	if (a2dp == NULL) {
		return -EINVAL;
	}

	if (a2dp->a2dp_state != INTERNAL_STATE_AVDTP_CONNECTED) {
		return -EIO;
	}

	a2dp->auto_select_endpoint_index = 0xFFu;
	a2dp->auto_configure_enabled = 1U;
	a2dp->configure_cb = result_cb;

	a2dp->discover_param.req.func = bt_a2dp_discover_cb;
	a2dp->discover_param.buf = NULL;
	err = bt_avdtp_discover(&a2dp->session, &a2dp->discover_param);
	if (err) {
		LOG_DBG("AVDTP discover failed");
	}
	return 0;
}

int bt_a2dp_discover_peer_endpoints(struct bt_a2dp *a2dp, bt_a2dp_discover_peer_endpoint_cb_t cb)
{
	int err;

	if (a2dp == NULL) {
		return -EINVAL;
	}

	a2dp->auto_configure_enabled = 0U;
	a2dp->discover_endpoint_cb = cb;
	a2dp->discover_param.req.func = bt_a2dp_discover_cb;
	a2dp->discover_param.buf = NULL;
	err = bt_avdtp_discover(&a2dp->session, &a2dp->discover_param);
	if (err) {
		LOG_DBG("AVDTP discover failed");
	}

	return err;
}

int bt_a2dp_configure_endpoint(struct bt_a2dp *a2dp, struct bt_a2dp_endpoint *endpoint,
		struct bt_a2dp_endpoint *peer_endpoint,
		struct bt_a2dp_endpoint_config *config)
{
	/* todo: see the API description in a2dp.h */
	return 0;
}

int bt_a2dp_deconfigure(struct bt_a2dp_endpoint *endpoint)
{
	/* todo: see the API description in a2dp.h */
	return 0;
}

static int bt_a2dp_start_cb(struct bt_avdtp_req *req)
{
	struct bt_a2dp *a2dp = START_PARAM(START_REQ(req));
	struct bt_a2dp_endpoint *endpoint;
	struct bt_a2dp_endpoint_internal *ep_internal;

	endpoint = CONTAINER_OF(a2dp->start_param.lsep, struct bt_a2dp_endpoint, info);
	ep_internal = bt_a2dp_get_endpoint_state(endpoint);
	if (ep_internal == NULL) {
		return -EPERM;
	}

	LOG_DBG("START result:%d", a2dp->start_param.status);
	if (!a2dp->start_param.status) {
		if (endpoint->control_cbs.start_play != NULL) {
			endpoint->control_cbs.start_play(0);
		}
	} else {
		if (endpoint->control_cbs.start_play != NULL) {
			endpoint->control_cbs.start_play(a2dp->start_param.status);
		}
	}
	return 0;
}

int bt_a2dp_start(struct bt_a2dp_endpoint *endpoint)
{
	struct bt_a2dp *a2dp;
	struct bt_a2dp_endpoint_internal *ep_internal;

	if (endpoint == NULL) {
		return -EPERM;
	}

	ep_internal = bt_a2dp_get_endpoint_state(endpoint);
	if (ep_internal == NULL) {
		return -EPERM;
	}

	a2dp = ep_internal->a2dp;
	if (a2dp == NULL) {
		return -EPERM;
	}

	a2dp->start_param.req.func = bt_a2dp_start_cb;
	a2dp->start_param.acp_stream_endpoint_id = ep_internal->remote_ep_info.id;
	a2dp->start_param.lsep = &endpoint->info;
	if (bt_avdtp_start(&a2dp->session, &a2dp->start_param) != 0) {
		if (endpoint->control_cbs.start_play != NULL) {
			endpoint->control_cbs.start_play(-1);
		}
		return -EIO;
	}
	return 0;
}

int bt_a2dp_stop(struct bt_a2dp_endpoint *endpoint)
{
	/* todo: see the API description in a2dp.h */
	return 0;
}

int bt_a2dp_reconfigure(struct bt_a2dp_endpoint *endpoint,
		struct bt_a2dp_endpoint_config *config)
{
	/* todo: see the API description in a2dp.h */
	return 0;
}

uint32_t bt_a2dp_get_media_mtu(struct bt_a2dp_endpoint *endpoint)
{
	struct bt_a2dp_endpoint_internal *ep_internal;

	if (endpoint == NULL) {
		return -EPERM;
	}

	ep_internal = bt_a2dp_get_endpoint_state(endpoint);
	return bt_avdtp_get_media_mtu(&ep_internal->endpoint->info);
}

#if defined(CONFIG_BT_A2DP_SOURCE)
struct net_buf *bt_a2dp_media_buf_alloc(struct net_buf_pool *pool)
{
	struct net_buf *buf;
	uint8_t *send_buf_header;

	buf = bt_l2cap_create_pdu(pool, 0);
	if (buf == NULL) {
		LOG_DBG("bt_l2cap_create_pdu fail");
		return buf;
	}

	send_buf_header = net_buf_add(buf, sizeof(struct bt_avdtp_media_hdr));
	memset(send_buf_header, 0, sizeof(struct bt_avdtp_media_hdr));
	((struct bt_avdtp_media_hdr *)(send_buf_header))->playload_type = 0x60U;
	((struct bt_avdtp_media_hdr *)(send_buf_header))->synchronization_source = 0U;
	return buf;
}

int bt_a2dp_media_send(struct bt_a2dp_endpoint *endpoint, struct net_buf *buf,
			uint16_t seq_num, uint32_t ts)
{
	uint8_t *send_buf_header = buf->data;
	struct bt_a2dp_endpoint_internal *ep_internal;

	if (endpoint == NULL) {
		return -EPERM;
	}

	ep_internal = bt_a2dp_get_endpoint_state(endpoint);
	if (ep_internal == NULL) {
		return -EIO;
	}

	/* update time_stamp in the buf */
	sys_put_be32(ts,
		(uint8_t *)&((struct bt_avdtp_media_hdr *)
		(send_buf_header))->time_stamp);
	/* update sequence_number in the buf */
	sys_put_be16(seq_num,
		(uint8_t *)&((struct bt_avdtp_media_hdr *)
		(send_buf_header))->sequence_number);
	/* send the buf */
	return bt_avdtp_send_media_data(&ep_internal->endpoint->info, buf);
}
#endif

static const struct bt_avdtp_ops_cb signaling_avdtp_ops = {
	.connected = avdtp_connected,
	.disconnected = avdtp_disconnected,
	.alloc_buf = avdtp_alloc_buf,
	.discovery_ind = a2dp_discovery_ind,
	.get_capabilities_ind = a2dp_get_capabilities_ind,
	.set_configuration_ind = a2dp_set_configuration_ind,
	.open_ind = a2dp_open_ind,
	.start_ind = a2dp_start_ind,
	.close_ind = a2dp_close_ind,
	.suspend_ind = a2dp_suspend_ind,
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

	memset(endpoint_internals, 0, sizeof(endpoint_internals));
	LOG_DBG("A2DP Initialized successfully.");
	return 0;
}

struct bt_a2dp *bt_a2dp_connect(struct bt_conn *conn)
{
	struct bt_a2dp *a2dp;
	int err;

	a2dp = get_new_connection(conn);
	if (!a2dp) {
		LOG_ERR("Cannot allocate memory");
		return NULL;
	}

	a2dp->a2dp_state = INTERNAL_STATE_IDLE;

	a2dp->session.ops = &signaling_avdtp_ops;
	err = bt_avdtp_connect(conn, &(a2dp->session));
	if (err < 0) {
		/* If error occurs, undo the saving and return the error */
		a2dp_reset(a2dp);
		LOG_DBG("AVDTP Connect failed");
		return NULL;
	}

	LOG_DBG("Connect request sent");
	return a2dp;
}

int bt_a2dp_disconnect(struct bt_a2dp *a2dp)
{
	/* todo: */
	return 0;
}

int bt_a2dp_register_endpoint(struct bt_a2dp_endpoint *endpoint,
				  uint8_t media_type, uint8_t role)
{
	int err;
	uint8_t index;

	BT_ASSERT(endpoint);

	err = bt_avdtp_register_sep(media_type, role, &(endpoint->info));
	if (err < 0) {
		return err;
	}

	for (index = 0; index < CONFIG_BT_A2DP_MAX_ENDPOINT_COUNT; index++) {
		if (endpoint_internals[index].endpoint == NULL) {
			endpoint_internals[index].endpoint = endpoint;
			break;
		}
	}
	return 0;
}

int bt_a2dp_register_connect_callback(struct bt_a2dp_connect_cb *cb)
{
	connect_cb = *cb;
	return 0;
}
