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

#include "bt.h"

struct bt_a2dp *default_a2dp;
static uint8_t a2dp_sink_sdp_registered;
static uint8_t a2dp_source_sdp_registered;
static uint8_t a2dp_initied;
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
				BT_SDP_ARRAY_16(0X0100u) /* AVDTP version: 01 00 */
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
				BT_SDP_ARRAY_16(0X0100u)
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

BT_A2DP_SBC_SINK_ENDPOINT(sink_sbc_endpoint);
BT_A2DP_SBC_SOURCE_ENDPOINT(source_sbc_endpoint, A2DP_SBC_SAMP_FREQ_44100);

static void shell_a2dp_print_capabilities(struct bt_a2dp_endpoint *endpoint)
{
	uint8_t codec_type;
	uint8_t *codec_ie;
	uint16_t codec_ie_len;

	codec_type = endpoint->codec_id;
	codec_ie = endpoint->capabilities->codec_ie;
	codec_ie_len = endpoint->capabilities->len;
	shell_print(ctx_shell, "endpoint id: %d, %s, %s:", endpoint->info.sep.id,
			(endpoint->info.sep.tsep == BT_A2DP_SINK) ? "(sink)" : "(source)",
			(endpoint->info.sep.inuse) ? "(in use)" : "(idle)");
	if (BT_A2DP_SBC == codec_type) {
		shell_print(ctx_shell, "  codec type: SBC");

		if (BT_A2DP_SBC_IE_LENGTH != codec_ie_len) {
			shell_error(ctx_shell, "  wrong sbc codec ie");
			return;
		}

		shell_print(ctx_shell, "  sample frequency:");
		if (0U != (codec_ie[0U] & A2DP_SBC_SAMP_FREQ_16000)) {
			shell_print(ctx_shell, "	16000 ");
		}
		if (0U != (codec_ie[0U] & A2DP_SBC_SAMP_FREQ_32000)) {
			shell_print(ctx_shell, "	32000 ");
		}
		if (0U != (codec_ie[0U] & A2DP_SBC_SAMP_FREQ_44100)) {
			shell_print(ctx_shell, "	44100 ");
		}
		if (0U != (codec_ie[0U] & A2DP_SBC_SAMP_FREQ_48000)) {
			shell_print(ctx_shell, "	48000");
		}

		shell_print(ctx_shell, "  channel mode:");
		if (0U != (codec_ie[0U] & A2DP_SBC_CH_MODE_MONO)) {
			shell_print(ctx_shell, "	Mono ");
		}
		if (0U != (codec_ie[0U] & A2DP_SBC_CH_MODE_DUAL)) {
			shell_print(ctx_shell, "	Dual ");
		}
		if (0U != (codec_ie[0U] & A2DP_SBC_CH_MODE_STREO)) {
			shell_print(ctx_shell, "	Stereo ");
		}
		if (0U != (codec_ie[0U] & A2DP_SBC_CH_MODE_JOINT)) {
			shell_print(ctx_shell, "	Joint-Stereo");
		}

		 /* Decode Support for Block Length */
		shell_print(ctx_shell, "  Block Length:");
		if (0U != (codec_ie[1U] & A2DP_SBC_BLK_LEN_4)) {
			shell_print(ctx_shell, "	4 ");
		}
		if (0U != (codec_ie[1U] & A2DP_SBC_BLK_LEN_8)) {
			shell_print(ctx_shell, "	8 ");
		}
		if (0U != (codec_ie[1U] & A2DP_SBC_BLK_LEN_12)) {
			shell_print(ctx_shell, "	12 ");
		}
		if (0U != (codec_ie[1U] & A2DP_SBC_BLK_LEN_16)) {
			shell_print(ctx_shell, "	16");
		}

		/* Decode Support for Subbands */
		shell_print(ctx_shell, "  Subbands:");
		if (0U != (codec_ie[1U] & A2DP_SBC_SUBBAND_4)) {
			shell_print(ctx_shell, "	4 ");
		}
		if (0U != (codec_ie[1U] & A2DP_SBC_SUBBAND_8)) {
			shell_print(ctx_shell, "	8");
		}

		/* Decode Support for Allocation Method */
		shell_print(ctx_shell, "  Allocation Method:");
		if (0U != (codec_ie[1U] & A2DP_SBC_ALLOC_MTHD_SNR)) {
			shell_print(ctx_shell, "	SNR ");
		}
		if (0U != (codec_ie[1U] & A2DP_SBC_ALLOC_MTHD_LOUDNESS)) {
			shell_print(ctx_shell, "	Loudness");
		}

		shell_print(ctx_shell, "  Bitpool Range: %d - %d",
					codec_ie[2U], codec_ie[3U]);
	} else {
		shell_print(ctx_shell, "  not SBC codecs");
	}
}

