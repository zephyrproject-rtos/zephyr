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
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/a2dp.h>
#include <bluetooth/a2dp_codec_sbc.h>
#include <bluetooth/sdp.h>
#include "app_connect.h"
#include "a2dp_media_48KHz_1ksin.h"

#define SDP_CLIENT_USER_BUF_LEN		512U
NET_BUF_POOL_FIXED_DEFINE(app_sdp_client_pool, CONFIG_BT_MAX_CONN,
			  SDP_CLIENT_USER_BUF_LEN, NULL);

static uint8_t app_sdp_a2sink_user(struct bt_conn *conn, struct bt_sdp_client_result *result);

static struct bt_sdp_discover_params discov_a2dp_sink = {
	.uuid = BT_UUID_DECLARE_16(BT_SDP_AUDIO_SINK_SVCLASS),
	.func = app_sdp_a2sink_user,
	.pool = &app_sdp_client_pool,
};

static void a2dp_playback_timeout_handler(struct k_timer *timer);
K_TIMER_DEFINE(a2dp_player_timer, a2dp_playback_timeout_handler, NULL);

#define APPL_A2DP_MTU   (672U)

static uint32_t a2dp_src_sf;
static uint16_t a2dp_src_num_samples;

static int64_t ref_time;
static uint32_t a2dp_src_missed_count;
static volatile uint8_t a2dp_src_playback;
static int tone_index;
uint8_t a2dp_src_nc;

#define A2DP_SRC_PERIOD_MS	10

struct bt_a2dp *default_a2dp;
struct bt_a2dp_endpoint *default_a2dp_endpoint;
BT_A2DP_SBC_SOURCE_ENDPOINT(sbc_endpoint, A2DP_SBC_SAMP_FREQ_48000);

static void a2dp_pl_produce_media(void)
{
	uint8_t * media;
	uint16_t  medialen;
	uint8_t malloc = 0;

	/* Music Audio is Stereo */
	medialen = (a2dp_src_num_samples << a2dp_src_nc);

	/* For mono or dual configuration, skip alternative samples */
	if (1 == a2dp_src_nc) {
		uint16_t index;

		/* Allocate Memory */
		malloc = 1;
		media = (uint8_t *)k_malloc(medialen);

		if (NULL == media) {
			return;
		}

		for (index = 0; index < a2dp_src_num_samples; index++) {
			media[(2 * index)] = *((uint8_t *)beethoven + tone_index);
			media[(2 * index) + 1] = *((uint8_t *)beethoven + tone_index + 1);
			/* Update the tone index */
			tone_index += 4u;
			if (tone_index >= sizeof(beethoven)) {
				tone_index = 0U;
			}
		}
	}
	else
	{
		if ((tone_index + (a2dp_src_num_samples << 2)) > sizeof(beethoven)) {
			malloc = 1;
			media = (uint8_t *)k_malloc(medialen);
			if (NULL == media) {
				printk("Memory Allocation failed in Produce Media\n");
				return;
			}
			memcpy(media, ((uint8_t *)beethoven + tone_index), sizeof(beethoven) - tone_index);
			memcpy(&media[sizeof(beethoven) - tone_index],
				   ((uint8_t *)beethoven),
				   ((a2dp_src_num_samples << 2) - (sizeof(beethoven) - tone_index)));
			/* Update the tone index */
			tone_index = ((a2dp_src_num_samples << 2) - (sizeof(beethoven) - tone_index));
		} else {
			media = ((uint8_t *)beethoven + tone_index);
			/* Update the tone index */
			tone_index += (a2dp_src_num_samples << 2);
			if (tone_index >= sizeof(beethoven)) {
				tone_index = 0U;
			}
		}
	}

	/* Give data to callback */
	bt_a2dp_src_media_write(default_a2dp_endpoint, media, medialen);

	if (malloc == 1) {
		k_free(media);
	}
}

static void a2dp_playback_timeout_handler(struct k_timer *timer)
{
	int64_t period_ms;

	/* If stopped then return */
	if (0U == a2dp_src_playback) {
		return;
	}

	period_ms = k_uptime_delta(&ref_time);

	/* Get the number of samples */
	a2dp_src_num_samples = (uint16_t)((period_ms * a2dp_src_sf) / 1000);
	a2dp_src_missed_count += (uint32_t)((period_ms * a2dp_src_sf) % 1000);

	/* Raw adjust for the drift */
	while (a2dp_src_missed_count >= 1000) {
		a2dp_src_num_samples++;
		a2dp_src_missed_count -= 1000;
	}
	a2dp_pl_produce_media();
}

static void a2dp_pl_start_playback_timer(void)
{
	k_uptime_delta(&ref_time);
	k_timer_start(&a2dp_player_timer, K_MSEC(10), K_MSEC(10));
}

static void music_control_a2dp_start_callback(int err)
{
	/* Start Audio Source */
	a2dp_src_playback = 1U;
	a2dp_pl_start_playback_timer();
}

void app_endpoint_configured(struct bt_a2dp_endpoint_configure_result *result)
{
	if (result->err == 0) {
		default_a2dp_endpoint = &sbc_endpoint;

		a2dp_src_sf = bt_a2dp_sbc_get_sampling_frequency((struct bt_a2dp_codec_sbc_params *)&result->config.media_config->codec_ie[0]);
		a2dp_src_nc = bt_a2dp_sbc_get_channel_num((struct bt_a2dp_codec_sbc_params *)&result->config.media_config->codec_ie[0]);
		bt_a2dp_start(default_a2dp_endpoint);
		printk("a2dp start playing\r\n");
	}
}

void app_configured(int err)
{
	if (err) {
		printk("configuring fail\r\n");
	}
}

void app_connected(struct bt_a2dp *a2dp, int err)
{
	if (!err) {
		bt_a2dp_configure(a2dp, app_configured);
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

static void app_a2dp_init(void)
{
	struct bt_a2dp_connect_cb connect_cb;

	connect_cb.connected = app_connected;
	connect_cb.disconnected = app_disconnected;

	sbc_endpoint.control_cbs.start_play = music_control_a2dp_start_callback;
	sbc_endpoint.control_cbs.configured = app_endpoint_configured;
	bt_a2dp_register_endpoint(&sbc_endpoint, BT_A2DP_AUDIO, BT_A2DP_SOURCE);

	bt_a2dp_register_connect_callback(&connect_cb);
}

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	app_connect_init();
	app_a2dp_init();
}

void main(void)
{
	int err;

	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}
}
