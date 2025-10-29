/*
 * Copyright 2024-2025 NXP
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
#if defined(CONFIG_BT_A2DP_SOURCE_SBC_AUDIO_SHELL)
#include <zephyr/bluetooth/sbc.h>
#include "sine.h"
#endif
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

#if defined(CONFIG_BT_A2DP_SOURCE_SBC_AUDIO_SHELL)
/* sbc audio stream control variables */
static int64_t ref_time;
static uint32_t a2dp_src_missed_count;
static volatile bool a2dp_src_playback;
static volatile int media_index;
static uint32_t a2dp_src_sf;
static uint8_t a2dp_src_nc;
static uint32_t send_samples_count;
static uint16_t send_count;
/* 20ms max packet pcm data size. the max is 480 * 2 * 2 * 2 */
static uint8_t a2dp_pcm_buffer[480 * 2 * 2 * 2];
struct sbc_encoder encoder;
#endif /* CONFIG_BT_A2DP_SOURCE_SBC_AUDIO_SHELL */
#if defined(CONFIG_BT_A2DP_SOURCE)
#if !defined(CONFIG_BT_A2DP_SOURCE_SBC_AUDIO_SHELL)
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
#endif

BT_A2DP_SBC_EP_CFG_DEFAULT(sbc_cfg_default, A2DP_SBC_SAMP_FREQ_44100);
NET_BUF_POOL_DEFINE(a2dp_tx_pool, CONFIG_BT_MAX_CONN,
		BT_L2CAP_BUF_SIZE(CONFIG_BT_A2DP_SOURCE_DATA_BUF_SIZE),
		CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
#if defined(CONFIG_BT_A2DP_SOURCE_SBC_AUDIO_SHELL)
static void a2dp_playback_timeout_handler(struct k_timer *timer);
K_TIMER_DEFINE(a2dp_player_timer, a2dp_playback_timeout_handler, NULL);
#endif

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
				BT_SDP_ARRAY_16(AVDTP_VERSION) /* AVDTP version: 01 03 */
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
				BT_SDP_ARRAY_16(0x0103U)
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("A2DPSource"),
	BT_SDP_SUPPORTED_FEATURES(0x0001U),
};

#if defined(CONFIG_BT_A2DP_SOURCE_SBC_AUDIO_SHELL)
static uint8_t *a2dp_produce_media(uint32_t samples_num)
{
	uint8_t *media = NULL;
	uint16_t  medialen;

	/* Music Audio is Stereo */
	medialen = (samples_num * a2dp_src_nc * 2);

	/* For mono or dual configuration, skip alternative samples */
	if (1 == a2dp_src_nc) {
		uint16_t index;

		media = (uint8_t *)&a2dp_pcm_buffer[0];

		for (index = 0; index < samples_num; index++) {
			media[(2 * index)] = *((uint8_t *)media_data + media_index);
			media[(2 * index) + 1] = *((uint8_t *)media_data + media_index + 1);
			/* Update the tone index */
			media_index += 4u;
			if (media_index >= sizeof(media_data)) {
				media_index = 0U;
			}
		}
	} else {
		if ((media_index + (samples_num << 2)) > sizeof(media_data)) {
			media = (uint8_t *)&a2dp_pcm_buffer[0];
			memcpy(media, ((uint8_t *)media_data + media_index),
				sizeof(media_data) - media_index);
			memcpy(&media[sizeof(media_data) - media_index],
				((uint8_t *)media_data),
				((samples_num << 2) - (sizeof(media_data) - media_index)));
			/* Update the tone index */
			media_index = ((samples_num << 2) -
				(sizeof(media_data) - media_index));
		} else {
			media = ((uint8_t *)media_data + media_index);
			/* Update the tone index */
			media_index += (samples_num << 2);
			if (media_index >= sizeof(media_data)) {
				media_index = 0U;
			}
		}
	}

	return media;
}