void app_connected(struct bt_a2dp *a2dp, int err)
{
	if (!err) {
		default_a2dp = a2dp;
		shell_print(ctx_shell, "a2dp connected");
	} else {
		shell_print(ctx_shell, "a2dp connecting fail");
	}
}

void app_disconnected(struct bt_a2dp *a2dp)
{
	shell_print(ctx_shell, "a2dp disconnected");
}

void sink_sbc_streamer_data(struct net_buf *buf, uint16_t seq_num, uint32_t ts)
{
	struct bt_a2dp_codec_sbc_media_packet_hdr *sbc_hdr;

	sbc_hdr = net_buf_pull_mem(buf, sizeof(*sbc_hdr));
	shell_print(ctx_shell, "received, num of frames: %d, data length:%d",
		sbc_hdr->number_of_sbc_frames, buf->len);
	shell_print(ctx_shell, "data: %d, %d, %d, %d, %d, %d ......", buf->data[0],
		buf->data[1], buf->data[2], buf->data[3], buf->data[4], buf->data[5]);
}

void common_configured(struct bt_a2dp_endpoint_configure_result *configResult)
{
	if (configResult->err == 0) {
		uint32_t sample_rate;

		shell_print(ctx_shell, "SBC configure success");
		sample_rate  = bt_a2dp_sbc_get_sampling_frequency(
					&configResult->config.media_config->codec_ie[0]);
		shell_print(ctx_shell, "sample rate %dHz", sample_rate);
	} else {
		shell_print(ctx_shell, "configure err");
	}
}

void source_sbc_configured(struct bt_a2dp_endpoint_configure_result *configResult)
{
	common_configured(configResult);
}

void sink_sbc_configured(struct bt_a2dp_endpoint_configure_result *configResult)
{
	common_configured(configResult);
}

void sbc_src_deconfigured(int err)
{
	shell_print(ctx_shell, "deconfigured");
}

void sbc_snk_deconfigured(int err)
{
	shell_print(ctx_shell, "deconfigured");
}

void sbc_src_start_play(int err)
{
	if (err == 0) {
		shell_print(ctx_shell, "a2dp start playing");
	} else {
		shell_print(ctx_shell, "a2dp start fail");
	}
}

void sbc_snk_start_play(int err)
{
	if (err == 0) {
		shell_print(ctx_shell, "a2dp start playing");
	} else {
		shell_print(ctx_shell, "a2dp start fail");
	}
}

void sbc_src_stop_play(int err)
{
	if (err == 0) {
		shell_print(ctx_shell, "a2dp stop playing");
	} else {
		shell_print(ctx_shell, "a2dp stop fail");
	}
}

void sbc_snk_stop_play(int err)
{
	if (err == 0) {
		shell_print(ctx_shell, "a2dp stop playing");
	} else {
		shell_print(ctx_shell, "a2dp stop fail");
	}
}

