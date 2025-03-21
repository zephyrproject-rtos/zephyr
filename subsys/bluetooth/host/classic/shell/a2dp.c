/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/a2dp_codec_sbc.h>
#include <zephyr/bluetooth/classic/a2dp.h>
#include <zephyr/bluetooth/classic/sdp.h>

#include <zephyr/shell/shell.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

struct bt_a2dp *default_a2dp;
static uint8_t a2dp_sink_sdp_registered;
static uint8_t a2dp_source_sdp_registered;
static uint8_t a2dp_initied;
BT_A2DP_SBC_SINK_EP_DEFAULT(sink_sbc_endpoint);
BT_A2DP_SBC_SOURCE_EP_DEFAULT(source_sbc_endpoint);
struct bt_a2dp_codec_ie peer_sbc_capabilities;
static struct bt_a2dp_ep peer_sbc_endpoint = {
	.codec_cap = &peer_sbc_capabilities,
};
static struct bt_a2dp_ep *found_peer_sbc_endpoint;
static struct bt_a2dp_ep *registered_sbc_endpoint;
static struct bt_a2dp_stream sbc_stream;
static struct bt_a2dp_stream_ops stream_ops;

#if defined(CONFIG_BT_A2DP_SOURCE)
static uint8_t media_data[] = {
0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
};
#endif

NET_BUF_POOL_DEFINE(a2dp_tx_pool, CONFIG_BT_MAX_CONN,
		BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
		CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

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
				BT_SDP_ARRAY_16(0x0100U) /* AVDTP version: 01 00 */
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
				BT_SDP_ARRAY_16(0x0103U) /* 01 03 */
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("A2DPSink"),
	BT_SDP_SUPPORTED_FEATURES(0x0001U),
};

static struct bt_sdp_record a2dp_sink_rec = BT_SDP_RECORD(a2dp_sink_attrs);

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
				BT_SDP_ARRAY_16(0x0100U)
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
				BT_SDP_ARRAY_16(0x0103U)
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("A2DPSink"),
	BT_SDP_SUPPORTED_FEATURES(0x0001U),
};

static struct bt_sdp_record a2dp_source_rec = BT_SDP_RECORD(a2dp_source_attrs);

static void shell_a2dp_print_capabilities(struct bt_a2dp_ep_info *ep_info)
{
	uint8_t codec_type;
	uint8_t *codec_ie;
	uint16_t codec_ie_len;

	codec_type = ep_info->codec_type;
	codec_ie = ep_info->codec_cap.codec_ie;
	codec_ie_len = ep_info->codec_cap.len;
	bt_shell_print("endpoint id: %d, %s, %s:", ep_info->sep_info->id,
		       (ep_info->sep_info->tsep == BT_AVDTP_SINK) ? "(sink)" : "(source)",
		       (ep_info->sep_info->inuse) ? "(in use)" : "(idle)");
	if (BT_A2DP_SBC == codec_type) {
		bt_shell_print("  codec type: SBC");

		if (BT_A2DP_SBC_IE_LENGTH != codec_ie_len) {
			bt_shell_error("  wrong sbc codec ie");
			return;
		}

		bt_shell_print("  sample frequency:");
		if (0U != (codec_ie[0U] & A2DP_SBC_SAMP_FREQ_16000)) {
			bt_shell_print("	16000 ");
		}
		if (0U != (codec_ie[0U] & A2DP_SBC_SAMP_FREQ_32000)) {
			bt_shell_print("	32000 ");
		}
		if (0U != (codec_ie[0U] & A2DP_SBC_SAMP_FREQ_44100)) {
			bt_shell_print("	44100 ");
		}
		if (0U != (codec_ie[0U] & A2DP_SBC_SAMP_FREQ_48000)) {
			bt_shell_print("	48000");
		}

		bt_shell_print("  channel mode:");
		if (0U != (codec_ie[0U] & A2DP_SBC_CH_MODE_MONO)) {
			bt_shell_print("	Mono ");
		}
		if (0U != (codec_ie[0U] & A2DP_SBC_CH_MODE_DUAL)) {
			bt_shell_print("	Dual ");
		}
		if (0U != (codec_ie[0U] & A2DP_SBC_CH_MODE_STREO)) {
			bt_shell_print("	Stereo ");
		}
		if (0U != (codec_ie[0U] & A2DP_SBC_CH_MODE_JOINT)) {
			bt_shell_print("	Joint-Stereo");
		}

		/* Decode Support for Block Length */
		bt_shell_print("  Block Length:");
		if (0U != (codec_ie[1U] & A2DP_SBC_BLK_LEN_4)) {
			bt_shell_print("	4 ");
		}
		if (0U != (codec_ie[1U] & A2DP_SBC_BLK_LEN_8)) {
			bt_shell_print("	8 ");
		}
		if (0U != (codec_ie[1U] & A2DP_SBC_BLK_LEN_12)) {
			bt_shell_print("	12 ");
		}
		if (0U != (codec_ie[1U] & A2DP_SBC_BLK_LEN_16)) {
			bt_shell_print("	16");
		}

		/* Decode Support for Subbands */
		bt_shell_print("  Subbands:");
		if (0U != (codec_ie[1U] & A2DP_SBC_SUBBAND_4)) {
			bt_shell_print("	4 ");
		}
		if (0U != (codec_ie[1U] & A2DP_SBC_SUBBAND_8)) {
			bt_shell_print("	8");
		}

		/* Decode Support for Allocation Method */
		bt_shell_print("  Allocation Method:");
		if (0U != (codec_ie[1U] & A2DP_SBC_ALLOC_MTHD_SNR)) {
			bt_shell_print("	SNR ");
		}
		if (0U != (codec_ie[1U] & A2DP_SBC_ALLOC_MTHD_LOUDNESS)) {
			bt_shell_print("	Loudness");
		}

		bt_shell_print("  Bitpool Range: %d - %d",
			       codec_ie[2U], codec_ie[3U]);
	} else {
		bt_shell_print("  not SBC codecs");
	}
}

