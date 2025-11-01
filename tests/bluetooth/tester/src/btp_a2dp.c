/* btp_a2dp.h - Bluetooth tester headers */

/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/a2dp_codec_sbc.h>
#include <zephyr/bluetooth/classic/a2dp.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <sys/types.h>

#include "btp/btp.h"

static uint8_t role;
static struct k_work_delayable send_media;
static struct bt_a2dp *default_a2dp;
static struct bt_a2dp_ep *found_peer_sbc_endpoint;
static struct bt_a2dp_ep *registered_sbc_endpoint;
struct bt_a2dp_codec_ie peer_sbc_capabilities;
static struct bt_a2dp_ep peer_sbc_endpoint = {
	.codec_cap = &peer_sbc_capabilities,
};
static struct bt_a2dp_stream sbc_stream;
#define A2DP_SERVICE_LEN 512
NET_BUF_POOL_FIXED_DEFINE(find_avdtp_version_pool, 1, A2DP_SERVICE_LEN,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static const struct bt_uuid *a2dp_snk_uuid = BT_UUID_DECLARE_16(BT_SDP_AUDIO_SINK_SVCLASS);
static const struct bt_uuid *a2dp_src_uuid = BT_UUID_DECLARE_16(BT_SDP_AUDIO_SOURCE_SVCLASS);
static struct bt_sdp_discover_params discov_a2dp = {
	.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR,
	.pool = &find_avdtp_version_pool,
};

NET_BUF_POOL_DEFINE(a2dp_tx_pool, CONFIG_BT_MAX_CONN, BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
		    CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
BT_A2DP_SBC_EP_CFG(sbc_cfg_default, A2DP_SBC_SAMP_FREQ_48000, A2DP_SBC_CH_MODE_JOINT,
		   A2DP_SBC_BLK_LEN_16 | A2DP_SBC_SUBBAND_8, A2DP_SBC_SUBBAND_8,
		   A2DP_SBC_ALLOC_MTHD_LOUDNESS, 14U, 100U);

#if defined(CONFIG_BT_A2DP_SINK)

BT_A2DP_SBC_SINK_EP(sink_sbc_endpoint,
		    A2DP_SBC_SAMP_FREQ_16000 | A2DP_SBC_SAMP_FREQ_32000 | A2DP_SBC_SAMP_FREQ_44100 |
			    A2DP_SBC_SAMP_FREQ_48000,
		    A2DP_SBC_CH_MODE_MONO | A2DP_SBC_CH_MODE_DUAL | A2DP_SBC_CH_MODE_STEREO |
			    A2DP_SBC_CH_MODE_JOINT,
		    A2DP_SBC_BLK_LEN_4 | A2DP_SBC_BLK_LEN_8 | A2DP_SBC_BLK_LEN_12 |
			    A2DP_SBC_BLK_LEN_16,
		    A2DP_SBC_SUBBAND_4 | A2DP_SBC_SUBBAND_8,
		    A2DP_SBC_ALLOC_MTHD_SNR | A2DP_SBC_ALLOC_MTHD_LOUDNESS, 14U, 100U, false);

static struct bt_sdp_attribute a2dp_sink_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3), /* 35 03 */
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16), /* 19 */
			BT_SDP_ARRAY_16(BT_SDP_AUDIO_SINK_SVCLASS) /* 11 0B */
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 16),/* 35 10 */
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),/* 35 06 */
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), /* 19 */
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP) /* 01 00 */
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16), /* 09 */
				BT_SDP_ARRAY_16(BT_UUID_AVDTP_VAL) /* 00 19 */
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),/* 35 06 */
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), /* 19 */
				BT_SDP_ARRAY_16(BT_UUID_AVDTP_VAL) /* 00 19 */
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16), /* 09 */
				BT_SDP_ARRAY_16(0x0103U) /* AVDTP version: 01 03 */
			},
			)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8), /* 35 08 */
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6), /* 35 06 */
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), /* 19 */
				BT_SDP_ARRAY_16(BT_SDP_ADVANCED_AUDIO_SVCLASS) /* 11 0d */
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16), /* 09 */
				BT_SDP_ARRAY_16(0x0104U) /* 01 04 */
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("A2DPSink"),
	BT_SDP_SUPPORTED_FEATURES(0x0001U),
};

static struct bt_sdp_record a2dp_sink_rec = BT_SDP_RECORD(a2dp_sink_attrs);