static void audio_work_handler(struct k_work *work)
{
	int64_t period_ms;
	uint32_t a2dp_src_num_samples;
	uint8_t *pcm_data;
	uint8_t index;
	uint32_t pcm_frame_size;
	uint32_t pcm_frame_samples;
	uint32_t encoded_frame_size;
	struct net_buf *buf;
	uint8_t frame_num = 0;
	uint8_t *sbc_hdr;
	uint32_t pdu_len;
	uint32_t out_size;
	int err;

	/* If stopped then return */
	if (!a2dp_src_playback) {
		return;
	}

	buf = bt_a2dp_stream_create_pdu(&a2dp_tx_pool, K_NO_WAIT);
	if (buf == NULL) {
		/* fail */
		return;
	}

	period_ms = k_uptime_delta(&ref_time);

	pcm_frame_size = sbc_frame_bytes(&encoder);
	pcm_frame_samples = sbc_frame_samples(&encoder);
	encoded_frame_size = sbc_frame_encoded_bytes(&encoder);

	sbc_hdr = net_buf_add(buf, 1u);
	/* Get the number of samples */
	a2dp_src_num_samples = (uint16_t)((period_ms * a2dp_src_sf) / 1000);
	a2dp_src_missed_count += (uint32_t)((period_ms * a2dp_src_sf) % 1000);
	a2dp_src_missed_count += ((a2dp_src_num_samples % pcm_frame_samples) * 1000);
	a2dp_src_num_samples = (a2dp_src_num_samples / pcm_frame_samples) * pcm_frame_samples;
	frame_num = a2dp_src_num_samples / pcm_frame_samples;

	pdu_len = buf->len + frame_num * encoded_frame_size;

	if (pdu_len > net_buf_tailroom(buf)) {
		printk("need increase buf size\n");
		return;
	}

	if (pdu_len > bt_a2dp_get_mtu(&sbc_stream)) {
		printk("need decrease CONFIG_BT_A2DP_SOURCE_DATA_SEND_INTERVAL\n");
		return;
	}

	/* Raw adjust for the drift */
	while (a2dp_src_missed_count >= (1000 * pcm_frame_samples)) {
		if (pdu_len + encoded_frame_size > bt_a2dp_get_mtu(&sbc_stream) ||
		    pdu_len + encoded_frame_size > net_buf_tailroom(buf)) {
			break;
		}

		pdu_len += encoded_frame_size;
		a2dp_src_num_samples += pcm_frame_samples;
		frame_num++;
		a2dp_src_missed_count -= (1000 * pcm_frame_samples);
	}

	pcm_data = a2dp_produce_media(a2dp_src_num_samples);
	if (pcm_data == NULL) {
		printk("no media data\n");
		return;
	}

	for (index = 0; index < frame_num; index++) {
		out_size = sbc_encode(&encoder, (uint8_t *)&pcm_data[index * pcm_frame_size],
				      net_buf_tail(buf));
		if (encoded_frame_size != out_size) {
			printk("sbc encode fail\n");
			continue;
		}

		net_buf_add(buf, encoded_frame_size);
	}

	*sbc_hdr = (uint8_t)BT_A2DP_SBC_MEDIA_HDR_ENCODE(frame_num, 0, 0, 0);

	if (default_a2dp != NULL) {
		err = bt_a2dp_stream_send(&sbc_stream, buf, send_count, send_samples_count);
		if (err < 0) {
			printk("  Failed to send SBC audio data on streams(%d)\n", err);
			net_buf_unref(buf);
		}
	}

	send_count++;
	send_samples_count += a2dp_src_num_samples;
}

static K_WORK_DEFINE(audio_work, audio_work_handler);

static void a2dp_playback_timeout_handler(struct k_timer *timer)
{
	k_work_submit(&audio_work);
}
#endif

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
		if (0U != (codec_ie[0U] & A2DP_SBC_CH_MODE_STEREO)) {
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

static void app_connected(struct bt_a2dp *a2dp, int err)
{
	if (!err) {
		default_a2dp = a2dp;
		bt_shell_print("a2dp connected");
	} else {
		bt_shell_print("a2dp connecting fail");
	}
}

static void app_disconnected(struct bt_a2dp *a2dp)
{
	found_peer_sbc_endpoint = NULL;
	bt_shell_print("a2dp disconnected");
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
	return 0;
}

static void app_release_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
	if (rsp_err_code == 0) {
		bt_shell_print("success to release");
	} else {
		bt_shell_print("fail to release");
	}
}

