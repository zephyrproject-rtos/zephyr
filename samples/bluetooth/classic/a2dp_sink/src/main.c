/* main.c - Application main entry point */

/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/a2dp_codec_sbc.h>
#include <zephyr/bluetooth/classic/a2dp.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/settings/settings.h>
#include "audio_buf.h"
#include "codec_play.h"

#define SAMPLE_BIT_WIDTH (16U)

struct bt_a2dp *default_a2dp;
BT_A2DP_SBC_SINK_EP_DEFAULT(sbc_sink_ep);
static struct bt_a2dp_stream sbc_stream;
BT_A2DP_SBC_EP_CFG_DEFAULT(sbc_cfg, A2DP_SBC_SAMP_FREQ_44100);

#define A2DP_VERSION 0x0104

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
				BT_SDP_ARRAY_16(A2DP_VERSION) /* 01 04 */
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("A2DPSink"),
	BT_SDP_SUPPORTED_FEATURES(0x0001U),
};

static struct bt_sdp_record a2dp_sink_rec = BT_SDP_RECORD(a2dp_sink_attrs);

static void sbc_stop_play(void)
{
	printk("stream stopped\n");
	codec_play_stop();
}

void sbc_stream_configured(struct bt_a2dp_stream *stream)
{
	uint32_t sample_freq;
	uint8_t channel_num;
	struct bt_a2dp_codec_sbc_params *sbc_config = (struct bt_a2dp_codec_sbc_params *)
						      &sbc_cfg.codec_config->codec_ie[0];

	channel_num = bt_a2dp_sbc_get_channel_num(sbc_config);
	sample_freq = bt_a2dp_sbc_get_sampling_frequency(sbc_config);
	codec_play_configure(sample_freq, SAMPLE_BIT_WIDTH, channel_num);

	printk("stream configured\n");
}

void sbc_stream_established(struct bt_a2dp_stream *stream)
{
	printk("stream established\n");
}

void sbc_stream_released(struct bt_a2dp_stream *stream)
{
	sbc_stop_play();
}

void sbc_stream_started(struct bt_a2dp_stream *stream)
{
	printk("stream started\n");

	uint32_t sample_freq;
	struct bt_a2dp_codec_sbc_params *sbc_config = (struct bt_a2dp_codec_sbc_params *)
						      &sbc_cfg.codec_config->codec_ie[0];

	sample_freq = bt_a2dp_sbc_get_sampling_frequency(sbc_config);
	audio_buf_reset(sample_freq);
	codec_play_start();
}

void sbc_stream_suspended(struct bt_a2dp_stream *stream)
{
	sbc_stop_play();
}

void sbc_stream_recv(struct bt_a2dp_stream *stream, struct net_buf *buf, uint16_t seq_num,
		     uint32_t ts)
{
	uint8_t channel_num;
	struct bt_a2dp_codec_sbc_params *sbc_config = (struct bt_a2dp_codec_sbc_params *)
						      &sbc_cfg.codec_config->codec_ie[0];

	channel_num = bt_a2dp_sbc_get_channel_num(sbc_config);

	audio_process_sbc_buf(net_buf_pull_u8(buf), buf->data, buf->len, seq_num, ts, channel_num);
}

static struct bt_a2dp_stream_ops stream_ops = {
	.configured = sbc_stream_configured,
	.established = sbc_stream_established,
	.released = sbc_stream_released,
	.started = sbc_stream_started,
	.suspended = sbc_stream_suspended,
	.recv = sbc_stream_recv,
};

void app_a2dp_connected(struct bt_a2dp *a2dp, int err)
{
	if (err == 0) {
		default_a2dp = a2dp;
		printk("a2dp connected success\n");
	} else {
		printk("a2dp connected fail\n");
	}
}

void app_a2dp_disconnected(struct bt_a2dp *a2dp)
{
	default_a2dp = NULL;
	codec_play_stop();
	printk("a2dp disconnected\n");
}

int app_a2dp_config_req(struct bt_a2dp *a2dp, struct bt_a2dp_ep *ep,
			struct bt_a2dp_codec_cfg *codec_cfg, struct bt_a2dp_stream **stream,
			uint8_t *rsp_err_code)
{
	uint32_t sample_rate;

	*sbc_cfg.codec_config = *codec_cfg->codec_config;

	bt_a2dp_stream_cb_register(&sbc_stream, &stream_ops);
	*stream = &sbc_stream;
	*rsp_err_code = 0;

	printk("receive requesting config and accept\n");
	sample_rate = bt_a2dp_sbc_get_sampling_frequency(
		(struct bt_a2dp_codec_sbc_params *)&codec_cfg->codec_config->codec_ie[0]);
	printk("sample rate %dHz\n", sample_rate);

	return 0;
}

static struct bt_a2dp_cb a2dp_cb = {
	.connected = app_a2dp_connected,
	.disconnected = app_a2dp_disconnected,
	.config_req = app_a2dp_config_req,
};

static void bt_ready(int err)
{
	if (err != 0) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	printk("Bluetooth initialized\n");

	bt_sdp_register_service(&a2dp_sink_rec);

	bt_a2dp_register_ep(&sbc_sink_ep, BT_AVDTP_AUDIO, BT_AVDTP_SINK);
	bt_a2dp_register_cb(&a2dp_cb);

	err = bt_br_set_connectable(true, NULL);
	if (err != 0) {
		printk("BR/EDR set/rest connectable failed (err %d)\n", err);
		return;
	}
	err = bt_br_set_discoverable(true, false);
	if (err != 0) {
		printk("BR/EDR set discoverable failed (err %d)\n", err);
		return;
	}

	printk("BR/EDR set connectable and discoverable done\n");
}

int main(void)
{
	int err;

	if (codec_play_init() != 0) {
		printk("codec initialization fail\n");
		return 0;
	}

	err = bt_enable(bt_ready);
	if (err != 0) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	codec_keep_play();

	return 0;
}