void stream_recv(struct bt_a2dp_stream *stream, struct net_buf *buf, uint16_t seq_num, uint32_t ts)
{
	uint8_t sbc_hdr;
	struct btp_a2dp_recv_media ev;

	if (buf->len < 1U) {
		return;
	}
	sbc_hdr = net_buf_pull_u8(buf);
	ev.frames_num = (uint8_t)BT_A2DP_SBC_MEDIA_HDR_NUM_FRAMES_GET(sbc_hdr);
	ev.length = buf->len;
	memcpy(ev.data, buf->data, ev.length);
	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_RECV_MEDIA, &ev, sizeof(ev));
}

#endif /*CONFIG_BT_A2DP_SINK*/

#if defined(CONFIG_BT_A2DP_SOURCE)

BT_A2DP_SBC_SINK_EP(source_sbc_endpoint,
		    A2DP_SBC_SAMP_FREQ_16000 | A2DP_SBC_SAMP_FREQ_32000 | A2DP_SBC_SAMP_FREQ_44100 |
			    A2DP_SBC_SAMP_FREQ_48000,
		    A2DP_SBC_CH_MODE_MONO | A2DP_SBC_CH_MODE_DUAL | A2DP_SBC_CH_MODE_STEREO |
			    A2DP_SBC_CH_MODE_JOINT,
		    A2DP_SBC_BLK_LEN_4 | A2DP_SBC_BLK_LEN_8 | A2DP_SBC_BLK_LEN_12 |
			    A2DP_SBC_BLK_LEN_16,
		    A2DP_SBC_SUBBAND_4 | A2DP_SBC_SUBBAND_8,
		    A2DP_SBC_ALLOC_MTHD_SNR | A2DP_SBC_ALLOC_MTHD_LOUDNESS, 14U, 100U, false);

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
				BT_SDP_ARRAY_16(0x0103U)
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
				BT_SDP_ARRAY_16(0x0104U)
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("A2DPSource"),
	BT_SDP_SUPPORTED_FEATURES(0x0001U),
};

static struct bt_sdp_record a2dp_source_rec = BT_SDP_RECORD(a2dp_source_attrs);

#endif /*CONFIG_BT_A2DP_SOURCE*/

static struct bt_a2dp_stream_ops stream_ops = {
#if defined(CONFIG_BT_A2DP_SINK)
	.recv = stream_recv,
#endif
};

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

static uint8_t supported_commands(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	struct btp_a2dp_read_supported_commands_rp *rp = rsp;

	*rsp_len = tester_supported_commands(BTP_SERVICE_ID_A2DP, rp->data);
	*rsp_len += sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static void a2dp_send_media_timeout(struct k_work *work)
{
	struct net_buf *buf;
	int ret;

	buf = bt_a2dp_stream_create_pdu(&a2dp_tx_pool, K_FOREVER);
	if (buf == NULL) {
		return;
	}

	/* num of frames is 1 */
	net_buf_add_u8(buf, (uint8_t)BT_A2DP_SBC_MEDIA_HDR_ENCODE(2, 0, 0, 0));
	net_buf_add_mem(buf, media_data, sizeof(media_data));
	ret = bt_a2dp_stream_send(&sbc_stream, buf, 0U, 0U);
	if (ret < 0) {
		net_buf_unref(buf);
		return;
	}
	k_work_schedule(&send_media, K_MSEC(1000));
}

static void app_connected(struct bt_a2dp *a2dp, int err)
{
	struct btp_a2dp_connected_ev ev;

	(void)memset(&ev, 0, sizeof(ev));
	if (err == 0) {
		default_a2dp = a2dp;
	}
	ev.errcode = err;
	if (role == BT_AVDTP_SOURCE) {
		k_work_init_delayable(&send_media, a2dp_send_media_timeout);
	}
	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_CONNECTED, &ev, sizeof(ev));
}

static void app_disconnected(struct bt_a2dp *a2dp)
{
	found_peer_sbc_endpoint = NULL;
	if (role == BT_AVDTP_SOURCE) {
		k_work_cancel_delayable(&send_media);
	}
	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_DISCONNECTED, NULL, 0);
}

static int app_config_req(struct bt_a2dp *a2dp, struct bt_a2dp_ep *ep,
			  struct bt_a2dp_codec_cfg *codec_cfg, struct bt_a2dp_stream **stream,
			  uint8_t *rsp_err_code)
{

	bt_a2dp_stream_cb_register(&sbc_stream, &stream_ops);
	*stream = &sbc_stream;
	*rsp_err_code = 0;
	return 0;
}