void app_connected(struct bt_a2dp *a2dp, int err)
{
	if (!err) {
		default_a2dp = a2dp;
		bt_shell_print("a2dp connected");
	} else {
		bt_shell_print("a2dp connecting fail");
	}
}

void app_disconnected(struct bt_a2dp *a2dp)
{
	found_peer_sbc_endpoint = NULL;
	bt_shell_print("a2dp disconnected");
}

int app_config_req(struct bt_a2dp *a2dp, struct bt_a2dp_ep *ep,
		struct bt_a2dp_codec_cfg *codec_cfg, struct bt_a2dp_stream **stream,
		uint8_t *rsp_err_code)
{
	uint32_t sample_rate;

	bt_a2dp_stream_cb_register(&sbc_stream, &stream_ops);
	*stream = &sbc_stream;
	*rsp_err_code = 0;

	bt_shell_print("receive requesting config and accept");
	sample_rate = bt_a2dp_sbc_get_sampling_frequency(
		(struct bt_a2dp_codec_sbc_params *)&codec_cfg->codec_config->codec_ie[0]);
	bt_shell_print("sample rate %dHz", sample_rate);

	return 0;
}

int app_reconfig_req(struct bt_a2dp_stream *stream,
	struct bt_a2dp_codec_cfg *codec_cfg, uint8_t *rsp_err_code)
{
	uint32_t sample_rate;

	*rsp_err_code = 0;
	bt_shell_print("receive requesting reconfig and accept");
	sample_rate = bt_a2dp_sbc_get_sampling_frequency(
		(struct bt_a2dp_codec_sbc_params *)&codec_cfg->codec_config->codec_ie[0]);
	bt_shell_print("sample rate %dHz", sample_rate);

	return 0;
}

void app_config_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
	if (rsp_err_code == 0) {
		bt_shell_print("success to configure");
	} else {
		bt_shell_print("fail to configure");
	}
}

int app_establish_req(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code)
{
	*rsp_err_code = 0;
	bt_shell_print("receive requesting establishment and accept");
	return 0;
}

void app_establish_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
	if (rsp_err_code == 0) {
		bt_shell_print("success to establish");
	} else {
		bt_shell_print("fail to establish");
	}
}

int app_release_req(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code)
{
	*rsp_err_code = 0;
	bt_shell_print("receive requesting release and accept");
	return 0;
}

void app_release_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
	if (rsp_err_code == 0) {
		bt_shell_print("success to release");
	} else {
		bt_shell_print("fail to release");
	}
}

int app_start_req(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code)
{
	*rsp_err_code = 0;
	bt_shell_print("receive requesting start and accept");
	return 0;
}

