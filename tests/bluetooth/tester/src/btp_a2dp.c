/* btp_a2dp.c - Bluetooth A2DP Tester */

/*
 * Copyright (c) 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/a2dp.h>
#include <zephyr/bluetooth/classic/a2dp_codec_sbc.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/sys/util.h>

#include "btp/btp.h"

#define BTP_A2DP_MAX_ENDPOINTS 8
#define BTP_A2DP_MAX_STREAMS   4
struct a2dp_stream_info {
	struct bt_a2dp_stream stream;
	uint8_t stream_id;
	bool in_use;
	uint8_t sep_type;
};

struct a2dp_connection {
	struct bt_conn *acl_conn;
	struct bt_a2dp *a2dp;
	bt_addr_t address;
	struct a2dp_stream_info streams[BTP_A2DP_MAX_STREAMS];
	bool in_use;
};

struct a2dp_endpoint_info {
	struct bt_a2dp_ep ep;
	struct bt_a2dp_codec_ie codec_ie;
	uint8_t ep_id;
	bool registered;
};

static struct a2dp_connection a2dp_connections[CONFIG_BT_MAX_CONN];
static struct a2dp_endpoint_info registered_endpoints[BTP_A2DP_MAX_ENDPOINTS];

static uint8_t role;
static const struct bt_uuid *a2dp_snk_uuid = BT_UUID_DECLARE_16(BT_SDP_AUDIO_SINK_SVCLASS);
static const struct bt_uuid *a2dp_src_uuid = BT_UUID_DECLARE_16(BT_SDP_AUDIO_SOURCE_SVCLASS);
NET_BUF_POOL_DEFINE(a2dp_tx_pool, CONFIG_BT_MAX_CONN, BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
		    CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

NET_BUF_POOL_FIXED_DEFINE(find_avdtp_version_pool, CONFIG_BT_MAX_CONN, 512,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

#if defined(CONFIG_BT_A2DP_SOURCE)
static uint8_t media_data[] = {
	0x9C, 0xFD, 0x21, 0xB4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6A, 0xAA,
	0xAA, 0xAA, 0xB5, 0x55, 0x55, 0x55, 0x5A, 0xAA, 0xAA, 0xAA, 0xAD, 0x55, 0x55, 0x55, 0x56,
	0xAA, 0xAA, 0xAA, 0xAB, 0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA, 0xD5, 0x55, 0x55,
	0x55, 0x6A, 0xAA, 0xAA, 0xAA, 0xB5, 0x55, 0x55, 0x55, 0x5A, 0xAA, 0xAA, 0xAA, 0xAD, 0x55,
	0x55, 0x55, 0x56, 0xAA, 0xAA, 0xAA, 0xAB, 0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAB,
	0x15, 0x55, 0x15, 0x55, 0x9C, 0xFD, 0x21, 0x39, 0xE2, 0x41, 0x00, 0x00, 0x00, 0x31, 0x00,
	0x00, 0x00, 0x78, 0xAD, 0x48, 0xCF, 0x3A, 0x6A, 0x2B, 0x87, 0xDF, 0x95, 0xAF, 0x84, 0x10,
	0x72, 0x37, 0x45, 0x87, 0xF5, 0x03, 0xED, 0x2B, 0xDA, 0x75, 0x8C, 0x29, 0xF8, 0x41, 0x17,
	0x26, 0xD7, 0xD0, 0xB3, 0xE5, 0x79, 0x8E, 0x58, 0x2B, 0xD0, 0x18, 0x0B, 0x27, 0x30, 0x75,
	0xE8, 0x5D, 0x70, 0xE4, 0xD6, 0x29, 0x37, 0xEE, 0xA8, 0x0F, 0xBD, 0x9B, 0xC5, 0x6F, 0x31,
	0xFD, 0xC5, 0x73, 0xCB, 0x08, 0xA6, 0x3F, 0x0F};

static struct k_work_delayable send_media_work;
static struct bt_a2dp_stream *active_stream;
#endif
#define A2DP_VERSION 0x0104U

#if defined(CONFIG_BT_A2DP_SINK)
static struct bt_sdp_attribute a2dp_sink_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_AUDIO_SINK_SVCLASS)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 16),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(BT_UUID_AVDTP_VAL)
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_UUID_AVDTP_VAL)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(AVDTP_VERSION)
			},
			)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_ADVANCED_AUDIO_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(A2DP_VERSION)
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("A2DPSink"),
	BT_SDP_SUPPORTED_FEATURES(0x0001U),
};

static struct bt_sdp_record a2dp_sink_rec = BT_SDP_RECORD(a2dp_sink_attrs);
#endif

#if defined(CONFIG_BT_A2DP_SOURCE)
static struct bt_sdp_attribute a2dp_source_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_AUDIO_SOURCE_SVCLASS)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 16),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(BT_UUID_AVDTP_VAL)
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_UUID_AVDTP_VAL)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(AVDTP_VERSION)
			},
			)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_ADVANCED_AUDIO_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(A2DP_VERSION)
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("A2DPSource"),
	BT_SDP_SUPPORTED_FEATURES(0x0001U),
};

static struct bt_sdp_record a2dp_source_rec = BT_SDP_RECORD(a2dp_source_attrs);
#endif
static struct a2dp_connection *find_connection_by_address(const bt_addr_t *address)
{
	for (size_t i = 0; i < ARRAY_SIZE(a2dp_connections); i++) {
		if (a2dp_connections[i].in_use &&
		    bt_addr_eq(&a2dp_connections[i].address, address)) {
			return &a2dp_connections[i];
		}
	}
	return NULL;
}

static struct a2dp_connection *find_connection_by_a2dp(struct bt_a2dp *a2dp)
{
	for (size_t i = 0; i < ARRAY_SIZE(a2dp_connections); i++) {
		if (a2dp_connections[i].in_use && a2dp_connections[i].a2dp == a2dp) {
			return &a2dp_connections[i];
		}
	}
	return NULL;
}

static struct a2dp_connection *find_connection_by_stream(struct bt_a2dp_stream *stream)
{
	for (size_t i = 0; i < ARRAY_SIZE(a2dp_connections); i++) {
		if (!a2dp_connections[i].in_use) {
			continue;
		}
		for (size_t j = 0; j < ARRAY_SIZE(a2dp_connections[i].streams); j++) {
			if (a2dp_connections[i].streams[j].in_use &&
			    &a2dp_connections[i].streams[j].stream == stream) {
				return &a2dp_connections[i];
			}
		}
	}
	return NULL;
}

static struct a2dp_connection *alloc_connection(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(a2dp_connections); i++) {
		if (!a2dp_connections[i].in_use) {
			memset(&a2dp_connections[i], 0, sizeof(a2dp_connections[i]));
			a2dp_connections[i].in_use = true;
			return &a2dp_connections[i];
		}
	}
	return NULL;
}

static void free_connection(struct a2dp_connection *conn)
{
	if (conn != NULL) {
		if (conn->acl_conn) {
			bt_conn_unref(conn->acl_conn);
		}
		memset(conn, 0, sizeof(*conn));
	}
}

static uint8_t add_stream(struct a2dp_connection *conn, struct bt_a2dp_stream **stream)
{
	if (conn == NULL) {
		return 0xFF;
	}

	for (size_t i = 0; i < ARRAY_SIZE(conn->streams); i++) {
		if (!conn->streams[i].in_use) {
			*stream = &conn->streams[i].stream;
			conn->streams[i].stream_id = i;
			conn->streams[i].in_use = true;
			return i;
		}
	}
	return 0xFF;
}

static void remove_stream(struct a2dp_connection *conn, struct bt_a2dp_stream *stream)
{
	if (conn == NULL) {
		return;
	}

	for (size_t i = 0; i < ARRAY_SIZE(conn->streams); i++) {
		if (conn->streams[i].in_use && &conn->streams[i].stream == stream) {
			conn->streams[i].in_use = false;
		}
	}
}

static uint8_t get_stream_id(struct a2dp_connection *conn, struct bt_a2dp_stream *stream)
{
	if (conn == NULL) {
		return 0xFF;
	}

	for (size_t i = 0; i < ARRAY_SIZE(conn->streams); i++) {
		if (conn->streams[i].in_use && &conn->streams[i].stream == stream) {
			return conn->streams[i].stream_id;
		}
	}
	return 0xFF;
}

static struct bt_a2dp_stream *get_stream_by_id(struct a2dp_connection *conn, uint8_t stream_id)
{
	if (!conn || stream_id >= ARRAY_SIZE(conn->streams)) {
		return NULL;
	}

	if (conn->streams[stream_id].in_use) {
		return &conn->streams[stream_id].stream;
	}
	return NULL;
}

static struct a2dp_endpoint_info *find_endpoint_by_id(uint8_t ep_id)
{
	for (size_t i = 0; i < ARRAY_SIZE(registered_endpoints); i++) {
		if (registered_endpoints[i].registered && registered_endpoints[i].ep_id == ep_id) {
			return &registered_endpoints[i];
		}
	}
	return NULL;
}

static struct a2dp_endpoint_info *alloc_endpoint(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(registered_endpoints); i++) {
		if (!registered_endpoints[i].registered) {
			memset(&registered_endpoints[i], 0, sizeof(registered_endpoints[i]));
			registered_endpoints[i].ep_id = i + 1;
			registered_endpoints[i].registered = true;
			return &registered_endpoints[i];
		}
	}
	return NULL;
}

#if defined(CONFIG_BT_A2DP_SOURCE)
static void a2dp_send_media_timeout(struct k_work *work)
{
	struct net_buf *buf;
	int ret;

	if (active_stream == NULL) {
		return;
	}

	buf = bt_a2dp_stream_create_pdu(&a2dp_tx_pool, K_FOREVER);
	if (buf == NULL) {
		return;
	}

	net_buf_add_u8(buf, (uint8_t)BT_A2DP_SBC_MEDIA_HDR_ENCODE(2, 0, 0, 0));
	net_buf_add_mem(buf, media_data, sizeof(media_data));

	ret = bt_a2dp_stream_send(active_stream, buf, 0U, 0U);
	if (ret < 0) {
		net_buf_unref(buf);
		k_work_schedule(&send_media_work, K_MSEC(1000));
		return;
	}

	k_work_schedule(&send_media_work, K_MSEC(1000));
}
#endif

static void stream_released(struct bt_a2dp_stream *stream)
{
	struct a2dp_connection *conn;

	conn = find_connection_by_stream(stream);
	if (conn == NULL) {
		return;
	}

#if defined(CONFIG_BT_A2DP_SOURCE)
	if (stream->local_ep && stream->local_ep->sep.sep_info.tsep == BT_AVDTP_SOURCE) {
		k_work_cancel_delayable(&send_media_work);
		if (active_stream == stream) {
			active_stream = NULL;
		}
	}
#endif

	remove_stream(conn, stream);
}

static void stream_started(struct bt_a2dp_stream *stream)
{
#if defined(CONFIG_BT_A2DP_SOURCE)
	if (stream->local_ep && stream->local_ep->sep.sep_info.tsep == BT_AVDTP_SOURCE) {
		active_stream = stream;
		k_work_schedule(&send_media_work, K_MSEC(1000));
	}
#endif
}

static void stream_suspended(struct bt_a2dp_stream *stream)
{
#if defined(CONFIG_BT_A2DP_SOURCE)
	if (stream->local_ep && stream->local_ep->sep.sep_info.tsep == BT_AVDTP_SOURCE) {
		k_work_cancel_delayable(&send_media_work);
		if (active_stream == stream) {
			active_stream = NULL;
		}
	}
#endif
}

#if defined(CONFIG_BT_A2DP_SINK)
static void stream_recv(struct bt_a2dp_stream *stream, struct net_buf *buf, uint16_t seq_num,
			uint32_t ts)
{
	struct btp_a2dp_stream_recv_ev *ev;
	uint8_t *event_buf;
	struct a2dp_connection *conn;
	uint8_t stream_id;
	uint16_t data_len;
	int err;

	conn = find_connection_by_stream(stream);
	if (conn == NULL) {
		return;
	}

	stream_id = get_stream_id(conn, stream);
	if (stream_id == 0xFF) {
		return;
	}

	data_len = buf->len;

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		return;
	}

	tester_rsp_buffer_allocate(sizeof(*ev) + data_len, &event_buf);
	ev = (struct btp_a2dp_stream_recv_ev *)event_buf;

	bt_addr_copy(&ev->address.a, &conn->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->stream_id = stream_id;
	ev->seq_num = sys_cpu_to_le16(seq_num);
	ev->timestamp = sys_cpu_to_le32(ts);
	ev->data_len = sys_cpu_to_le16(data_len);
	memcpy(ev->data, buf->data, data_len);

	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_STREAM_RECV, ev, sizeof(*ev) + data_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}
#endif

#if defined(CONFIG_BT_A2DP_SOURCE)
static void stream_sent(struct bt_a2dp_stream *stream)
{
	struct btp_a2dp_stream_sent_ev ev;
	struct a2dp_connection *conn;
	uint8_t stream_id;

	conn = find_connection_by_stream(stream);
	if (conn == NULL) {
		return;
	}

	stream_id = get_stream_id(conn, stream);
	if (stream_id == 0xFF) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;
	ev.stream_id = stream_id;
	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_STREAM_SENT, &ev, sizeof(ev));
}

#endif

static struct bt_a2dp_stream_ops stream_ops = {
	.released = stream_released,
	.started = stream_started,
	.suspended = stream_suspended,
#if defined(CONFIG_BT_A2DP_SINK)
	.recv = stream_recv,
#endif
#if defined(CONFIG_BT_A2DP_SOURCE)
	.sent = stream_sent,
#endif
};

static void a2dp_connected(struct bt_a2dp *a2dp, int err)
{
	struct btp_a2dp_connected_ev ev;
	struct a2dp_connection *conn;
	struct bt_conn *acl_conn;
	bt_addr_t addr;

	acl_conn = bt_a2dp_get_conn(a2dp);
	if (acl_conn == NULL) {
		return;
	}

	bt_addr_copy(&addr, bt_conn_get_dst_br(acl_conn));

	conn = find_connection_by_address(&addr);
	if (conn == NULL) {
		conn = alloc_connection();
		if (conn == NULL) {

			bt_conn_unref(acl_conn);
			return;
		}
		conn->acl_conn = acl_conn;
	} else {
		bt_conn_unref(acl_conn);
	}

	conn->a2dp = a2dp;
	bt_addr_copy(&conn->address, &addr);

	bt_addr_copy(&ev.address.a, &addr);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.result = (int8_t)err;
	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_CONNECTED, &ev, sizeof(ev));
}

static void a2dp_disconnected(struct bt_a2dp *a2dp)
{
	struct btp_a2dp_disconnected_ev ev;
	struct a2dp_connection *conn;

	conn = find_connection_by_a2dp(a2dp);
	if (conn == NULL) {

		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;
	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_DISCONNECTED, &ev, sizeof(ev));

	free_connection(conn);
}

static int a2dp_config_req(struct bt_a2dp *a2dp, struct bt_a2dp_ep *ep,
			   struct bt_a2dp_codec_cfg *codec_cfg, struct bt_a2dp_stream **stream,
			   uint8_t *rsp_err_code)
{
	struct btp_a2dp_config_req_ev *ev;
	uint8_t *buf;
	struct a2dp_connection *conn;
	int err;
	struct bt_a2dp_stream *sbc_stream = NULL;
	uint8_t stream_id;

	conn = find_connection_by_a2dp(a2dp);
	if (conn == NULL) {
		*rsp_err_code = BT_AVDTP_BAD_STATE;
		return -EINVAL;
	}

	stream_id = add_stream(conn, &sbc_stream);

	if (sbc_stream == NULL || stream_id == 0xFF) {
		*rsp_err_code = BT_AVDTP_BAD_STATE;
	}

	bt_a2dp_stream_cb_register(sbc_stream, &stream_ops);
	*stream = sbc_stream;

	*rsp_err_code = BT_AVDTP_SUCCESS;

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		*rsp_err_code = BT_AVDTP_BAD_STATE;
		return -ENOMEM;
	}

	tester_rsp_buffer_allocate(sizeof(*ev) + codec_cfg->codec_config->len, &buf);
	ev = (struct btp_a2dp_config_req_ev *)buf;

	bt_addr_copy(&ev->address.a, &conn->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->stream_id = stream_id;
	ev->delay_report = codec_cfg->delay_report ? 1 : 0;
	ev->codec_ie_len = codec_cfg->codec_config->len;
	ev->result = *rsp_err_code;
	memcpy(ev->codec_ie, codec_cfg->codec_config->codec_ie, codec_cfg->codec_config->len);

	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_CONFIG_REQ, ev,
		     sizeof(*ev) + codec_cfg->codec_config->len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();

	return 0;
}

static int a2dp_reconfig_req(struct bt_a2dp_stream *stream, struct bt_a2dp_codec_cfg *codec_cfg,
			     uint8_t *rsp_err_code)
{
	struct btp_a2dp_reconfig_req_ev *ev;
	uint8_t *buf;
	struct a2dp_connection *conn;
	uint8_t stream_id;
	int err;

	conn = find_connection_by_stream(stream);
	if (conn == NULL) {
		*rsp_err_code = BT_AVDTP_BAD_STATE;
		return -EINVAL;
	}

	stream_id = get_stream_id(conn, stream);
	if (stream_id == 0xFF) {
		*rsp_err_code = BT_AVDTP_BAD_STATE;
		return -EINVAL;
	}

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		*rsp_err_code = BT_AVDTP_BAD_STATE;
		return -ENOMEM;
	}

	*rsp_err_code = BT_AVDTP_SUCCESS;

	tester_rsp_buffer_allocate(sizeof(*ev) + codec_cfg->codec_config->len, &buf);
	ev = (struct btp_a2dp_reconfig_req_ev *)buf;

	bt_addr_copy(&ev->address.a, &conn->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->result = *rsp_err_code;
	ev->stream_id = stream_id;
	ev->delay_report = codec_cfg->delay_report ? 1 : 0;
	ev->codec_ie_len = codec_cfg->codec_config->len;
	memcpy(ev->codec_ie, codec_cfg->codec_config->codec_ie, codec_cfg->codec_config->len);

	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_RECONFIG_REQ, ev,
		     sizeof(*ev) + codec_cfg->codec_config->len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();

	return 0;
}

static void a2dp_config_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
	struct btp_a2dp_config_rsp_ev ev;
	struct a2dp_connection *conn;
	uint8_t stream_id;

	conn = find_connection_by_stream(stream);
	if (conn == NULL) {
		return;
	}

	stream_id = get_stream_id(conn, stream);
	if (stream_id == 0xFF) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;
	ev.stream_id = stream_id;
	ev.rsp_err_code = rsp_err_code;
	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_CONFIG_RSP, &ev, sizeof(ev));
}

static int a2dp_establish_req(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code)
{
	struct btp_a2dp_establish_req_ev ev;
	struct a2dp_connection *conn;
	uint8_t stream_id;

	conn = find_connection_by_stream(stream);
	if (conn == NULL) {
		*rsp_err_code = BT_AVDTP_BAD_STATE;
		return -EINVAL;
	}

	stream_id = get_stream_id(conn, stream);
	if (stream_id == 0xFF) {
		*rsp_err_code = BT_AVDTP_BAD_STATE;
		return -EINVAL;
	}
	*rsp_err_code = BT_AVDTP_SUCCESS;
	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;
	ev.result = *rsp_err_code;
	ev.stream_id = stream_id;
	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_ESTABLISH_REQ, &ev, sizeof(ev));

	return 0;
}

static void a2dp_establish_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
	struct btp_a2dp_establish_rsp_ev ev;
	struct a2dp_connection *conn;
	uint8_t stream_id;

	conn = find_connection_by_stream(stream);
	if (conn == NULL) {
		return;
	}

	stream_id = get_stream_id(conn, stream);
	if (stream_id == 0xFF) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;
	ev.stream_id = stream_id;
	ev.rsp_err_code = rsp_err_code;
	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_ESTABLISH_RSP, &ev, sizeof(ev));
}

static int a2dp_release_req(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code)
{
	struct btp_a2dp_release_req_ev ev;
	struct a2dp_connection *conn;
	uint8_t stream_id;

	conn = find_connection_by_stream(stream);
	if (conn == NULL) {
		*rsp_err_code = BT_AVDTP_BAD_STATE;
		return -EINVAL;
	}

	stream_id = get_stream_id(conn, stream);
	if (stream_id == 0xFF) {
		*rsp_err_code = BT_AVDTP_BAD_STATE;
		return -EINVAL;
	}
	*rsp_err_code = BT_AVDTP_SUCCESS;
	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;
	ev.result = *rsp_err_code;
	ev.stream_id = stream_id;
	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_RELEASE_REQ, &ev, sizeof(ev));

	return 0;
}

static void a2dp_release_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
	struct btp_a2dp_release_rsp_ev ev;
	struct a2dp_connection *conn;
	uint8_t stream_id;

	conn = find_connection_by_stream(stream);
	if (conn == NULL) {
		return;
	}

	stream_id = get_stream_id(conn, stream);
	if (stream_id == 0xFF) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;
	ev.stream_id = stream_id;
	ev.rsp_err_code = rsp_err_code;
	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_RELEASE_RSP, &ev, sizeof(ev));
}

static int a2dp_start_req(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code)
{
	struct btp_a2dp_start_req_ev ev;
	struct a2dp_connection *conn;
	uint8_t stream_id;

	conn = find_connection_by_stream(stream);
	if (conn == NULL) {
		*rsp_err_code = BT_AVDTP_BAD_STATE;
		return -EINVAL;
	}

	stream_id = get_stream_id(conn, stream);
	if (stream_id == 0xFF) {
		*rsp_err_code = BT_AVDTP_BAD_STATE;
		return -EINVAL;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	*rsp_err_code = BT_AVDTP_SUCCESS;
	ev.address.type = BTP_BR_ADDRESS_TYPE;
	ev.result = *rsp_err_code;
	ev.stream_id = stream_id;
	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_START_REQ, &ev, sizeof(ev));

	return 0;
}

static void a2dp_start_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
	struct btp_a2dp_start_rsp_ev ev;
	struct a2dp_connection *conn;
	uint8_t stream_id;

	conn = find_connection_by_stream(stream);

	if (conn == NULL) {

		return;
	}

	stream_id = get_stream_id(conn, stream);
	if (stream_id == 0xFF) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;
	ev.stream_id = stream_id;
	ev.rsp_err_code = rsp_err_code;
	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_START_RSP, &ev, sizeof(ev));
}

static int a2dp_suspend_req(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code)
{
	struct btp_a2dp_suspend_req_ev ev;
	struct a2dp_connection *conn;
	uint8_t stream_id;

	conn = find_connection_by_stream(stream);
	if (conn == NULL) {
		*rsp_err_code = BT_AVDTP_BAD_STATE;
		return -EINVAL;
	}

	stream_id = get_stream_id(conn, stream);
	if (stream_id == 0xFF) {
		*rsp_err_code = BT_AVDTP_BAD_STATE;
		return -EINVAL;
	}
	*rsp_err_code = BT_AVDTP_SUCCESS;
	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;
	ev.result = *rsp_err_code;
	ev.stream_id = stream_id;
	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_SUSPEND_REQ, &ev, sizeof(ev));

	return 0;
}

static void a2dp_suspend_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
	struct btp_a2dp_suspend_rsp_ev ev;
	struct a2dp_connection *conn;
	uint8_t stream_id;

	conn = find_connection_by_stream(stream);
	if (conn == NULL) {
		return;
	}

	stream_id = get_stream_id(conn, stream);
	if (stream_id == 0xFF) {

		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;
	ev.stream_id = stream_id;
	ev.rsp_err_code = rsp_err_code;
	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_SUSPEND_RSP, &ev, sizeof(ev));
}

static int a2dp_abort_req(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code)
{
	struct btp_a2dp_abort_req_ev ev;
	struct a2dp_connection *conn;
	uint8_t stream_id;

	conn = find_connection_by_stream(stream);
	if (conn == NULL) {

		*rsp_err_code = BT_AVDTP_BAD_STATE;
		return -EINVAL;
	}

	stream_id = get_stream_id(conn, stream);
	if (stream_id == 0xFF) {

		*rsp_err_code = BT_AVDTP_BAD_STATE;
		return -EINVAL;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	*rsp_err_code = BT_AVDTP_SUCCESS;
	ev.address.type = BTP_BR_ADDRESS_TYPE;
	ev.result = *rsp_err_code;
	ev.stream_id = stream_id;
	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_ABORT_REQ, &ev, sizeof(ev));

	return 0;
}

static void a2dp_abort_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
	struct btp_a2dp_abort_rsp_ev ev;
	struct a2dp_connection *conn;
	uint8_t stream_id;

	conn = find_connection_by_stream(stream);
	if (conn == NULL) {
		return;
	}

	stream_id = get_stream_id(conn, stream);
	if (stream_id == 0xFF) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;
	ev.stream_id = stream_id;
	ev.rsp_err_code = rsp_err_code;
	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_ABORT_RSP, &ev, sizeof(ev));
}

static int a2dp_get_config_req(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code)
{
	struct btp_a2dp_get_config_req_ev ev;
	struct a2dp_connection *conn;
	uint8_t stream_id;

	conn = find_connection_by_stream(stream);
	if (conn == NULL) {
		*rsp_err_code = BT_AVDTP_BAD_STATE;
		return -EINVAL;
	}

	stream_id = get_stream_id(conn, stream);
	if (stream_id == 0xFF) {
		*rsp_err_code = BT_AVDTP_BAD_STATE;
		return -EINVAL;
	}
	*rsp_err_code = BT_AVDTP_SUCCESS;
	bt_addr_copy(&ev.address.a, &conn->address);
	ev.result = *rsp_err_code;
	ev.address.type = BTP_BR_ADDRESS_TYPE;
	ev.stream_id = stream_id;
	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_GET_CONFIG_REQ, &ev, sizeof(ev));

	return 0;
}

static void a2dp_get_config_rsp(struct bt_a2dp_stream *stream, struct bt_a2dp_codec_cfg *codec_cfg,
				uint8_t rsp_err_code)
{
	struct btp_a2dp_get_config_rsp_ev *ev;
	uint8_t *buf;
	struct a2dp_connection *conn;
	uint8_t stream_id;
	int err;

	conn = find_connection_by_stream(stream);
	if (conn == NULL) {
		return;
	}

	stream_id = get_stream_id(conn, stream);
	if (stream_id == 0xFF) {
		return;
	}

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		return;
	}

	tester_rsp_buffer_allocate(sizeof(*ev) + (codec_cfg ? codec_cfg->codec_config->len : 0),
				   &buf);
	ev = (struct btp_a2dp_get_config_rsp_ev *)buf;

	bt_addr_copy(&ev->address.a, &conn->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->stream_id = stream_id;
	ev->rsp_err_code = rsp_err_code;

	if (codec_cfg && rsp_err_code == BT_AVDTP_SUCCESS) {
		ev->delay_report = codec_cfg->delay_report ? 1 : 0;
		ev->codec_ie_len = codec_cfg->codec_config->len;
		memcpy(ev->codec_ie, codec_cfg->codec_config->codec_ie,
		       codec_cfg->codec_config->len);
	} else {
		ev->delay_report = 0;
		ev->codec_ie_len = 0;
	}

	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_GET_CONFIG_RSP, ev,
		     sizeof(*ev) + ev->codec_ie_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

#if defined(CONFIG_BT_A2DP_SOURCE)
static int a2dp_delay_report_req(struct bt_a2dp_stream *stream, uint16_t value,
				 uint8_t *rsp_err_code)
{
	struct btp_a2dp_delay_report_req_ev ev;
	struct a2dp_connection *conn;
	uint8_t stream_id;

	conn = find_connection_by_stream(stream);
	if (conn == NULL) {
		*rsp_err_code = BT_AVDTP_BAD_STATE;
		return -EINVAL;
	}

	stream_id = get_stream_id(conn, stream);
	if (stream_id == 0xFF) {
		*rsp_err_code = BT_AVDTP_BAD_STATE;
		return -EINVAL;
	}

	*rsp_err_code = BT_AVDTP_SUCCESS;
	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;
	ev.stream_id = stream_id;
	ev.result = *rsp_err_code;
	ev.delay = sys_cpu_to_le16(value);
	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_DELAY_REPORT_REQ, &ev, sizeof(ev));

	return 0;
}
#endif

#if defined(CONFIG_BT_A2DP_SINK)
static void a2dp_delay_report_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
	struct btp_a2dp_delay_report_rsp_ev ev;
	struct a2dp_connection *conn;
	uint8_t stream_id;

	conn = find_connection_by_stream(stream);
	if (conn == NULL) {
		return;
	}

	stream_id = get_stream_id(conn, stream);
	if (stream_id == 0xFF) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;
	ev.stream_id = stream_id;
	ev.rsp_err_code = rsp_err_code;
	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_DELAY_REPORT_RSP, &ev, sizeof(ev));
}
#endif

static struct bt_a2dp_cb a2dp_cb = {
	.connected = a2dp_connected,
	.disconnected = a2dp_disconnected,
	.config_req = a2dp_config_req,
	.reconfig_req = a2dp_reconfig_req,
	.config_rsp = a2dp_config_rsp,
	.establish_req = a2dp_establish_req,
	.establish_rsp = a2dp_establish_rsp,
	.release_req = a2dp_release_req,
	.release_rsp = a2dp_release_rsp,
	.start_req = a2dp_start_req,
	.start_rsp = a2dp_start_rsp,
	.suspend_req = a2dp_suspend_req,
	.suspend_rsp = a2dp_suspend_rsp,
	.abort_req = a2dp_abort_req,
	.abort_rsp = a2dp_abort_rsp,
	.get_config_req = a2dp_get_config_req,
	.get_config_rsp = a2dp_get_config_rsp,
#if defined(CONFIG_BT_A2DP_SOURCE)
	.delay_report_req = a2dp_delay_report_req,
#endif
#if defined(CONFIG_BT_A2DP_SINK)
	.delay_report_rsp = a2dp_delay_report_rsp,
#endif
};

static uint8_t a2dp_read_supported_commands(const void *cmd, uint16_t cmd_len, void *rsp,
					    uint16_t *rsp_len)
{
	struct btp_a2dp_read_supported_commands_rp *rp = rsp;

	*rsp_len = tester_supported_commands(BTP_SERVICE_ID_A2DP, rp->data);
	*rsp_len += sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t a2dp_connect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_a2dp_connect_cmd *cp = cmd;
	struct a2dp_connection *conn;
	struct bt_conn *acl_conn;
	struct bt_a2dp *a2dp;

	conn = find_connection_by_address(&cp->address.a);
	if (conn != NULL) {
		return BTP_STATUS_FAILED;
	}

	acl_conn = bt_conn_lookup_addr_br(&cp->address.a);
	if (acl_conn == NULL) {
		acl_conn = bt_conn_create_br(&cp->address.a, BT_BR_CONN_PARAM_DEFAULT);
		if (acl_conn == NULL) {
			return BTP_STATUS_FAILED;
		}
	}

	a2dp = bt_a2dp_connect(acl_conn);
	bt_conn_unref(acl_conn);

	if (a2dp == NULL) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t a2dp_disconnect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_a2dp_disconnect_cmd *cp = cmd;
	struct a2dp_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_a2dp_disconnect(conn->a2dp);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static struct bt_a2dp_codec_ie bt_a2dp_ep_cap = {
	.len = BT_A2DP_SBC_IE_LENGTH,
	.codec_ie = {A2DP_SBC_SAMP_FREQ_16000 | A2DP_SBC_SAMP_FREQ_32000 |
			     A2DP_SBC_SAMP_FREQ_44100 | A2DP_SBC_SAMP_FREQ_48000 |
			     A2DP_SBC_CH_MODE_MONO | A2DP_SBC_CH_MODE_DUAL |
			     A2DP_SBC_CH_MODE_STEREO | A2DP_SBC_CH_MODE_JOINT,
		     A2DP_SBC_BLK_LEN_4 | A2DP_SBC_BLK_LEN_8 | A2DP_SBC_BLK_LEN_12 |
			     A2DP_SBC_BLK_LEN_16 | A2DP_SBC_SUBBAND_4 | A2DP_SBC_SUBBAND_8 |
			     A2DP_SBC_ALLOC_MTHD_SNR | A2DP_SBC_ALLOC_MTHD_LOUDNESS,
		     14U, 100U}};

static uint8_t a2dp_register_endpoint(const void *cmd, uint16_t cmd_len, void *rsp,
				      uint16_t *rsp_len)
{
	const struct btp_a2dp_register_endpoint_cmd *cp = cmd;
	struct a2dp_endpoint_info *ep_info;
	int err;

	ep_info = alloc_endpoint();
	if (ep_info == NULL) {
		return BTP_STATUS_FAILED;
	}

	ep_info->ep.codec_type = cp->codec_type;
	ep_info->ep.sep.sep_info.media_type = cp->media_type;
	ep_info->ep.sep.sep_info.tsep = cp->tsep;
	ep_info->ep.delay_report = cp->delay_report ? true : false;
	role = cp->tsep;

	if (cp->codec_ie_len > 0) {
		ep_info->codec_ie.len = cp->codec_ie_len;
		memcpy(ep_info->codec_ie.codec_ie, cp->codec_ie, cp->codec_ie_len);
		ep_info->ep.codec_cap = &ep_info->codec_ie;
	} else {
		ep_info->ep.codec_cap = &bt_a2dp_ep_cap;
		memcpy(&ep_info->codec_ie, &bt_a2dp_ep_cap.codec_ie,
		       sizeof(bt_a2dp_ep_cap.codec_ie));
	}

	err = bt_a2dp_register_ep(&ep_info->ep, cp->media_type, cp->tsep);
	if (err != 0) {
		k_free(ep_info->ep.codec_cap);
		ep_info->registered = false;
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

struct bt_a2dp_media_codec_capabilities {
	uint8_t media_type;
	uint8_t codec_type;
	uint8_t codec_ie[0];
};

static struct bt_avdtp_sep_info found_seps[BTP_A2DP_MAX_ENDPOINTS];
static uint8_t found_seps_count;
static struct bt_a2dp_sep_info discovered_ep[BTP_A2DP_MAX_ENDPOINTS];

static void copy_seps_to_discovered_ep(void)
{
	for (uint8_t i = 0; i < found_seps_count && i < BTP_A2DP_MAX_ENDPOINTS; i++) {
		discovered_ep[i].id = found_seps[i].id;
		discovered_ep[i].inuse = found_seps[i].inuse;
		discovered_ep[i].rfa0 = 0;
		discovered_ep[i].media_type = (uint8_t)found_seps[i].media_type;
		discovered_ep[i].tsep = (uint8_t)found_seps[i].tsep;
		discovered_ep[i].rfa1 = 0;
	}
}

static uint8_t discover_ep_cb(struct bt_a2dp *a2dp, struct bt_a2dp_ep_info *info,
			      struct bt_a2dp_ep **ep)
{
	struct btp_a2dp_get_capabilities_ev *ev;
	struct btp_a2dp_discovered_ev *discovered_ev;
	struct bt_a2dp_service_category_capabilities service_cap;
	struct bt_a2dp_media_codec_capabilities cap;
	uint8_t *buf;
	struct a2dp_connection *conn;
	int err;

	conn = find_connection_by_a2dp(a2dp);
	if (conn == NULL) {
		return BT_A2DP_DISCOVER_EP_STOP;
	}

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		return BT_A2DP_DISCOVER_EP_STOP;
	}

	if (info == NULL) {
		copy_seps_to_discovered_ep();
		tester_rsp_buffer_allocate(
			sizeof(*discovered_ev) + sizeof(struct bt_a2dp_sep_info) * found_seps_count,
			&buf);
		discovered_ev = (struct btp_a2dp_discovered_ev *)buf;
		bt_addr_copy(&discovered_ev->address.a, &conn->address);
		discovered_ev->address.type = BTP_BR_ADDRESS_TYPE;
		discovered_ev->result = 0;

		discovered_ev->len = sizeof(struct bt_a2dp_sep_info) * found_seps_count;
		memcpy(&discovered_ev->sep, discovered_ep, discovered_ev->len);
		tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_DISCOVERED, discovered_ev,
			     sizeof(*discovered_ev) + discovered_ev->len);
		tester_rsp_buffer_free();
		return BT_A2DP_DISCOVER_EP_STOP;
	}

	found_seps_count++;

	tester_rsp_buffer_allocate(sizeof(*ev) +
					   sizeof(struct bt_a2dp_service_category_capabilities) +
					   info->codec_cap.len,
				   &buf);
	ev = (struct btp_a2dp_get_capabilities_ev *)buf;
	bt_addr_copy(&ev->address.a, &conn->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->ep_id = info->sep_info->id;

	cap.media_type = info->sep_info->media_type;
	cap.codec_type = info->codec_type;
	memcpy(cap.codec_ie, info->codec_cap.codec_ie, info->codec_cap.len);

	service_cap.service_category = BT_AVDTP_SERVICE_MEDIA_CODEC;
	service_cap.losc = sizeof(cap) + info->codec_cap.len;
	memcpy(&service_cap.capability_info, &cap, service_cap.losc);
	memcpy(ev->cap, &service_cap, sizeof(service_cap) + service_cap.losc);

	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_GET_CAPABILITIES, ev,
		     sizeof(*ev) + sizeof(service_cap) + service_cap.losc);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();

	return BT_A2DP_DISCOVER_EP_CONTINUE;
}

static struct bt_a2dp_discover_param discover_param = {
	.cb = discover_ep_cb,
	.seps_info = &found_seps[0],
	.avdtp_version = AVDTP_VERSION + 1,
	.sep_count = 5,
};

static struct bt_sdp_discover_params discov_a2dp = {
	.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR,
	.pool = &find_avdtp_version_pool,
};

static uint8_t a2dp_sdp_discover_cb(struct bt_conn *conn, struct bt_sdp_client_result *result,
				    const struct bt_sdp_discover_params *params)
{
	int8_t err;
	uint16_t peer_avdtp_version = AVDTP_VERSION_1_3;
	struct bt_a2dp *a2dp = NULL;

	if ((result == NULL) || (result->resp_buf == NULL) || (result->resp_buf->len == 0)) {
		return BT_SDP_DISCOVER_UUID_STOP;
	}

	err = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_AVDTP, &peer_avdtp_version);
	if (err == 0) {
		discover_param.avdtp_version = peer_avdtp_version;
		for (uint8_t i = 0; i < ARRAY_SIZE(a2dp_connections); i++) {
			if (a2dp_connections[i].acl_conn == conn) {
				a2dp = a2dp_connections[i].a2dp;
				break;
			}
		}
		err = bt_a2dp_discover(a2dp, &discover_param);
		if (err != 0) {
			return BT_SDP_DISCOVER_UUID_STOP;
		}
	}
	return BT_SDP_DISCOVER_UUID_STOP;
}

static uint8_t a2dp_discover(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_a2dp_discover_cmd *cp = cmd;
	struct a2dp_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (conn->acl_conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (role == BT_AVDTP_SOURCE) {
		discov_a2dp.uuid = a2dp_snk_uuid;
	} else if (role == BT_AVDTP_SINK) {
		discov_a2dp.uuid = a2dp_src_uuid;
	} else {
		return BTP_STATUS_FAILED;
	}

	discov_a2dp.func = a2dp_sdp_discover_cb;

	err = bt_sdp_discover(conn->acl_conn, &discov_a2dp);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}
	return BTP_STATUS_SUCCESS;
}

static struct bt_a2dp_codec_ie bt_a2dp_ep_cfg = {
	.len = BT_A2DP_SBC_IE_LENGTH,
	.codec_ie = {A2DP_SBC_SAMP_FREQ_48000 | A2DP_SBC_CH_MODE_JOINT,
		     A2DP_SBC_BLK_LEN_16 | A2DP_SBC_SUBBAND_8 | A2DP_SBC_ALLOC_MTHD_LOUDNESS, 14U,
		     100U}};

static uint8_t a2dp_stream_config(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_a2dp_config_cmd *cp = cmd;
	struct a2dp_connection *conn;
	struct a2dp_endpoint_info *local_ep_info;
	struct bt_a2dp_ep remote_ep;
	struct bt_a2dp_codec_cfg config;
	struct bt_a2dp_codec_ie codec_ie;
	struct bt_a2dp_stream *stream;
	uint8_t stream_id;
	int err;
	uint8_t i = 0;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	local_ep_info = find_endpoint_by_id(cp->local_ep_id);
	if (local_ep_info == NULL) {
		return BTP_STATUS_FAILED;
	}

	memset(&remote_ep, 0, sizeof(remote_ep));

	for (i = 0; i < ARRAY_SIZE(found_seps); i++) {
		if (found_seps[i].id == cp->remote_ep_id) {
			memcpy(&remote_ep.sep.sep_info, &found_seps[i],
			       sizeof(struct bt_avdtp_sep_info));
			break;
		}
	}
	if (i >= ARRAY_SIZE(found_seps)) {
		return BTP_STATUS_FAILED;
	}

	if (cp->codec_ie_len > 0) {
		codec_ie.len = cp->codec_ie_len;
		memcpy(codec_ie.codec_ie, cp->codec_ie, cp->codec_ie_len);
	} else {
		memcpy(&codec_ie, &bt_a2dp_ep_cfg, sizeof(bt_a2dp_ep_cfg));
	}

	config.delay_report = cp->delay_report ? true : false;
	config.codec_config = &codec_ie;

	stream_id = 0xFF;
	for (size_t i = 0; i < ARRAY_SIZE(conn->streams); i++) {
		if (!conn->streams[i].in_use) {
			stream = &conn->streams[i].stream;
			stream_id = i;
			conn->streams[i].in_use = true;
			conn->streams[i].stream_id = stream_id;
			break;
		}
	}

	if (stream_id == 0xFF) {
		return BTP_STATUS_FAILED;
	}

	bt_a2dp_stream_cb_register(stream, &stream_ops);

	err = bt_a2dp_stream_config(conn->a2dp, stream, &local_ep_info->ep, &remote_ep, &config);
	if (err != 0) {

		conn->streams[stream_id].in_use = false;
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t a2dp_stream_establish(const void *cmd, uint16_t cmd_len, void *rsp,
				     uint16_t *rsp_len)
{
	const struct btp_a2dp_establish_cmd *cp = cmd;
	struct a2dp_connection *conn;
	struct bt_a2dp_stream *stream;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	stream = get_stream_by_id(conn, cp->stream_id);
	if (stream == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_a2dp_stream_establish(stream);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t a2dp_stream_release(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_a2dp_release_cmd *cp = cmd;
	struct a2dp_connection *conn;
	struct bt_a2dp_stream *stream;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	stream = get_stream_by_id(conn, cp->stream_id);
	if (stream == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_a2dp_stream_release(stream);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t a2dp_stream_start(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_a2dp_start_cmd *cp = cmd;
	struct a2dp_connection *conn;
	struct bt_a2dp_stream *stream;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	stream = get_stream_by_id(conn, cp->stream_id);
	if (stream == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_a2dp_stream_start(stream);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t a2dp_stream_suspend(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_a2dp_suspend_cmd *cp = cmd;
	struct a2dp_connection *conn;
	struct bt_a2dp_stream *stream;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	stream = get_stream_by_id(conn, cp->stream_id);
	if (stream == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_a2dp_stream_suspend(stream);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t a2dp_stream_reconfig(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_a2dp_reconfig_cmd *cp = cmd;
	struct a2dp_connection *conn;
	struct bt_a2dp_stream *stream;
	struct bt_a2dp_codec_cfg config;
	struct bt_a2dp_codec_ie codec_ie;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	stream = get_stream_by_id(conn, cp->stream_id);
	if (stream == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (cp->codec_ie_len > 0) {
		codec_ie.len = cp->codec_ie_len;
		memcpy(codec_ie.codec_ie, cp->codec_ie, cp->codec_ie_len);
	} else {
		memcpy(&codec_ie, &bt_a2dp_ep_cfg, sizeof(bt_a2dp_ep_cfg));
	}

	config.delay_report = cp->delay_report ? true : false;
	config.codec_config = &codec_ie;

	err = bt_a2dp_stream_reconfig(stream, &config);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t a2dp_stream_abort(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_a2dp_abort_cmd *cp = cmd;
	struct a2dp_connection *conn;
	struct bt_a2dp_stream *stream;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	stream = get_stream_by_id(conn, cp->stream_id);
	if (stream == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_a2dp_stream_abort(stream);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t a2dp_stream_get_config(const void *cmd, uint16_t cmd_len, void *rsp,
				      uint16_t *rsp_len)
{
	const struct btp_a2dp_get_config_cmd *cp = cmd;
	struct a2dp_connection *conn;
	struct bt_a2dp_stream *stream;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	stream = get_stream_by_id(conn, cp->stream_id);
	if (stream == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_a2dp_stream_get_config(stream);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

#if defined(CONFIG_BT_A2DP_SINK)
static uint8_t a2dp_stream_delay_report(const void *cmd, uint16_t cmd_len, void *rsp,
					uint16_t *rsp_len)
{
	const struct btp_a2dp_delay_report_cmd *cp = cmd;
	struct a2dp_connection *conn;
	struct bt_a2dp_stream *stream;
	uint16_t delay;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	stream = get_stream_by_id(conn, cp->stream_id);
	if (stream == NULL) {
		return BTP_STATUS_FAILED;
	}

	delay = sys_le16_to_cpu(cp->delay);

	err = bt_a2dp_stream_delay_report(stream, delay);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif

static uint8_t a2dp_get_capabilities(const void *cmd, uint16_t cmd_len, void *rsp,
				     uint16_t *rsp_len)
{
	return BTP_STATUS_SUCCESS;
}

static uint8_t a2dp_get_all_capabilities(const void *cmd, uint16_t cmd_len, void *rsp,
					 uint16_t *rsp_len)
{
	return BTP_STATUS_SUCCESS;
}

static const struct btp_handler a2dp_handlers[] = {
	{
		.opcode = BTP_A2DP_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = a2dp_read_supported_commands
	},
	{
		.opcode = BTP_A2DP_CONNECT,
		.expect_len = sizeof(struct btp_a2dp_connect_cmd),
		.func = a2dp_connect
	},
	{
		.opcode = BTP_A2DP_DISCONNECT,
		.expect_len = sizeof(struct btp_a2dp_disconnect_cmd),
		.func = a2dp_disconnect
	},
	{
		.opcode = BTP_A2DP_REGISTER_ENDPOINT,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = a2dp_register_endpoint
	},
	{
		.opcode = BTP_A2DP_DISCOVER,
		.expect_len = sizeof(struct btp_a2dp_discover_cmd),
		.func = a2dp_discover
	},
	{
		.opcode = BTP_A2DP_GET_CAPABILITIES,
		.expect_len = sizeof(struct btp_a2dp_get_capabilities_cmd),
		.func = a2dp_get_capabilities
	},
	{
		.opcode = BTP_A2DP_GET_ALL_CAPABILITIES,
		.expect_len = sizeof(struct btp_a2dp_get_all_capabilities_cmd),
		.func = a2dp_get_all_capabilities
	},
	{
		.opcode = BTP_A2DP_CONFIG,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = a2dp_stream_config
	},
	{
		.opcode = BTP_A2DP_ESTABLISH,
		.expect_len = sizeof(struct btp_a2dp_establish_cmd),
		.func = a2dp_stream_establish
	},
	{
		.opcode = BTP_A2DP_RELEASE,
		.expect_len = sizeof(struct btp_a2dp_release_cmd),
		.func = a2dp_stream_release
	},
	{
		.opcode = BTP_A2DP_START,
		.expect_len = sizeof(struct btp_a2dp_start_cmd),
		.func = a2dp_stream_start
	},
	{
		.opcode = BTP_A2DP__SUSPEND,
		.expect_len = sizeof(struct btp_a2dp_suspend_cmd),
		.func = a2dp_stream_suspend
	},
	{
		.opcode = BTP_A2DP_RECONFIG,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = a2dp_stream_reconfig
	},
	{
		.opcode = BTP_A2DP_ABORT,
		.expect_len = sizeof(struct btp_a2dp_abort_cmd),
		.func = a2dp_stream_abort
	},
	{
		.opcode = BTP_A2DP_GET_CONFIG,
		.expect_len = sizeof(struct btp_a2dp_get_config_cmd),
		.func = a2dp_stream_get_config
	},
#if defined(CONFIG_BT_A2DP_SINK)
	{
		.opcode = BTP_A2DP_DELAY_REPORT,
		.expect_len = sizeof(struct btp_a2dp_delay_report_cmd),
		.func = a2dp_stream_delay_report
	},
#endif
};

uint8_t tester_init_a2dp(void)
{
	int err;

	memset(a2dp_connections, 0, sizeof(a2dp_connections));
	memset(registered_endpoints, 0, sizeof(registered_endpoints));

	err = bt_a2dp_register_cb(&a2dp_cb);
	if (err != 0) {

		return BTP_STATUS_FAILED;
	}

#if defined(CONFIG_BT_A2DP_SINK)
	err = bt_sdp_register_service(&a2dp_sink_rec);
	if (err != 0) {

		return BTP_STATUS_FAILED;
	}
#endif

#if defined(CONFIG_BT_A2DP_SOURCE)
	err = bt_sdp_register_service(&a2dp_source_rec);
	if (err != 0) {

		return BTP_STATUS_FAILED;
	}

	k_work_init_delayable(&send_media_work, a2dp_send_media_timeout);
	active_stream = NULL;
#endif

	tester_register_command_handlers(BTP_SERVICE_ID_A2DP, a2dp_handlers,
					 ARRAY_SIZE(a2dp_handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_a2dp(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(registered_endpoints); i++) {
		if (registered_endpoints[i].registered &&
		    registered_endpoints[i].ep.codec_cap != NULL) {
			k_free(registered_endpoints[i].ep.codec_cap);
			registered_endpoints[i].ep.codec_cap = NULL;
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(a2dp_connections); i++) {
		if (a2dp_connections[i].in_use && a2dp_connections[i].a2dp != NULL) {
			bt_a2dp_disconnect(a2dp_connections[i].a2dp);
		}
	}

	return BTP_STATUS_SUCCESS;
}