static void app_config_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
	struct btp_a2dp_set_config_rsp ev;

	memset(&ev, 0, sizeof(ev));
	ev.errcode = rsp_err_code;
	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_SET_CONFIGURATION_RSP, &ev, sizeof(ev));
}

static void app_establish_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
	struct btp_a2dp_establish_rsp ev;

	memset(&ev, 0, sizeof(ev));
	ev.errcode = rsp_err_code;
	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_ESTABLISH_RSP, &ev, sizeof(ev));
}

static void app_release_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
	struct btp_a2dp_release_rsp ev;

	memset(&ev, 0, sizeof(ev));
	ev.errcode = rsp_err_code;
	if (role == BT_AVDTP_SOURCE && rsp_err_code == 0) {
		k_work_cancel_delayable(&send_media);
	}
	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_RELEASE_RSP, &ev, sizeof(ev));
}

static int app_start_req(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code)
{
	*rsp_err_code = 0;

	if (role == BT_AVDTP_SOURCE && *rsp_err_code == 0) {
		k_work_schedule(&send_media, K_MSEC(1000));
	}

	return 0;
}

static void app_start_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
	struct btp_a2dp_start_rsp ev;

	memset(&ev, 0, sizeof(ev));
	ev.errcode = rsp_err_code;
	if (role == BT_AVDTP_SOURCE && rsp_err_code == 0) {
		k_work_schedule(&send_media, K_MSEC(1000));
	}
	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_START_RSP, &ev, sizeof(ev));
}

static int app_suspend_req(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code)
{
	*rsp_err_code = 0;
	if (role == BT_AVDTP_SOURCE && *rsp_err_code == 0) {
		k_work_cancel_delayable(&send_media);
	}
	return 0;
}

static void app_suspend_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
	struct btp_a2dp_suspend_rsp ev;

	memset(&ev, 0, sizeof(ev));
	ev.errcode = rsp_err_code;
	if (role == BT_AVDTP_SOURCE && rsp_err_code == 0) {
		k_work_cancel_delayable(&send_media);
	}
	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_SUSPEND_RSP, &ev, sizeof(ev));
}

void app_abort_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
	struct btp_a2dp_abort_rsp ev;

	memset(&ev, 0, sizeof(ev));
	ev.errcode = rsp_err_code;
	tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_ABORT_RSP, &ev, sizeof(ev));
}

#if defined(CONFIG_BT_A2DP_SOURCE)
int app_delay_report_req(struct bt_a2dp_stream *stream, uint16_t value,
			 uint8_t *rsp_err_code)
{
	*rsp_err_code = 0;
	return 0;
}
#endif

#if defined(CONFIG_BT_A2DP_SINK)
void app_delay_report_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{

	if (rsp_err_code == 0) {
		struct btp_a2dp_send_delay_report_rsp ev;

		memset(&ev, 0, sizeof(ev));
		ev.errcode = rsp_err_code;
		tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_SEND_DELAY_REPORT_RSP,
			     &ev, sizeof(ev));
	}
}

static uint8_t a2dp_enable_delay_report(const void *cmd, uint16_t cmd_len,
					void *rsp, uint16_t *rsp_len)
{
	sink_sbc_endpoint.delay_report = true;

	return BTP_STATUS_SUCCESS;
}