static int cmd_register_cb(const struct shell *sh, int32_t argc, char *argv[])
{
	int err = -1;

	if (a2dp_initied == 0) {
		struct bt_a2dp_connect_cb cb;

		a2dp_initied = 1;
		cb.connected = app_connected;
		cb.disconnected = app_disconnected;

		err = bt_a2dp_register_connect_callback(&cb);
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
			sink_sbc_endpoint.control_cbs.configured = sink_sbc_configured;
			sink_sbc_endpoint.control_cbs.deconfigured = sbc_snk_deconfigured;
			sink_sbc_endpoint.control_cbs.start_play = sbc_snk_start_play;
			sink_sbc_endpoint.control_cbs.stop_play = sbc_snk_stop_play;
			sink_sbc_endpoint.control_cbs.sink_streamer_data = sink_sbc_streamer_data;
			err = bt_a2dp_register_endpoint(&sink_sbc_endpoint,
				BT_A2DP_AUDIO, BT_A2DP_SINK);
			if (!err) {
				shell_print(sh, "SBC sink endpoint is registered");
			}
		} else if (!strcmp(type, "source")) {
			if (a2dp_source_sdp_registered == 0) {
				a2dp_source_sdp_registered = 1;
				bt_sdp_register_service(&a2dp_source_rec);
			}
			source_sbc_endpoint.control_cbs.configured = source_sbc_configured;
			source_sbc_endpoint.control_cbs.deconfigured = sbc_src_deconfigured;
			source_sbc_endpoint.control_cbs.start_play = sbc_src_start_play;
			source_sbc_endpoint.control_cbs.stop_play = sbc_src_stop_play;
			err = bt_a2dp_register_endpoint(&source_sbc_endpoint, BT_A2DP_AUDIO,
				BT_A2DP_SOURCE);
			if (!err) {
				shell_print(sh, "SBC source endpoint is registered");
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
		shell_print(ctx_shell, "configure fail");
	}
}

static int cmd_configure(const struct shell *sh, int32_t argc, char *argv[])
{
	if (a2dp_initied == 0) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	if (default_a2dp != NULL) {
		bt_a2dp_configure(default_a2dp, app_configured);
	} else {
		shell_error(sh, "a2dp is not connected");
	}
	return 0;
}

static uint8_t bt_a2dp_discover_peer_endpoint_cb(struct bt_a2dp *a2dp,
					struct bt_a2dp_endpoint *endpoint, int err)
{
	if ((!err) && (endpoint != NULL)) {
		shell_a2dp_print_capabilities(endpoint);
		return BT_A2DP_DISCOVER_ENDPOINT_CONTINUE;
	}

	shell_error(ctx_shell, "discover done");
	return BT_A2DP_DISCOVER_ENDPOINT_STOP;
}

static int cmd_get_peer_eps(const struct shell *sh, int32_t argc, char *argv[])
{
	if (a2dp_initied == 0) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	if (default_a2dp != NULL) {
		int err = bt_a2dp_discover_peer_endpoints(default_a2dp,
				bt_a2dp_discover_peer_endpoint_cb);

		if (err) {
			shell_error(sh, "discover fail");
		}
	} else {
		shell_error(sh, "a2dp is not connected");
	}
	return 0;
}

static int cmd_start(const struct shell *sh, int32_t argc, char *argv[])
{
	if (a2dp_initied == 0) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	if (bt_a2dp_start(&source_sbc_endpoint) != 0) {
		shell_print(sh, "fail");
	}
	return 0;
}

static int cmd_send_media(const struct shell *sh, int32_t argc, char *argv[])
{
	struct net_buf *buf;
	struct bt_a2dp_codec_sbc_media_packet_hdr *sbc_hdr;
	int ret;

	if (a2dp_initied == 0) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	buf = bt_a2dp_media_buf_alloc(NULL);
	if (buf == NULL) {
		shell_print(sh, "fail");
		return -ENOEXEC;
	}

	sbc_hdr = net_buf_add(buf, sizeof(struct bt_a2dp_codec_sbc_media_packet_hdr));
	memset(sbc_hdr, 0, sizeof(struct bt_a2dp_codec_sbc_media_packet_hdr));
	sbc_hdr->number_of_sbc_frames = 1;
	net_buf_add_mem(buf, media_data, sizeof(media_data));
	shell_print(sh, "num of frames: %d, data length: %d", 1u, sizeof(media_data));
	shell_print(sh, "data: %d, %d, %d, %d, %d, %d ......", media_data[0],
		media_data[1], media_data[2], media_data[3], media_data[4], media_data[5]);

	ret = bt_a2dp_media_send(&source_sbc_endpoint, buf, 0u, 0u);
	if (ret < 0) {
		printk("  Failed to send SBC audio data on streams(%d)\n", ret);
		net_buf_unref(buf);
	}

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
	SHELL_CMD_ARG(configure, NULL, HELP_NONE, cmd_configure, 1, 0),
	SHELL_CMD_ARG(discover_peer_eps, NULL, HELP_NONE, cmd_get_peer_eps, 1, 0),
	SHELL_CMD_ARG(start, NULL, "\"start the default selected ep\"", cmd_start, 1, 0),
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