static int app_start_req(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code)
{
	*rsp_err_code = 0;
	bt_shell_print("receive requesting start and accept");
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
	return 0;
}

static void app_suspend_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
	if (rsp_err_code == 0) {
		bt_shell_print("success to suspend");
	} else {
		bt_shell_print("fail to suspend");
	}
}

static void stream_configured(struct bt_a2dp_stream *stream)
{
	bt_shell_print("stream configured");
#if defined(CONFIG_BT_A2DP_SOURCE_SBC_AUDIO_SHELL)
	struct sbc_encoder_init_param param;
	struct bt_a2dp_codec_sbc_params *sbc_config = (struct bt_a2dp_codec_sbc_params *)
						      &sbc_cfg_default.codec_config->codec_ie[0];

	a2dp_src_sf = bt_a2dp_sbc_get_sampling_frequency(sbc_config);
	a2dp_src_nc = bt_a2dp_sbc_get_channel_num(sbc_config);

	param.bit_rate = CONFIG_BT_A2DP_SOURCE_SBC_BIT_RATE_DEFAULT;
	param.samp_freq = a2dp_src_sf;
	param.blk_len = bt_a2dp_sbc_get_block_length(sbc_config);
	param.subband = bt_a2dp_sbc_get_subband_num(sbc_config);
	param.alloc_mthd = bt_a2dp_sbc_get_allocation_method(sbc_config);
	param.ch_mode = bt_a2dp_sbc_get_channel_mode(sbc_config);
	param.ch_num = bt_a2dp_sbc_get_channel_num(sbc_config);
	param.min_bitpool = sbc_config->min_bitpool;
	param.max_bitpool = sbc_config->max_bitpool;

	if (sbc_setup_encoder(&encoder, &param)) {
		printk("sbc encoder initialization fail\n");
	} else {
		printk("sbc encoder initialization success\n");
	}
#endif
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
#if defined(CONFIG_BT_A2DP_SOURCE_SBC_AUDIO_SHELL)
	uint32_t audio_time_interval = CONFIG_BT_A2DP_SOURCE_DATA_SEND_INTERVAL;

	/* Start Audio Source */
	a2dp_src_playback = true;

	k_uptime_delta(&ref_time);
	k_timer_start(&a2dp_player_timer, K_MSEC(audio_time_interval), K_MSEC(audio_time_interval));
#endif
}

static void stream_suspended(struct bt_a2dp_stream *stream)
{
	bt_shell_print("stream suspended");
#if defined(CONFIG_BT_A2DP_SOURCE_SBC_AUDIO_SHELL)
	k_timer_stop(&a2dp_player_timer);
#endif
}

#if defined(CONFIG_BT_A2DP_SINK)
static void sink_sbc_streamer_data(struct bt_a2dp_stream *stream, struct net_buf *buf,
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

static void stream_recv(struct bt_a2dp_stream *stream,
			struct net_buf *buf, uint16_t seq_num, uint32_t ts)
{
	sink_sbc_streamer_data(stream, buf, seq_num, ts);
}

static void app_delay_report_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
	if (rsp_err_code == 0) {
		bt_shell_print("success to send report delay");
	} else {
		bt_shell_print("fail to send report delay");
	}
}
#endif

#if defined(CONFIG_BT_A2DP_SOURCE)
static int app_delay_report_req(struct bt_a2dp_stream *stream, uint16_t value,
				uint8_t *rsp_err_code)
{
	*rsp_err_code = 0;
	bt_shell_print("receive delay report and accept");
	return 0;
}

static void delay_report(struct bt_a2dp_stream *stream, uint16_t value)
{
	bt_shell_print("received delay report: %d 1/10ms", value);
}
#endif

static int app_get_config_req(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code)
{
	*rsp_err_code = 0;
	bt_shell_print("receive get config request and accept");
	return 0;
}

static void app_get_config_rsp(struct bt_a2dp_stream *stream, struct bt_a2dp_codec_cfg *codec_cfg,
			       uint8_t rsp_err_code)
{
	bt_shell_print("get config result: %d", rsp_err_code);

	if (rsp_err_code == 0) {
		uint32_t sample_rate = bt_a2dp_sbc_get_sampling_frequency(
			(struct bt_a2dp_codec_sbc_params *)&codec_cfg->codec_config->codec_ie[0]);
		bt_shell_print("sample rate %dHz", sample_rate);
	}
}

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
	.get_config_req = app_get_config_req,
	.get_config_rsp = app_get_config_rsp,
