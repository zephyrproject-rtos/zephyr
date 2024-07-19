/*
 * Copyright 2020 - 2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/libsbc/sbc.h>
#include <zephyr/bluetooth/classic/a2dp_codec_sbc.h>
#include <zephyr/bluetooth/classic/a2dp.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include "app_connect.h"
#include "a2dp_media_48KHz_1ksin.h"

#define SDP_CLIENT_USER_BUF_LEN		512U
NET_BUF_POOL_FIXED_DEFINE(app_sdp_client_pool, CONFIG_BT_MAX_CONN,
			  SDP_CLIENT_USER_BUF_LEN, 8, NULL);

static uint8_t app_sdp_a2sink_user(struct bt_conn *conn, struct bt_sdp_client_result *result);

static struct bt_sdp_discover_params discov_a2dp_sink = {
	.uuid = BT_UUID_DECLARE_16(BT_SDP_AUDIO_SINK_SVCLASS),
	.func = app_sdp_a2sink_user,
	.pool = &app_sdp_client_pool,
};

static void a2dp_playback_timeout_handler(struct k_timer *timer);
K_TIMER_DEFINE(a2dp_player_timer, a2dp_playback_timeout_handler, NULL);

#define APPL_A2DP_MTU   (672U)
#define DEFAULT_BIT_RATE (328u)

static uint32_t a2dp_src_sf;

static int64_t ref_time;
static uint32_t a2dp_src_missed_count;
static volatile uint8_t a2dp_src_playback;
static int tone_index;
uint8_t a2dp_src_nc;

#define A2DP_SBC_BLOCK_MAX (512U)
uint32_t audio_time_interval; /* ms */
uint32_t audio_frame_sample_count;
uint8_t a2dp_pcm_buffer[1920u]; /* 10ms max packet pcm data size. the max is 480 * 2 * 2 */
uint8_t a2dp_sbc_encode_buffer_frame[A2DP_SBC_BLOCK_MAX];
struct sbc_encoder encoder;
uint32_t send_samples_count;
uint16_t send_count;

#define A2DP_SRC_PERIOD_MS	10