static uint8_t a2dp_send_delay_report(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	int err;
	const struct btp_a2dp_send_delay_report *cp = cmd;

	err = bt_a2dp_stream_delay_report(&sbc_stream, cp->delay);
	if (err < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif

struct bt_a2dp_cb a2dp_cb = {
	.connected = app_connected,
	.disconnected = app_disconnected,
	.config_req = app_config_req,
	.config_rsp = app_config_rsp,
	.establish_rsp = app_establish_rsp,
	.release_rsp = app_release_rsp,
	.start_req = app_start_req,
	.start_rsp = app_start_rsp,
	.suspend_req = app_suspend_req,
	.suspend_rsp = app_suspend_rsp,
	.abort_rsp = app_abort_rsp,
#if defined(CONFIG_BT_A2DP_SOURCE)
	.delay_report_req = app_delay_report_req,
#endif
#if defined(CONFIG_BT_A2DP_SINK)
	.delay_report_rsp = app_delay_report_rsp,
#endif
};

static uint8_t register_ep(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	int err = -1;
	const struct btp_a2dp_register_ep_cmd *cp = cmd;

	if (cp->media_type == BTP_A2DP_REGISTER_EP_CODEC_SBC) {
		if (cp->sep_type == BT_AVDTP_SINK) {
#if defined(CONFIG_BT_A2DP_SINK)
			err = bt_a2dp_register_ep(&sink_sbc_endpoint, BT_AVDTP_AUDIO,
						  BT_AVDTP_SINK);
			if (err != 0) {
				return BTP_STATUS_FAILED;
			}
			registered_sbc_endpoint = &sink_sbc_endpoint;
			role = BT_AVDTP_SINK;
#endif /*CONFIG_BT_A2DP_SINK*/
		} else if (cp->sep_type == BT_AVDTP_SOURCE) {
#if defined(CONFIG_BT_A2DP_SOURCE)
			err = bt_a2dp_register_ep(&source_sbc_endpoint, BT_AVDTP_AUDIO,
						  BT_AVDTP_SOURCE);
			if (err != 0) {
				return BTP_STATUS_FAILED;
			}
			registered_sbc_endpoint = &source_sbc_endpoint;
			role = BT_AVDTP_SOURCE;
#endif /*CONFIG_BT_A2DP_SOURCE*/
		} else {
			return BTP_STATUS_FAILED;
		}
	} else {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t a2dp_connect(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	struct bt_conn *acl_conn;
	const struct btp_a2dp_connect_cmd *cp = cmd;

	acl_conn = bt_conn_lookup_addr_br(&cp->address);

	if (acl_conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	default_a2dp = bt_a2dp_connect(acl_conn);
	bt_conn_unref(acl_conn);
	if (NULL == default_a2dp) {
		return BTP_STATUS_FAILED;
	}
	return BTP_STATUS_SUCCESS;
}

static uint8_t a2dp_configure(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	int err;

	if (default_a2dp != NULL) {
		if (registered_sbc_endpoint == NULL) {
			return BTP_STATUS_FAILED;
		}

		if (found_peer_sbc_endpoint == NULL) {
			found_peer_sbc_endpoint = &peer_sbc_endpoint;
		}

		bt_a2dp_stream_cb_register(&sbc_stream, &stream_ops);

		err = bt_a2dp_stream_config(default_a2dp, &sbc_stream, registered_sbc_endpoint,
					    found_peer_sbc_endpoint, &sbc_cfg_default);
		if (err != 0) {
			return BTP_STATUS_FAILED;
		}
	} else {

		return BTP_STATUS_FAILED;
	}
	return BTP_STATUS_SUCCESS;
}

static uint8_t a2dp_reconfigure(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	if (bt_a2dp_stream_reconfig(&sbc_stream, &sbc_cfg_default) != 0) {
		return BTP_STATUS_FAILED;
	}
	return BTP_STATUS_SUCCESS;
}

static uint8_t a2dp_establish(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	if (bt_a2dp_stream_establish(&sbc_stream) != 0) {
		return BTP_STATUS_FAILED;
	}
	return BTP_STATUS_SUCCESS;
}

static uint8_t a2dp_start(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	if (bt_a2dp_stream_start(&sbc_stream) != 0) {
		return BTP_STATUS_FAILED;
	}
	return BTP_STATUS_SUCCESS;
}

static uint8_t a2dp_suspend(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	if (bt_a2dp_stream_suspend(&sbc_stream) != 0) {
		return BTP_STATUS_FAILED;
	}
	return BTP_STATUS_SUCCESS;
}

static uint8_t a2dp_release(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	if (bt_a2dp_stream_release(&sbc_stream) != 0) {
		return BTP_STATUS_FAILED;
	}
	return BTP_STATUS_SUCCESS;
}

static uint8_t a2dp_abort(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	if (bt_a2dp_stream_abort(&sbc_stream) != 0) {
		return BTP_STATUS_FAILED;
	}
	return BTP_STATUS_SUCCESS;
}

static uint8_t bt_a2dp_discover_peer_endpoint_cb(struct bt_a2dp *a2dp, struct bt_a2dp_ep_info *info,
						 struct bt_a2dp_ep **ep)
{
	if (info != NULL) {
		if ((info->codec_type == BT_A2DP_SBC) && (ep != NULL)) {
			*ep = &peer_sbc_endpoint;
			found_peer_sbc_endpoint = &peer_sbc_endpoint;
		}
	} else {
		tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_GET_CAPABILITIES_RSP, NULL, 0);
	}
	return BT_A2DP_DISCOVER_EP_CONTINUE;
}

static struct bt_avdtp_sep_info found_seps[5];
struct bt_a2dp_discover_param discover_param = {
	.cb = bt_a2dp_discover_peer_endpoint_cb,
	.seps_info = &found_seps[0],
	.sep_count = 5,
};

static uint8_t a2dp_sdp_discover_cb(struct bt_conn *conn,
					   struct bt_sdp_client_result *result,
					   const struct bt_sdp_discover_params *params)
{
	int8_t err;
	struct btp_a2dp_discover_rsp ev;
	uint16_t peer_avdtp_version = AVDTP_VERSION_1_3;

	memset(&ev, 0, sizeof(ev));
	if ((result == NULL) || (result->resp_buf == NULL) || (result->resp_buf->len == 0)) {
		err = -EINVAL;
		ev.errcode = err;
		tester_event(BTP_SERVICE_ID_A2DP, BTP_A2DP_EV_DISCOVER_RSP, &ev, sizeof(ev));
		return BT_SDP_DISCOVER_UUID_STOP;
	}

	err = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_AVDTP, &peer_avdtp_version);
	if (err == 0 && default_a2dp != NULL) {
		discover_param.avdtp_version = peer_avdtp_version;
		bt_a2dp_discover(default_a2dp, &discover_param);
	}
	return BT_SDP_DISCOVER_UUID_STOP;
}

static uint8_t a2dp_discover(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	int err;
	const struct btp_a2dp_discover_cmd *cp = cmd;
	struct bt_conn *acl_conn;

	acl_conn = bt_conn_lookup_addr_br(&cp->address);

	if (acl_conn == NULL) {
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

	err = bt_sdp_discover(acl_conn, &discov_a2dp);
	bt_conn_unref(acl_conn);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}
	return BTP_STATUS_SUCCESS;
}

static const struct btp_handler handlers[] = {
	{
		.opcode = BTP_A2DP_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = supported_commands,
	},
	{
		.opcode = BTP_A2DP_REGISTER_EP,
		.expect_len = sizeof(struct btp_a2dp_register_ep_cmd),
		.func = register_ep,
	},
	{
		.opcode = BTP_A2DP_CONNECT,
		.expect_len = sizeof(struct btp_a2dp_connect_cmd),
		.func = a2dp_connect,
	},
	{
		.opcode = BTP_A2DP_DISCOVER,
		.expect_len = sizeof(struct btp_a2dp_discover_cmd),
		.func = a2dp_discover,
	},
	{
		.opcode = BTP_A2DP_CONFIGURE,
		.expect_len = 0,
		.func = a2dp_configure,
	},
	{
		.opcode = BTP_A2DP_ESTABLISH,
		.expect_len = 0,
		.func = a2dp_establish,
	},
	{
		.opcode = BTP_A2DP_START,
		.expect_len = 0,
		.func = a2dp_start,
	},
	{
		.opcode = BTP_A2DP_SUSPEND,
		.expect_len = 0,
		.func = a2dp_suspend,
	},
	{
		.opcode = BTP_A2DP_RELEASE,
		.expect_len = 0,
		.func = a2dp_release,
	},
	{
		.opcode = BTP_A2DP_ABORT,
		.expect_len = 0,
		.func = a2dp_abort,
	},
	{
		.opcode = BTP_A2DP_RECONFIGURE,
		.expect_len = 0,
		.func = a2dp_reconfigure,
	},
#ifdef CONFIG_BT_A2DP_SINK
	{
		.opcode = BTP_A2DP_ENABLE_DELAY_REPORT,
		.expect_len = 0,
		.func = a2dp_enable_delay_report,
	},
	{
		.opcode = BTP_A2DP_SEND_DELAY_REPORT,
		.expect_len = sizeof(struct btp_a2dp_send_delay_report),
		.func = a2dp_send_delay_report,
	},
#endif
};

uint8_t tester_init_a2dp(void)
{
	int err;

	bt_a2dp_register_cb(&a2dp_cb);
#if defined(CONFIG_BT_A2DP_SINK)
	err = bt_sdp_register_service(&a2dp_sink_rec);
	if (err) {
		return BTP_STATUS_FAILED;
	}
#endif /*CONFIG_BT_A2DP_SINK*/

#if defined(CONFIG_BT_A2DP_SOURCE)
	err = bt_sdp_register_service(&a2dp_source_rec);
	if (err) {
		return BTP_STATUS_FAILED;
	}
#endif /*CONFIG_BT_A2DP_SOURCE*/

	tester_register_command_handlers(BTP_SERVICE_ID_A2DP, handlers, ARRAY_SIZE(handlers));
	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_a2dp(void)
{
	return BTP_STATUS_SUCCESS;
}
