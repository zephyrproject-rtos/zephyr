/*
 * Copyright 2025 NXP
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

static struct bt_a2dp *default_a2dp;
static uint8_t role;
static bool a2dp_initied;
static struct bt_a2dp_codec_ie peer_sbc_capabilities;
static struct bt_a2dp_ep peer_sbc_endpoint = {
	.codec_cap = &peer_sbc_capabilities,
};
#define A2DP_SERVICE_LEN 512
NET_BUF_POOL_FIXED_DEFINE(find_avdtp_version_pool, 1, A2DP_SERVICE_LEN,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static const struct bt_uuid *a2dp_snk_uuid = BT_UUID_DECLARE_16(BT_SDP_AUDIO_SINK_SVCLASS);
static const struct bt_uuid *a2dp_src_uuid = BT_UUID_DECLARE_16(BT_SDP_AUDIO_SOURCE_SVCLASS);
static struct bt_sdp_discover_params discov_a2dp = {
	.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR,
	.pool = &find_avdtp_version_pool,
};
static struct bt_a2dp_ep *found_peer_sbc_endpoint;
static struct bt_a2dp_ep *registered_sbc_endpoint;
static struct bt_a2dp_stream sbc_stream;
static struct bt_a2dp_stream_ops stream_ops;

#if defined(CONFIG_BT_A2DP_SOURCE)
static struct k_work_delayable send_media;
NET_BUF_POOL_DEFINE(a2dp_tx_media_pool, CONFIG_BT_MAX_CONN,
		    BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU), CONFIG_BT_CONN_TX_USER_DATA_SIZE,
		    NULL);

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

static void a2dp_send_media_timeout(struct k_work *work)
{
	struct net_buf *buf;
	int ret;

	buf = bt_a2dp_stream_create_pdu(&a2dp_tx_media_pool, K_FOREVER);
	if (buf == NULL) {
		bt_shell_print("fail to allocate buffer");
		return;
	}

	/* num of frames is 1 */
	net_buf_add_u8(buf, (uint8_t)BT_A2DP_SBC_MEDIA_HDR_ENCODE(1, 0, 0, 0));
	net_buf_add_mem(buf, media_data, sizeof(media_data));
	ret = bt_a2dp_stream_send(&sbc_stream, buf, 0U, 0U);
	if (ret < 0) {
		net_buf_unref(buf);
		return;
	}
	k_work_schedule(&send_media, K_MSEC(1000));
}
#endif

static bool a2dp_source_sdp_registered;
BT_A2DP_SBC_SOURCE_EP_DEFAULT(source_sbc_endpoint);
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
	BT_SDP_SERVICE_NAME("A2DPSource"),
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
		bt_shell_print(" codec type: SBC");

		if (BT_A2DP_SBC_IE_LENGTH != codec_ie_len) {
			bt_shell_error(" wrong sbc codec ie");
			return;
		}

		bt_shell_print(" sample frequency:");
		if (0U != (codec_ie[0U] & A2DP_SBC_SAMP_FREQ_16000)) {
			bt_shell_print(" 16000 ");
		}
		if (0U != (codec_ie[0U] & A2DP_SBC_SAMP_FREQ_32000)) {
			bt_shell_print(" 32000 ");
		}
		if (0U != (codec_ie[0U] & A2DP_SBC_SAMP_FREQ_44100)) {
			bt_shell_print(" 44100 ");
		}
		if (0U != (codec_ie[0U] & A2DP_SBC_SAMP_FREQ_48000)) {
			bt_shell_print(" 48000");
		}

		bt_shell_print("  channel mode:");
		if (0U != (codec_ie[0U] & A2DP_SBC_CH_MODE_MONO)) {
			bt_shell_print(" Mono ");
		}
		if (0U != (codec_ie[0U] & A2DP_SBC_CH_MODE_DUAL)) {
			bt_shell_print(" Dual ");
		}
		if (0U != (codec_ie[0U] & A2DP_SBC_CH_MODE_STEREO)) {
			bt_shell_print(" Stereo ");
		}
		if (0U != (codec_ie[0U] & A2DP_SBC_CH_MODE_JOINT)) {
			bt_shell_print(" Joint-Stereo");
		}
		bt_shell_print(" Block Length:");
		if (0U != (codec_ie[1U] & A2DP_SBC_BLK_LEN_4)) {
			bt_shell_print(" 4 ");
		}
		if (0U != (codec_ie[1U] & A2DP_SBC_BLK_LEN_8)) {
			bt_shell_print(" 8 ");
		}
		if (0U != (codec_ie[1U] & A2DP_SBC_BLK_LEN_12)) {
			bt_shell_print(" 12 ");
		}
		if (0U != (codec_ie[1U] & A2DP_SBC_BLK_LEN_16)) {
			bt_shell_print(" 16");
		}

		/* Decode Support for Subbands */
		bt_shell_print("  Subbands:");
		if (0U != (codec_ie[1U] & A2DP_SBC_SUBBAND_4)) {
			bt_shell_print(" 4 ");
		}
		if (0U != (codec_ie[1U] & A2DP_SBC_SUBBAND_8)) {
			bt_shell_print(" 8");
		}

		/* Decode Support for Allocation Method */
		bt_shell_print("  Allocation Method:");
		if (0U != (codec_ie[1U] & A2DP_SBC_ALLOC_MTHD_SNR)) {
			bt_shell_print(" SNR ");
		}
		if (0U != (codec_ie[1U] & A2DP_SBC_ALLOC_MTHD_LOUDNESS)) {
			bt_shell_print(" Loudness");
		}

		bt_shell_print("  Bitpool Range: %d - %d", codec_ie[2U], codec_ie[3U]);
	} else {
		bt_shell_print("  not SBC codecs");
	}
}