void app_start_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
	if (rsp_err_code == 0) {
		bt_shell_print("success to start");
	} else {
		bt_shell_print("fail to start");
	}
}

int app_suspend_req(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code)
{
	*rsp_err_code = 0;
	bt_shell_print("receive requesting suspend and accept");
	return 0;
}

void app_suspend_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
	if (rsp_err_code == 0) {
		bt_shell_print("success to suspend");
	} else {
		bt_shell_print("fail to suspend");
	}
}

void stream_configured(struct bt_a2dp_stream *stream)
{
	bt_shell_print("stream configured");
}

void stream_established(struct bt_a2dp_stream *stream)
{
	bt_shell_print("stream established");
}

void stream_released(struct bt_a2dp_stream *stream)
{
	bt_shell_print("stream released");
}

void stream_started(struct bt_a2dp_stream *stream)
{
	bt_shell_print("stream started");
}

void stream_suspended(struct bt_a2dp_stream *stream)
{
	bt_shell_print("stream suspended");
}

void stream_aborted(struct bt_a2dp_stream *stream)
{
	bt_shell_print("stream aborted");
}

void sink_sbc_streamer_data(struct bt_a2dp_stream *stream, struct net_buf *buf,
			uint16_t seq_num, uint32_t ts)
{
	uint8_t sbc_hdr;

	if (buf->len < 1U) {
		return;
	}
	sbc_hdr = net_buf_pull_u8(buf);
	bt_shell_print("received, num of frames: %d, data length:%d",
		       (uint8_t)BT_A2DP_SBC_MEDIA_HDR_NUM_FRAMES_GET(sbc_hdr), buf->len);
	bt_shell_print("data: %d, %d, %d, %d, %d, %d ......", buf->data[0],
		buf->data[1], buf->data[2], buf->data[3], buf->data[4], buf->data[5]);
}

void stream_recv(struct bt_a2dp_stream *stream,
		struct net_buf *buf, uint16_t seq_num, uint32_t ts)
{
	sink_sbc_streamer_data(stream, buf, seq_num, ts);
}

struct bt_a2dp_cb a2dp_cb = {
	.connected = app_connected,
	.disconnected = app_disconnected,
	.config_req = app_config_req,
	.config_rsp = app_config_rsp,
	.establish_req = app_establish_req,
	.establish_rsp = app_establish_rsp,
	.release_req = app_release_req,
	.release_rsp = app_release_rsp,
	.start_req = app_start_req,
	.start_rsp = app_start_rsp,
	.suspend_req = app_suspend_req,
	.suspend_rsp = app_suspend_rsp,
	.reconfig_req = app_reconfig_req,
};

static int cmd_register_cb(const struct shell *sh, int32_t argc, char *argv[])
{
	int err = -1;

	if (a2dp_initied == 0) {
		a2dp_initied = 1;

		err = bt_a2dp_register_cb(&a2dp_cb);
		if (!err) {
			shell_print(sh, "success");
		} else {
			shell_print(sh, "fail");
		}
	} else {
		shell_print(sh, "already registered");
	}

	return 0;
}

static int cmd_register_ep(const struct shell *sh, int32_t argc, char *argv[])
{
	int err = -1;
	const char *type;
	const char *action;

	if (a2dp_initied == 0) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	type = argv[1];
	action = argv[2];
	if (!strcmp(action, "sbc")) {
		if (!strcmp(type, "sink")) {
			if (a2dp_sink_sdp_registered == 0) {
				a2dp_sink_sdp_registered = 1;
				bt_sdp_register_service(&a2dp_sink_rec);
			}
			err = bt_a2dp_register_ep(&sink_sbc_endpoint,
				BT_AVDTP_AUDIO, BT_AVDTP_SINK);
			if (!err) {
				shell_print(sh, "SBC sink endpoint is registered");
				registered_sbc_endpoint = &sink_sbc_endpoint;
			}
		} else if (!strcmp(type, "source")) {
			if (a2dp_source_sdp_registered == 0) {
				a2dp_source_sdp_registered = 1;
				bt_sdp_register_service(&a2dp_source_rec);
			}
			err = bt_a2dp_register_ep(&source_sbc_endpoint,
				BT_AVDTP_AUDIO, BT_AVDTP_SOURCE);
			if (!err) {
				shell_print(sh, "SBC source endpoint is registered");
				registered_sbc_endpoint = &source_sbc_endpoint;
			}
		} else {
			shell_help(sh);
			return 0;
		}
	} else {
		shell_help(sh);
		return 0;
	}

	if (err) {
		shell_print(sh, "fail to register endpoint");
	}

	return 0;
}

