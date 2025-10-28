/*
 * Copyright 2026 NXP
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
#include <zephyr/bluetooth/sbc.h>
#include "sine.h"
#include <zephyr/bluetooth/classic/a2dp.h>
#include <zephyr/bluetooth/classic/sdp.h>

#include <zephyr/shell/shell.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

/* sbc audio stream control variables */
static int64_t ref_time;
static uint32_t a2dp_src_missed_count;
static volatile bool a2dp_src_playback;
static volatile int media_index;
static uint32_t a2dp_src_sf;
static uint8_t a2dp_src_nc;
static uint32_t send_samples_count;
static uint16_t send_count;
/* max pcm data size per interval. The max sample freq is 48K.
 * interval * 48 * 2 (max channels) * 2 (sample width) * 2 (the worst case: send two intervals'
 * data if timer is blocked)
 */
static uint8_t a2dp_pcm_buffer[CONFIG_BT_A2DP_SOURCE_DATA_SEND_INTERVAL * 48 * 2 * 2 * 2];
struct sbc_encoder encoder;
extern struct bt_a2dp_stream sbc_stream;
extern struct bt_a2dp *default_a2dp;

NET_BUF_POOL_DEFINE(a2dp_audio_tx_pool, CONFIG_BT_MAX_CONN,
		BT_L2CAP_BUF_SIZE(CONFIG_BT_A2DP_SOURCE_DATA_BUF_SIZE),
		CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static void a2dp_playback_timeout_handler(struct k_timer *timer);
K_TIMER_DEFINE(a2dp_player_timer, a2dp_playback_timeout_handler, NULL);

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

	buf = bt_a2dp_stream_create_pdu(&a2dp_audio_tx_pool, K_NO_WAIT);
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
void a2dp_audio_sbc_configure(struct bt_a2dp_codec_cfg *config)
{
	struct sbc_encoder_init_param param;
	struct bt_a2dp_codec_sbc_params *sbc_config = (struct bt_a2dp_codec_sbc_params *)
						    &config->codec_config->codec_ie[0];

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
}

void a2dp_audio_sbc_stream_started(void)
{
	uint32_t audio_time_interval = CONFIG_BT_A2DP_SOURCE_DATA_SEND_INTERVAL;

	/* Start Audio Source */
	a2dp_src_playback = true;

	k_uptime_delta(&ref_time);
	k_timer_start(&a2dp_player_timer, K_MSEC(audio_time_interval), K_MSEC(audio_time_interval));
}

void a2dp_audio_sbc_stream_suspended(void)
{
	k_timer_stop(&a2dp_player_timer);
}