static void app_connected(struct bt_a2dp *a2dp, int err)
{
	if (err == 0) {
		default_a2dp = a2dp;
		bt_shell_print("a2dp connected");
#if defined(CONFIG_BT_A2DP_SOURCE)
		if (role == BT_AVDTP_SOURCE) {
			k_work_init_delayable(&send_media, a2dp_send_media_timeout);
		}
#endif
	} else {
		bt_shell_print("a2dp connecting fail");
	}
}

static void app_disconnected(struct bt_a2dp *a2dp)
{
	found_peer_sbc_endpoint = NULL;
	default_a2dp = NULL;
	bt_shell_print("a2dp disconnected");
#if defined(CONFIG_BT_A2DP_SOURCE)
	if (role == BT_AVDTP_SOURCE) {
		k_work_cancel_delayable(&send_media);
	}
#endif
}

static int app_config_req(struct bt_a2dp *a2dp, struct bt_a2dp_ep *ep,
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

static int app_reconfig_req(struct bt_a2dp_stream *stream, struct bt_a2dp_codec_cfg *codec_cfg,
			    uint8_t *rsp_err_code)
{
	uint32_t sample_rate;

	*rsp_err_code = 0;
	bt_shell_print("receive requesting reconfig and accept");
	sample_rate = bt_a2dp_sbc_get_sampling_frequency(
		(struct bt_a2dp_codec_sbc_params *)&codec_cfg->codec_config->codec_ie[0]);
	bt_shell_print("sample rate %dHz", sample_rate);

	return 0;
}

static void app_config_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
	if (rsp_err_code == 0) {
		bt_shell_print("success to configure");
	} else {
		bt_shell_print("fail to configure");
	}
}

static int app_establish_req(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code)
{
	*rsp_err_code = 0;
	bt_shell_print("receive requesting establishment and accept");
	return 0;
}

static void app_establish_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
	if (rsp_err_code == 0) {
		bt_shell_print("success to establish");
	} else {
		bt_shell_print("fail to establish");
	}
}

static int app_release_req(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code)
{
	*rsp_err_code = 0;
	bt_shell_print("receive requesting release and accept");
#if defined(CONFIG_BT_A2DP_SOURCE)
	if (role == BT_AVDTP_SOURCE) {
		k_work_cancel_delayable(&send_media);
	}
#endif
	return 0;
}

static void app_release_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
	if (rsp_err_code == 0) {
		bt_shell_print("success to release");
#if defined(CONFIG_BT_A2DP_SOURCE)
		if (role == BT_AVDTP_SOURCE) {
			k_work_cancel_delayable(&send_media);
		}
#endif
	} else {
		bt_shell_print("fail to release");
	}
}