static int cmd_connect(const struct shell *sh, int32_t argc, char *argv[])
{
	if (a2dp_initied == 0) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	default_a2dp = bt_a2dp_connect(default_conn);
	if (NULL == default_a2dp) {
		shell_error(sh, "fail to connect a2dp");
	}
	return 0;
}

static int cmd_disconnect(const struct shell *sh, int32_t argc, char *argv[])
{
	if (a2dp_initied == 0) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	if (default_a2dp != NULL) {
		bt_a2dp_disconnect(default_a2dp);
		default_a2dp = NULL;
	} else {
		shell_error(sh, "a2dp is not connected");
	}
	return 0;
}

void app_configured(int err)
{
	if (err) {
		bt_shell_print("configure fail");
	}
}

static struct bt_a2dp_stream_ops stream_ops = {
	.configured = stream_configured,
	.established = stream_established,
	.released = stream_released,
	.started = stream_started,
	.suspended = stream_suspended,
	.aborted = stream_aborted,
#if defined(CONFIG_BT_A2DP_SINK)
	.recv = stream_recv,
#endif
#if defined(CONFIG_BT_A2DP_SOURCE)
	.sent = NULL,
#endif
};

BT_A2DP_SBC_EP_CFG_DEFAULT(sbc_cfg_default, A2DP_SBC_SAMP_FREQ_44100);
static int cmd_configure(const struct shell *sh, int32_t argc, char *argv[])
{
	int err;

	if (a2dp_initied == 0) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	if (default_a2dp != NULL) {
		if (registered_sbc_endpoint == NULL) {
			shell_error(sh, "no endpoint");
			return 0;
		}

		if (found_peer_sbc_endpoint == NULL) {
			shell_error(sh, "don't find the peer sbc endpoint");
			return 0;
		}

		bt_a2dp_stream_cb_register(&sbc_stream, &stream_ops);

		err = bt_a2dp_stream_config(default_a2dp, &sbc_stream,
			registered_sbc_endpoint, found_peer_sbc_endpoint,
			&sbc_cfg_default);
		if (err) {
			shell_error(sh, "fail to configure");
		}
	} else {
		shell_error(sh, "a2dp is not connected");
	}
	return 0;
}

static int cmd_reconfigure(const struct shell *sh, int32_t argc, char *argv[])
{
	if (a2dp_initied == 0) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	if (bt_a2dp_stream_reconfig(&sbc_stream, &sbc_cfg_default) != 0) {
		shell_print(sh, "fail");
	}
	return 0;
}

static uint8_t bt_a2dp_discover_peer_endpoint_cb(struct bt_a2dp *a2dp,
		struct bt_a2dp_ep_info *info, struct bt_a2dp_ep **ep)
{
	if (info != NULL) {
		bt_shell_print("find one endpoint");
		shell_a2dp_print_capabilities(info);
		if ((info->codec_type == BT_A2DP_SBC) &&
		    (ep != NULL)) {
			*ep = &peer_sbc_endpoint;
			found_peer_sbc_endpoint = &peer_sbc_endpoint;
		}
	}
	return BT_A2DP_DISCOVER_EP_CONTINUE;
}

static struct bt_avdtp_sep_info found_seps[5];

struct bt_a2dp_discover_param discover_param = {
	.cb = bt_a2dp_discover_peer_endpoint_cb,
	.seps_info = &found_seps[0],
	.sep_count = 5,
};

static int cmd_get_peer_eps(const struct shell *sh, int32_t argc, char *argv[])
{
	if (a2dp_initied == 0) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	if (default_a2dp != NULL) {
		int err = bt_a2dp_discover(default_a2dp, &discover_param);

		if (err) {
			shell_error(sh, "discover fail");
		}
	} else {
		shell_error(sh, "a2dp is not connected");
	}
	return 0;
}

static int cmd_establish(const struct shell *sh, int32_t argc, char *argv[])
{
	if (a2dp_initied == 0) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	if (bt_a2dp_stream_establish(&sbc_stream) != 0) {
		shell_print(sh, "fail");
	}
	return 0;
}