struct bt_a2dp *default_a2dp;
struct bt_a2dp_endpoint *default_a2dp_endpoint;
BT_A2DP_SBC_SOURCE_EP_DEFAULT(sbc_endpoint);
struct bt_a2dp_codec_ie peer_sbc_capabilities;
static struct bt_a2dp_ep peer_sbc_endpoint = {
	.codec_cap = &peer_sbc_capabilities,
};
static bool peer_sbc_found;
BT_A2DP_SBC_EP_CFG_DEFAULT(sbc_cfg_default, A2DP_SBC_SAMP_FREQ_44100);
static struct bt_a2dp_stream sbc_stream;
NET_BUF_POOL_DEFINE(a2dp_tx_pool, CONFIG_BT_MAX_CONN,
		BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
		CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static uint8_t *a2dp_pl_produce_media(uint32_t a2dp_src_num_samples)
{
	uint8_t *media = NULL;
	uint16_t  medialen;

	/* Music Audio is Stereo */
	medialen = (a2dp_src_num_samples * a2dp_src_nc * 2);

	/* For mono or dual configuration, skip alternative samples */
	if (1 == a2dp_src_nc) {
		uint16_t index;

		media = (uint8_t *)&a2dp_pcm_buffer[0];

		for (index = 0; index < a2dp_src_num_samples; index++) {
			media[(2 * index)] = *((uint8_t *)beethoven + tone_index);
			media[(2 * index) + 1] = *((uint8_t *)beethoven + tone_index + 1);
			/* Update the tone index */
			tone_index += 4u;
			if (tone_index >= sizeof(beethoven)) {
				tone_index = 0U;
			}
		}
	} else {
		if ((tone_index + (a2dp_src_num_samples << 2)) > sizeof(beethoven)) {
			media = (uint8_t *)&a2dp_pcm_buffer[0];
			memcpy(media, ((uint8_t *)beethoven + tone_index),
				sizeof(beethoven) - tone_index);
			memcpy(&media[sizeof(beethoven) - tone_index],
				((uint8_t *)beethoven),
				((a2dp_src_num_samples << 2) - (sizeof(beethoven) - tone_index)));
			/* Update the tone index */
			tone_index = ((a2dp_src_num_samples << 2) -
				(sizeof(beethoven) - tone_index));
		} else {
			media = ((uint8_t *)beethoven + tone_index);
			/* Update the tone index */
			tone_index += (a2dp_src_num_samples << 2);
			if (tone_index >= sizeof(beethoven)) {
				tone_index = 0U;
			}
		}
	}

	return media;
}

static void a2dp_playback_timeout_handler(struct k_timer *timer)
{
	int64_t period_ms;
	uint32_t a2dp_src_num_samples;
	uint8_t *pcm_data;
	uint8_t index;
	uint32_t pcm_frame_size;
	uint32_t pcm_frame_samples;
	uint32_t encoded_frame_size;
	uint8_t *net_buffer;
	struct net_buf *buf;
	uint32_t sample_count = 0;
	uint8_t frame_num = 0;
	uint8_t *sbc_hdr;
	int err;

	/* If stopped then return */
	if (0U == a2dp_src_playback) {
		return;
	}

	period_ms = k_uptime_delta(&ref_time);

	buf = net_buf_alloc(&a2dp_tx_pool, K_NO_WAIT);
	if (buf == NULL) {
		return;
	}

	net_buf_reserve(buf, BT_A2DP_STREAM_BUF_RESERVE);
	sbc_hdr = net_buf_add(buf, 1u);
	/* Get the number of samples */
	a2dp_src_num_samples = (uint16_t)((period_ms * a2dp_src_sf) / 1000);
	a2dp_src_missed_count += (uint32_t)((period_ms * a2dp_src_sf) % 1000);

	/* Raw adjust for the drift */
	while (a2dp_src_missed_count >= (1000 * audio_frame_sample_count)) {
		a2dp_src_num_samples += audio_frame_sample_count;
		a2dp_src_missed_count -= (1000 * audio_frame_sample_count);
	}

	pcm_data = a2dp_pl_produce_media(a2dp_src_num_samples);
	if (pcm_data == NULL) {
		return;
	}

	pcm_frame_size = sbc_frame_bytes(&encoder);
	pcm_frame_samples = sbc_frame_samples(&encoder);
	encoded_frame_size = sbc_frame_encoded_bytes(&encoder);
	for (index = 0; index < (a2dp_src_num_samples / audio_frame_sample_count); index++) {
		net_buffer = net_buf_tail(buf);
		if (buf->len + encoded_frame_size > bt_a2dp_get_mtu(&sbc_stream)) {
			printk("mtu error");
			return;
		}

		err = sbc_encode(&encoder,
				(uint8_t *)&pcm_data[index * pcm_frame_size],
				encoded_frame_size, &net_buffer[0]);
		if (err) {
			printk("sbc encode fail");
			continue;
		}
		buf->len += encoded_frame_size;
		sample_count += pcm_frame_samples;
		frame_num++;
	}

	*sbc_hdr = (uint8_t)BT_A2DP_SBC_MEDIA_HDR_ENCODE(frame_num, 0, 0, 0);
	err = bt_a2dp_stream_send(&sbc_stream, buf, send_count, send_samples_count);
	if (err < 0) {
		printk("  Failed to send SBC audio data on streams(%d)\n", err);
		net_buf_unref(buf);
	} else {
		send_count++;
		send_samples_count += sample_count;
	}
}

static void music_control_a2dp_start_callback(void)
{
	/* Start Audio Source */
	a2dp_src_playback = 1U;
	audio_frame_sample_count = sbc_frame_samples(&encoder);

	/* calculate the interval that contains multiple of frame. default is 10ms */
	audio_time_interval = ((10 * a2dp_src_sf / 1000) / audio_frame_sample_count);
	audio_time_interval = audio_time_interval * audio_frame_sample_count * 1000 / a2dp_src_sf;
	k_uptime_delta(&ref_time);
	k_timer_start(&a2dp_player_timer, K_MSEC(audio_time_interval), K_MSEC(audio_time_interval));
}

void app_endpoint_configured(struct bt_a2dp_codec_cfg *config)
{
	struct bt_a2dp_codec_sbc_params *sbc_config =
		(struct bt_a2dp_codec_sbc_params *)
		&config->codec_config->codec_ie[0];

	a2dp_src_sf = bt_a2dp_sbc_get_sampling_frequency(sbc_config);
	a2dp_src_nc = bt_a2dp_sbc_get_channel_num(sbc_config);

	sbc_setup_encoder(&encoder, a2dp_src_sf, bt_a2dp_sbc_get_channel_mode(sbc_config),
		bt_a2dp_sbc_get_block_length(sbc_config), bt_a2dp_sbc_get_subband_num(sbc_config),
		bt_a2dp_sbc_get_allocation_method(sbc_config), DEFAULT_BIT_RATE);
	bt_a2dp_stream_establish(&sbc_stream);
}

void stream_configured(struct bt_a2dp_stream *stream)
{
	printk("stream configured\r\n");
	app_endpoint_configured(&sbc_cfg_default);
}

void stream_established(struct bt_a2dp_stream *stream)
{
	printk("stream established\r\n");
	bt_a2dp_stream_start(stream);
}

void stream_released(struct bt_a2dp_stream *stream)
{
	printk("stream released\r\n");
	k_timer_stop(&a2dp_player_timer);
}

void stream_started(struct bt_a2dp_stream *stream)
{
	printk("stream started\r\n");
	music_control_a2dp_start_callback();
}

static struct bt_a2dp_stream_ops stream_ops = {
	.configured = stream_configured,
	.established = stream_established,
	.released = stream_released,
	.started = stream_started,
};

static uint8_t bt_a2dp_discover_peer_endpoint_cb(struct bt_a2dp *a2dp,
		struct bt_a2dp_ep_info *info, struct bt_a2dp_ep **ep)
{
	*ep = NULL;
	if (info != NULL) {
		printk("find one endpoint:");
		if (info->codec_type == BT_A2DP_SBC) {
			printk("SBC and use it\r\n");
			if (ep != NULL) {
				peer_sbc_found = true;
				*ep = &peer_sbc_endpoint;
			}
		} else {
			printk("not SBC codecs\r\n");
		}
	} else {
		if (peer_sbc_found) {
			int err;

			bt_a2dp_stream_cb_register(&sbc_stream, &stream_ops);
			err = bt_a2dp_stream_config(a2dp, &sbc_stream,
				&sbc_endpoint, &peer_sbc_endpoint,
				&sbc_cfg_default);
			if (err) {
				printk("fail to configure\r\n");
			}
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

void app_connected(struct bt_a2dp *a2dp, int err)
{
	if (!err) {
		int err = bt_a2dp_discover(a2dp, &discover_param);
		if (err) {
			printk("fail to discover\r\n");
		}
		printk("a2dp connected success\r\n");
	} else {
		if (default_a2dp != NULL) {
			default_a2dp = NULL;
		}
		printk("a2dp connected fail\r\n");
	}
}

void app_disconnected(struct bt_a2dp *a2dp)
{
	if (default_a2dp != NULL) {
		default_a2dp = NULL;
	}
	a2dp_src_playback = 0U;
	/* stop timer */
	k_timer_stop(&a2dp_player_timer);
	printk("a2dp disconnected\r\n");
}

static uint8_t app_sdp_a2sink_user(struct bt_conn *conn,
			   struct bt_sdp_client_result *result)
{
	uint16_t param;
	int res;

	if ((result) && (result->resp_buf)) {
		printk("sdp success callback\r\n");
		res = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_L2CAP, &param);
		if (res < 0) {
			printk("PSM is not found\r\n");
			return BT_SDP_DISCOVER_UUID_CONTINUE;
		}
		if (param == BT_UUID_AVDTP_VAL) {
			printk("A2DP Service found. Connecting ...\n");
			default_a2dp = bt_a2dp_connect(default_conn);
			if (default_a2dp == NULL) {
				printk("fail to connect a2dp\r\n");
			}
			return BT_SDP_DISCOVER_UUID_STOP;
		}
		return BT_SDP_DISCOVER_UUID_CONTINUE;
	}

	printk("sdp fail callback\r\n");
	return BT_SDP_DISCOVER_UUID_CONTINUE;
}

void app_sdp_discover_a2dp_sink(void)
{
	int res;

	res = bt_sdp_discover(default_conn, &discov_a2dp_sink);
	if (res) {
		printk("SDP discovery failed: result\r\n");
	} else {
		printk("SDP discovery started\r\n");
	}
}

static struct bt_a2dp_cb a2dp_cb = {
	.connected = app_connected,
	.disconnected = app_disconnected,
};

static void app_a2dp_init(void)
{
	bt_a2dp_register_ep(&sbc_endpoint, BT_AVDTP_AUDIO, BT_AVDTP_SOURCE);
	bt_a2dp_register_cb(&a2dp_cb);
}

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
    BT_SDP_SERVICE_NAME("A2DPSource"),
    BT_SDP_SUPPORTED_FEATURES(0x0001U),
};

static struct bt_sdp_record a2dp_source_rec = BT_SDP_RECORD(a2dp_source_attrs);

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	bt_sdp_register_service(&a2dp_source_rec);
	app_connect_init();
	app_a2dp_init();
}

int main(void)
{
	int err;

	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}

	return err;
}