static int app_start_req(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code)
{
	*rsp_err_code = 0;
	bt_shell_print("receive requesting start and accept");
#if defined(CONFIG_BT_A2DP_SOURCE)
	if (role == BT_AVDTP_SOURCE) {
		k_work_schedule(&send_media, K_MSEC(1000));
	}
#endif
	return 0;
}

static void app_start_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
	if (rsp_err_code == 0) {
		bt_shell_print("success to start");
	} else {
		bt_shell_print("fail to start");
	}
}

static int app_suspend_req(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code)
{
	*rsp_err_code = 0;
	bt_shell_print("receive requesting suspend and accept");
#if defined(CONFIG_BT_A2DP_SOURCE)
	if (role == BT_AVDTP_SOURCE) {
		k_work_cancel_delayable(&send_media);
	}
#endif
	return 0;
}

static void app_suspend_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
	if (rsp_err_code == 0) {
		bt_shell_print("success to suspend");
#if defined(CONFIG_BT_A2DP_SOURCE)
		if (role == BT_AVDTP_SOURCE) {
			k_work_cancel_delayable(&send_media);
		}
#endif
	} else {
		bt_shell_print("fail to suspend");
	}
}

static void stream_configured(struct bt_a2dp_stream *stream)
{
	bt_shell_print("stream configured");
}

static void stream_established(struct bt_a2dp_stream *stream)
{
	bt_shell_print("stream established");
}

static void stream_released(struct bt_a2dp_stream *stream)
{
	bt_shell_print("stream released");
}

static void stream_started(struct bt_a2dp_stream *stream)
{
	bt_shell_print("stream started");
}

static void stream_suspended(struct bt_a2dp_stream *stream)
{
	bt_shell_print("stream suspended");
}

#if defined(CONFIG_BT_A2DP_SINK)
static void sink_sbc_streamer_data(struct bt_a2dp_stream *stream, struct net_buf *buf,
				   uint16_t seq_num, uint32_t ts)
{
	uint8_t sbc_hdr;

	if (buf->len < 7U) {
		return;
	}
	sbc_hdr = net_buf_pull_u8(buf);
	bt_shell_print("received, num of frames: %d, data length:%d",
		       (uint8_t)BT_A2DP_SBC_MEDIA_HDR_NUM_FRAMES_GET(sbc_hdr), buf->len);
	bt_shell_print("data: %d, %d, %d, %d, %d, %d ......", buf->data[0], buf->data[1],
		       buf->data[2], buf->data[3], buf->data[4], buf->data[5]);
}

static void stream_recv(struct bt_a2dp_stream *stream, struct net_buf *buf, uint16_t seq_num,
			uint32_t ts)
{
	sink_sbc_streamer_data(stream, buf, seq_num, ts);
}
#endif

static struct bt_a2dp_cb a2dp_cb = {
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

	if (a2dp_initied == false) {
		a2dp_initied = true;

		err = bt_a2dp_register_cb(&a2dp_cb);
		if (err == 0) {
			shell_print(sh, "success");
		} else {
			shell_print(sh, "fail");
		}
	} else {
		shell_print(sh, "already registered");
	}

	return err;
}

static int cmd_register_ep(const struct shell *sh, int32_t argc, char *argv[])
{
	int err = -1;
	const char *type;
	const char *codec;

	if (a2dp_initied == false) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	type = argv[1];
	codec = argv[2];

	if (strcmp(codec, "sbc") != 0) {
		shell_help(sh);
		return 0;
	}

	if (strcmp(type, "source") != 0) {
		shell_error(sh, "Unsupported endpoint type %s", type);
		shell_help(sh);
		return -EINVAL;
	}

	if (!IS_ENABLED(CONFIG_BT_A2DP_SOURCE)) {
		shell_error(sh, "CONFIG_BT_A2DP_SOURCE is not enabled");
		return -ENOTSUP;
	}

	if (a2dp_source_sdp_registered == false) {
		a2dp_source_sdp_registered = true;
		bt_sdp_register_service(&a2dp_source_rec);
	}

	err = bt_a2dp_register_ep(&source_sbc_endpoint, BT_AVDTP_AUDIO, BT_AVDTP_SOURCE);
	if (err != 0) {
		shell_error(sh, "fail to register endpoint");
		return err;
	}

	registered_sbc_endpoint = &source_sbc_endpoint;
	shell_print(sh, "SBC source endpoint is registered");

	return 0;
}