static int cmd_release(const struct shell *sh, int32_t argc, char *argv[])
{
	if (a2dp_initied == 0) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	if (bt_a2dp_stream_release(&sbc_stream) != 0) {
		shell_print(sh, "fail");
	}
	return 0;
}

static int cmd_start(const struct shell *sh, int32_t argc, char *argv[])
{
	if (a2dp_initied == 0) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	if (bt_a2dp_stream_start(&sbc_stream) != 0) {
		shell_print(sh, "fail");
	}
	return 0;
}

static int cmd_suspend(const struct shell *sh, int32_t argc, char *argv[])
{
	if (a2dp_initied == 0) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	if (bt_a2dp_stream_suspend(&sbc_stream) != 0) {
		shell_print(sh, "fail");
	}
	return 0;
}

static int cmd_abort(const struct shell *sh, int32_t argc, char *argv[])
{
	if (a2dp_initied == 0) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	if (bt_a2dp_stream_abort(&sbc_stream) != 0) {
		shell_print(sh, "fail");
	}
	return 0;
}

static int cmd_send_media(const struct shell *sh, int32_t argc, char *argv[])
{
#if defined(CONFIG_BT_A2DP_SOURCE)
	struct net_buf *buf;
	int ret;

	if (a2dp_initied == 0) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	buf = net_buf_alloc(&a2dp_tx_pool, K_FOREVER);
	net_buf_reserve(buf, BT_A2DP_STREAM_BUF_RESERVE);

	/* num of frames is 1 */
	net_buf_add_u8(buf, (uint8_t)BT_A2DP_SBC_MEDIA_HDR_ENCODE(1, 0, 0, 0));
	net_buf_add_mem(buf, media_data, sizeof(media_data));
	shell_print(sh, "num of frames: %d, data length: %d", 1U, sizeof(media_data));
	shell_print(sh, "data: %d, %d, %d, %d, %d, %d ......", media_data[0],
		media_data[1], media_data[2], media_data[3], media_data[4], media_data[5]);

	ret = bt_a2dp_stream_send(&sbc_stream, buf, 0U, 0U);
	if (ret < 0) {
		printk("  Failed to send SBC audio data on streams(%d)\n", ret);
		net_buf_unref(buf);
	}
#endif
	return 0;
}

#define HELP_NONE "[none]"

SHELL_STATIC_SUBCMD_SET_CREATE(a2dp_cmds,
	SHELL_CMD_ARG(register_cb, NULL, "register a2dp connection callbacks",
			cmd_register_cb, 1, 0),
	SHELL_CMD_ARG(register_ep, NULL, "<type: sink or source> <value: sbc>",
			cmd_register_ep, 3, 0),
	SHELL_CMD_ARG(connect, NULL, HELP_NONE, cmd_connect, 1, 0),
	SHELL_CMD_ARG(disconnect, NULL, HELP_NONE, cmd_disconnect, 1, 0),
	SHELL_CMD_ARG(discover_peer_eps, NULL, HELP_NONE, cmd_get_peer_eps, 1, 0),
	SHELL_CMD_ARG(configure, NULL, "\"configure/enable the stream\"", cmd_configure, 1, 0),
	SHELL_CMD_ARG(establish, NULL, "\"establish the stream\"", cmd_establish, 1, 0),
	SHELL_CMD_ARG(reconfigure, NULL, "\"reconfigure the stream\"", cmd_reconfigure, 1, 0),
	SHELL_CMD_ARG(release, NULL, "\"release the stream\"", cmd_release, 1, 0),
	SHELL_CMD_ARG(start, NULL, "\"start the stream\"", cmd_start, 1, 0),
	SHELL_CMD_ARG(suspend, NULL, "\"suspend the stream\"", cmd_suspend, 1, 0),
	SHELL_CMD_ARG(abort, NULL, "\"abort the stream\"", cmd_abort, 1, 0),
	SHELL_CMD_ARG(send_media, NULL, HELP_NONE, cmd_send_media, 1, 0),
	SHELL_SUBCMD_SET_END
);

static int cmd_a2dp(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		/* sh returns 1 when help is printed */
		return 1;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(a2dp, &a2dp_cmds, "Bluetooth A2DP sh commands",
			   cmd_a2dp, 1, 1);