#if defined(CONFIG_BT_A2DP_SOURCE)
	.delay_report_req = app_delay_report_req,
#endif
#if defined(CONFIG_BT_A2DP_SINK)
	.delay_report_rsp = app_delay_report_rsp,
#endif
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

static struct bt_a2dp_stream_ops stream_ops = {
	.configured = stream_configured,
	.established = stream_established,
	.released = stream_released,
	.started = stream_started,
	.suspended = stream_suspended,
#if defined(CONFIG_BT_A2DP_SINK)
	.recv = stream_recv,
#endif
#if defined(CONFIG_BT_A2DP_SOURCE)
	.sent = NULL,
	.delay_report = delay_report,
#endif
};

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
			shell_error(sh, "fail to configure: %d", err);
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

static struct bt_a2dp_discover_param discover_param = {
	.cb = bt_a2dp_discover_peer_endpoint_cb,
	.seps_info = &found_seps[0],
	.sep_count = 5,
};

static int cmd_get_peer_eps(const struct shell *sh, int32_t argc, char *argv[])
{
	int err = 0;

	if (a2dp_initied == 0) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	if (default_a2dp != NULL) {
		discover_param.avdtp_version = (uint16_t)shell_strtoul(argv[1], 0, &err);
		if (err != 0) {
			shell_error(sh, "failed to parse avdtp version: %d", err);

			return -ENOEXEC;
		}

		err = bt_a2dp_discover(default_a2dp, &discover_param);

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

	buf = bt_a2dp_stream_create_pdu(&a2dp_tx_pool, K_FOREVER);
	if (buf == NULL) {
		shell_error(sh, "fail to allocate buffer");
		return -ENOEXEC;
	}

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

#if defined(CONFIG_BT_A2DP_SINK)
static int cmd_send_delay_report(const struct shell *sh, int32_t argc, char *argv[])
{
	int err;

	if (a2dp_initied == 0) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	err = bt_a2dp_stream_delay_report(&sbc_stream, 1);
	if (err < 0) {
		shell_print(sh, "fail to send delay report (%d)\n", err);
	}

	return 0;
}
#endif

static int cmd_get_config(const struct shell *sh, int32_t argc, char *argv[])
{
	if (a2dp_initied == 0) {
		shell_print(sh, "need to register a2dp connection callbacks");
		return -ENOEXEC;
	}

	if (bt_a2dp_stream_get_config(&sbc_stream) != 0) {
		shell_error(sh, "fail");
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
	SHELL_CMD_ARG(discover_peer_eps, NULL, "<avdtp version value>", cmd_get_peer_eps, 2, 0),
	SHELL_CMD_ARG(configure, NULL, "\"configure/enable the stream\"", cmd_configure, 1, 0),
	SHELL_CMD_ARG(establish, NULL, "\"establish the stream\"", cmd_establish, 1, 0),
	SHELL_CMD_ARG(reconfigure, NULL, "\"reconfigure the stream\"", cmd_reconfigure, 1, 0),
	SHELL_CMD_ARG(release, NULL, "\"release the stream\"", cmd_release, 1, 0),
	SHELL_CMD_ARG(start, NULL, "\"start the stream\"", cmd_start, 1, 0),
	SHELL_CMD_ARG(suspend, NULL, "\"suspend the stream\"", cmd_suspend, 1, 0),
	SHELL_CMD_ARG(abort, NULL, "\"abort the stream\"", cmd_abort, 1, 0),
	SHELL_CMD_ARG(send_media, NULL, HELP_NONE, cmd_send_media, 1, 0),
#if defined(CONFIG_BT_A2DP_SINK)
	SHELL_CMD_ARG(send_delay_report, NULL, HELP_NONE, cmd_send_delay_report, 1, 0),
#endif
	SHELL_CMD_ARG(get_config, NULL, HELP_NONE, cmd_get_config, 1, 0),
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