static int cmd_connect(const struct shell *sh, int32_t argc, char *argv[])
{
	if (a2dp_initied == false) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	default_a2dp = bt_a2dp_connect(default_conn);
	if (default_a2dp == NULL) {
		shell_error(sh, "fail to connect a2dp");
		return -EINVAL;
	}
	return 0;
}

static int cmd_disconnect(const struct shell *sh, int32_t argc, char *argv[])
{
	int err;

	if (a2dp_initied == false) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	if (default_a2dp == NULL) {
		shell_error(sh, "a2dp is not connected");
		return -ENOEXEC;
	}

	err = bt_a2dp_disconnect(default_a2dp);
	if (err == 0) {
		default_a2dp = NULL;
	} else {
		shell_print(sh, "fail to send disconnect cmd");
	}
	return err;
}

static struct bt_a2dp_stream_ops stream_ops = {
	.configured = stream_configured,
	.established = stream_established,
	.released = stream_released,
	.started = stream_started,
	.suspended = stream_suspended,
#if defined(CONFIG_BT_A2DP_SINK)
	.recv = stream_recv,
#endif
};

BT_A2DP_SBC_EP_CFG_DEFAULT(sbc_cfg, A2DP_SBC_SAMP_FREQ_44100);
static int cmd_configure(const struct shell *sh, int32_t argc, char *argv[])
{
	int err;

	if (a2dp_initied == false) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	if (a2dp_initied == false) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	if (default_a2dp == NULL) {
		shell_error(sh, "a2dp is not connected");
		return -ENOEXEC;
	}
	if (registered_sbc_endpoint == NULL) {
		shell_error(sh, "no endpoint");
		return -ENOEXEC;
	}
	if (found_peer_sbc_endpoint == NULL) {
		shell_error(sh, "don't find the peer sbc endpoint");
		return -ENOEXEC;
	}
	bt_a2dp_stream_cb_register(&sbc_stream, &stream_ops);
	err = bt_a2dp_stream_config(default_a2dp, &sbc_stream, registered_sbc_endpoint,
				    found_peer_sbc_endpoint, &sbc_cfg);
	if (err != 0) {
		shell_error(sh, "fail to configure (err %d)", err);
		return -ENOEXEC;
	}
	return 0;
}

static int cmd_reconfigure(const struct shell *sh, int32_t argc, char *argv[])
{
	if (a2dp_initied == false) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	if (bt_a2dp_stream_reconfig(&sbc_stream, &sbc_cfg) != 0) {
		shell_print(sh, "fail");
		return -EINVAL;
	}
	return 0;
}

static uint8_t bt_a2dp_discover_peer_endpoint_cb(struct bt_a2dp *a2dp, struct bt_a2dp_ep_info *info,
						 struct bt_a2dp_ep **ep)
{
	if (info != NULL) {
		bt_shell_print("find one endpoint");
		shell_a2dp_print_capabilities(info);
		if ((info->codec_type == BT_A2DP_SBC) && (ep != NULL)) {
			*ep = &peer_sbc_endpoint;
			found_peer_sbc_endpoint = &peer_sbc_endpoint;
		}
	}
	return BT_A2DP_DISCOVER_EP_CONTINUE;
}

static struct bt_avdtp_sep_info found_seps[5];

static struct bt_a2dp_discover_param discover_param = {
	.cb = bt_a2dp_discover_peer_endpoint_cb,
	.seps_info = &found_seps[0],
	.sep_count = ARRAY_SIZE(found_seps),
};

static uint8_t a2dp_sdp_discover_cb(struct bt_conn *conn, struct bt_sdp_client_result *result,
				    const struct bt_sdp_discover_params *params)
{
	int err;
	uint16_t peer_avdtp_version = AVDTP_VERSION_1_3;

	if ((result == NULL) || (result->resp_buf == NULL) || (result->resp_buf->len == 0)) {
		bt_shell_error("SDP discover nothing");
		return BT_SDP_DISCOVER_UUID_STOP;
	}

	err = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_AVDTP, &peer_avdtp_version);
	if (err != 0) {
		bt_shell_error("fail to get avdtp version");
		return BT_SDP_DISCOVER_UUID_CONTINUE;
	}
	if (default_a2dp != NULL) {
		discover_param.avdtp_version = peer_avdtp_version;
		err = bt_a2dp_discover(default_a2dp, &discover_param);
		if (err != 0) {
			bt_shell_error("fail to discover peer endpoints");
		}
	}

	return BT_SDP_DISCOVER_UUID_STOP;
}

static int cmd_get_peer_eps(const struct shell *sh, int32_t argc, char *argv[])
{
	int err = 0;

	if (role == BT_AVDTP_SOURCE) {
		discov_a2dp.uuid = a2dp_snk_uuid;
	} else if (role == BT_AVDTP_SINK) {
		discov_a2dp.uuid = a2dp_src_uuid;
	} else {
		return -EINVAL;
	}

	discov_a2dp.func = a2dp_sdp_discover_cb;

	err = bt_sdp_discover(default_conn, &discov_a2dp);
	if (err != 0) {
		shell_error(sh, "SDP discover failed (err %d)", err);
	}
	return err;
}

static int cmd_establish(const struct shell *sh, int32_t argc, char *argv[])
{
	if (a2dp_initied == false) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	if (bt_a2dp_stream_establish(&sbc_stream) != 0) {
		shell_print(sh, "fail");
		return -EINVAL;
	}
	return 0;
}

static int cmd_release(const struct shell *sh, int32_t argc, char *argv[])
{
	if (a2dp_initied == false) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	if (bt_a2dp_stream_release(&sbc_stream) != 0) {
		shell_print(sh, "fail");
		return -EINVAL;
	}
	return 0;
}

static int cmd_start(const struct shell *sh, int32_t argc, char *argv[])
{
	if (a2dp_initied == false) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	if (bt_a2dp_stream_start(&sbc_stream) != 0) {
		shell_print(sh, "fail");
		return -EINVAL;
	}
	return 0;
}

static int cmd_suspend(const struct shell *sh, int32_t argc, char *argv[])
{
	if (a2dp_initied == false) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	if (bt_a2dp_stream_suspend(&sbc_stream) != 0) {
		shell_print(sh, "fail");
		return -EINVAL;
	}
	return 0;
}

static int cmd_abort(const struct shell *sh, int32_t argc, char *argv[])
{
	if (a2dp_initied == false) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	if (bt_a2dp_stream_abort(&sbc_stream) != 0) {
		shell_print(sh, "fail");
		return -EINVAL;
	}
	return 0;
}

#define HELP_NONE "[none]"

static int cmd_a2dp_source(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		/* sh returns 1 when help is printed */
		return 1;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	a2dp_source_cmds,
	SHELL_CMD_ARG(register_cb, NULL, "register a2dp connection callbacks", cmd_register_cb, 1,
		      0),
	SHELL_CMD_ARG(register_ep, NULL, "<source> <sbc>", cmd_register_ep, 3, 0),
	SHELL_CMD_ARG(connect, NULL, HELP_NONE, cmd_connect, 1, 0),
	SHELL_CMD_ARG(disconnect, NULL, HELP_NONE, cmd_disconnect, 1, 0),
	SHELL_CMD_ARG(discover_peer_eps, NULL, "[avdtp version value]", cmd_get_peer_eps, 1, 1),
	SHELL_CMD_ARG(configure, NULL, HELP_NONE, cmd_configure, 1, 0),
	SHELL_CMD_ARG(establish, NULL, HELP_NONE, cmd_establish, 1, 0),
	SHELL_CMD_ARG(reconfigure, NULL, HELP_NONE, cmd_reconfigure, 1, 0),
	SHELL_CMD_ARG(release, NULL, HELP_NONE, cmd_release, 1, 0),
	SHELL_CMD_ARG(start, NULL, HELP_NONE, cmd_start, 1, 0),
	SHELL_CMD_ARG(suspend, NULL, HELP_NONE, cmd_suspend, 1, 0),
	SHELL_CMD_ARG(abort, NULL, HELP_NONE, cmd_abort, 1, 0), SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(a2dp_source, &a2dp_source_cmds, "Bluetooth test A2DP Source sh commands",
		       cmd_a2dp_source, 1, 1);
