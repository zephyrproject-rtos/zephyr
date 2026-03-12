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
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/sbc.h>
#include <zephyr/bluetooth/classic/a2dp_codec_sbc.h>
#include <zephyr/bluetooth/classic/a2dp.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/settings/settings.h>
#include "sine.h"

static struct bt_br_discovery_param br_discover;
static struct bt_br_discovery_result scan_result[CONFIG_BT_A2DP_SOURCE_DISCOVER_RESULT_COUNT];
struct k_work discover_work;

static struct bt_conn *default_conn;
struct bt_a2dp *default_a2dp;
static struct bt_a2dp_stream sbc_stream;
struct sbc_encoder encoder;

BT_A2DP_SBC_SOURCE_EP_DEFAULT(sbc_source_ep);
static bool peer_sbc_found;
struct bt_a2dp_codec_ie sbc_sink_ep_capabilities;
static struct bt_a2dp_ep sbc_sink_ep = {
	.codec_cap = &sbc_sink_ep_capabilities,
};
BT_A2DP_SBC_EP_CFG_DEFAULT(sbc_cfg_default, A2DP_SBC_SAMP_FREQ_44100);

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
NET_BUF_POOL_DEFINE(a2dp_tx_pool, CONFIG_BT_A2DP_SOURCE_DATA_BUF_COUNT,
		    BT_L2CAP_BUF_SIZE(CONFIG_BT_A2DP_SOURCE_DATA_BUF_SIZE),
		    CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
static void a2dp_playback_timeout_handler(struct k_timer *timer);
K_TIMER_DEFINE(a2dp_player_timer, a2dp_playback_timeout_handler, NULL);

NET_BUF_POOL_DEFINE(sdp_discover_pool, 10, BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
		    CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

struct k_work_q audio_play_work_q;
static K_KERNEL_STACK_DEFINE(audio_play_work_q_thread_stack,
			     CONFIG_BT_A2DP_SOURCE_DATA_SEND_WORKQ_STACK_SIZE);

#define A2DP_VERSION 0x0104

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

static bool a2dp_produce_media_check(uint32_t samples_num)
{
	uint16_t  medialen;

	/* Music Audio is Stereo */
	medialen = (samples_num * a2dp_src_nc * 2);

	/* need to use a2dp_pcm_buffer to do memcpy */
	if ((a2dp_src_nc == 1) || ((media_index + (samples_num << 2)) > sizeof(media_data))) {
		if (medialen > sizeof(a2dp_pcm_buffer)) {
			return false;
		}
	}

	return true;
}

static uint8_t *a2dp_produce_media(uint32_t samples_num)
{
	uint8_t *media = NULL;
	uint16_t  medialen;

	/* Music Audio is Stereo */
	medialen = (samples_num * a2dp_src_nc * 2);

	/* For mono or dual configuration, skip alternative samples */
	if (a2dp_src_nc == 1) {
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

static void audio_play_check(void)
{
	uint32_t a2dp_src_num_samples;
	uint32_t pcm_frame_samples;
	uint8_t frame_num;
	struct net_buf *buf;
	uint32_t pdu_len;

	a2dp_src_num_samples = (uint16_t)((CONFIG_BT_A2DP_SOURCE_DATA_SEND_INTERVAL * a2dp_src_sf)
					  / 1000);
	pcm_frame_samples = sbc_frame_samples(&encoder);
	frame_num = a2dp_src_num_samples / pcm_frame_samples;
	if (frame_num * pcm_frame_samples < a2dp_src_num_samples) {
		frame_num++;
	}
	a2dp_src_num_samples = frame_num * pcm_frame_samples;

	buf = bt_a2dp_stream_create_pdu(&a2dp_tx_pool, K_FOREVER);
	pdu_len = frame_num * sbc_frame_encoded_bytes(&encoder);

	if (pdu_len > net_buf_tailroom(buf)) {
		printk("need increase buf size %d > %d\n", pdu_len, net_buf_tailroom(buf));
	}

	pdu_len += buf->len;
	if (pdu_len > bt_a2dp_get_mtu(&sbc_stream)) {
		printk("need decrease CONFIG_BT_A2DP_SOURCE_DATA_SEND_INTERVAL %d > %d\n", pdu_len,
			bt_a2dp_get_mtu(&sbc_stream));
	}

	if (!a2dp_produce_media_check(a2dp_src_num_samples)) {
		printk("need increase a2dp_pcm_buffer\n");
	}

	net_buf_unref(buf);
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
	uint8_t *sbc_hdr;
	uint32_t pdu_len;
	uint32_t out_size;
	int err;
	uint8_t frame_num = 0;
	uint8_t remaining_frame_num;

	/* If stopped then return */
	if (!a2dp_src_playback || default_a2dp == NULL) {
		return;
	}

	buf = bt_a2dp_stream_create_pdu(&a2dp_tx_pool, K_FOREVER);
	if (buf == NULL) {
		/* fail */
		printk("no buf\n");
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
	remaining_frame_num = a2dp_src_num_samples / pcm_frame_samples;

	pdu_len = buf->len + remaining_frame_num * encoded_frame_size;

	/* Raw adjust for the drift */
	while (a2dp_src_missed_count >= (1000 * pcm_frame_samples)) {
		pdu_len += encoded_frame_size;
		a2dp_src_num_samples += pcm_frame_samples;
		remaining_frame_num++;
		a2dp_src_missed_count -= (1000 * pcm_frame_samples);
	}

	do {
		frame_num = remaining_frame_num;
		/* adjust the buf size */
		while ((pdu_len - buf->len > net_buf_tailroom(buf)) ||
		       (pdu_len > bt_a2dp_get_mtu(&sbc_stream)) ||
		       (!a2dp_produce_media_check(a2dp_src_num_samples))) {
			pdu_len -= encoded_frame_size;
			a2dp_src_missed_count += 1000 * pcm_frame_samples;
			a2dp_src_num_samples -= pcm_frame_samples;
			frame_num--;
		}

		pcm_data = a2dp_produce_media(a2dp_src_num_samples);
		if (pcm_data == NULL) {
			net_buf_unref(buf);
			printk("no media data\n");
			return;
		}

		for (index = 0; index < frame_num; index++) {
			out_size = sbc_encode(&encoder,
					      (uint8_t *)&pcm_data[index * pcm_frame_size],
					      net_buf_tail(buf));
			if (encoded_frame_size != out_size) {
				printk("sbc encode fail\n");
				continue;
			}

			net_buf_add(buf, encoded_frame_size);
		}

		*sbc_hdr = (uint8_t)BT_A2DP_SBC_MEDIA_HDR_ENCODE(frame_num, 0, 0, 0);

		err = bt_a2dp_stream_send(&sbc_stream, buf, send_count, send_samples_count);
		if (err != 0) {
			net_buf_unref(buf);
			printk("  Failed to send SBC audio data on streams(%d)\n", err);
		}

		send_count++;
		send_samples_count += a2dp_src_num_samples;
		remaining_frame_num -= frame_num;
	} while (remaining_frame_num > 0);
}

static K_WORK_DEFINE(audio_work, audio_work_handler);

static void a2dp_playback_timeout_handler(struct k_timer *timer)
{
	k_work_submit_to_queue(&audio_play_work_q, &audio_work);
}

static void sbc_stream_configured(struct bt_a2dp_stream *stream)
{
	struct sbc_encoder_init_param param;
	struct bt_a2dp_codec_sbc_params *sbc_config = (struct bt_a2dp_codec_sbc_params *)
						      &sbc_cfg_default.codec_config->codec_ie[0];

	printk("stream configured\n");

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

	if (sbc_setup_encoder(&encoder, &param) != 0) {
		printk("sbc encoder initialization fail\n");
		return;
	}

	bt_a2dp_stream_establish(stream);
}

static void sbc_stream_established(struct bt_a2dp_stream *stream)
{
	printk("stream established\n");
	audio_play_check();
	bt_a2dp_stream_start(&sbc_stream);
}

static void sbc_stream_released(struct bt_a2dp_stream *stream)
{
	printk("stream released\n");
	k_timer_stop(&a2dp_player_timer);
}

static void sbc_stream_started(struct bt_a2dp_stream *stream)
{
	uint32_t audio_time_interval = CONFIG_BT_A2DP_SOURCE_DATA_SEND_INTERVAL;

	printk("stream started\n");
	/* Start Audio Source */
	a2dp_src_playback = true;

	k_uptime_delta(&ref_time);
	k_timer_start(&a2dp_player_timer, K_MSEC(audio_time_interval), K_MSEC(audio_time_interval));
}

static struct bt_a2dp_stream_ops sbc_stream_ops = {
	.configured = sbc_stream_configured,
	.established = sbc_stream_established,
	.released = sbc_stream_released,
	.started = sbc_stream_started,
};

static uint8_t a2dp_discover_ep_cb(struct bt_a2dp *a2dp, struct bt_a2dp_ep_info *info,
				   struct bt_a2dp_ep **ep)
{
	if (peer_sbc_found) {
		int err;

		bt_a2dp_stream_cb_register(&sbc_stream, &sbc_stream_ops);
		err = bt_a2dp_stream_config(a2dp, &sbc_stream,
						&sbc_source_ep, &sbc_sink_ep,
						&sbc_cfg_default);
		if (err != 0) {
			printk("fail to configure\n");
		}

		return BT_A2DP_DISCOVER_EP_STOP;
	}

	if (info != NULL) {
		printk("find one endpoint:");

		if (info->codec_type == BT_A2DP_SBC) {
			printk("it is SBC codec and use it\n");

			if (ep != NULL && !peer_sbc_found) {
				peer_sbc_found = true;
				*ep = &sbc_sink_ep;
			}
		} else {
			printk("it is not SBC codecs\n");
		}
	}

	return BT_A2DP_DISCOVER_EP_CONTINUE;
}

static struct bt_avdtp_sep_info found_seps[CONFIG_BT_A2DP_SOURCE_EPS_DISCOVER_COUNT];

static struct bt_a2dp_discover_param ep_discover_param = {
	.cb = a2dp_discover_ep_cb,
	.seps_info = &found_seps[0],
	.sep_count = CONFIG_BT_A2DP_SOURCE_EPS_DISCOVER_COUNT,
};

static void app_a2dp_connected(struct bt_a2dp *a2dp, int err)
{
	if (err == 0) {
		peer_sbc_found = false;

		err = bt_a2dp_discover(a2dp, &ep_discover_param);
		if (err != 0) {
			printk("fail to discover\n");
		}

		printk("a2dp connected success\n");
	} else {
		if (default_a2dp != NULL) {
			default_a2dp = NULL;
		}

		printk("a2dp connected fail\n");
	}
}

static void app_a2dp_disconnected(struct bt_a2dp *a2dp)
{
	if (default_a2dp != NULL) {
		default_a2dp = NULL;
	}

	a2dp_src_playback = false;
	/* stop timer */
	k_timer_stop(&a2dp_player_timer);
	printk("a2dp disconnected\n");
}

static uint8_t sdp_discover_cb(struct bt_conn *conn, struct bt_sdp_client_result *result,
			       const struct bt_sdp_discover_params *params)
{
	int err;
	uint16_t value;

	printk("Discover done\n");

	if (result != NULL && result->resp_buf != NULL) {
		err = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_L2CAP, &value);

		if (err != 0) {
			printk("PSM is not found\n");
		} else if (value == BT_UUID_AVDTP_VAL) {
			printk("The A2DP server found, connecting a2dp\n");
			default_a2dp = bt_a2dp_connect(conn);
			if (default_a2dp == NULL) {
				printk("Fail to create A2DP connection (err %d)\n", err);
			}
		}
	}

	return BT_SDP_DISCOVER_UUID_STOP;
}

static struct bt_sdp_discover_params sdp_discover = {
	.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR,
	.func = sdp_discover_cb,
	.pool = &sdp_discover_pool,
	.uuid = BT_UUID_DECLARE_16(BT_SDP_AUDIO_SINK_SVCLASS),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	int res;

	if (err != 0) {
		if (default_conn != NULL) {
			default_conn = NULL;
		}

		printk("Connection failed, err 0x%02x %s\n", err, bt_hci_err_to_str(err));
	} else {
		if (default_conn == conn) {
			struct bt_conn_info info;

			bt_conn_get_info(conn, &info);
			if (info.type != BT_CONN_TYPE_BR) {
				return;
			}

			/*
			 * Do an SDP Query on Successful ACL connection complete with the
			 * required device
			 */
			res = bt_sdp_discover(default_conn, &sdp_discover);
			if (res != 0) {
				printk("SDP discovery failed (err %d)\n", res);
			} else {
				printk("SDP discovery started\n");
			}
			printk("Connected\n");
		}
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected, reason 0x%02x %s\n", reason, bt_hci_err_to_str(reason));

	if (default_conn == conn) {
		default_conn = NULL;
	}
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	struct bt_conn_info info;

	bt_conn_get_info(conn, &info);

	bt_addr_to_str(info.br.dst, addr, sizeof(addr));

	printk("Security changed: %s level %u, err %s(%d)\n", addr, level,
	       bt_security_err_to_str(err), err);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

static void discovery_timeout_cb(const struct bt_br_discovery_result *results, size_t count)
{
	char addr[BT_ADDR_LE_STR_LEN];
	const uint8_t *eir;
	bool cod_a2dp = false;
	static uint8_t temp[240];
	size_t len = sizeof(results->eir);
	uint8_t major_device;
	uint8_t minor_device;
	size_t i;

	for (i = 0; i < count; i++) {
		bt_addr_to_str(&results[i].addr, addr, sizeof(addr));
		printk("Device[%d]: %s, rssi %d, cod 0x%02x%02x%02x", i, addr, results[i].rssi,
		       results[i].cod[0], results[i].cod[1], results[i].cod[2]);

		major_device = (uint8_t)BT_COD_MAJOR_DEVICE_CLASS(results[i].cod);
		minor_device = (uint8_t)BT_COD_MINOR_DEVICE_CLASS(results[i].cod);

		if ((major_device & BT_COD_MAJOR_DEVICE_CLASS_AUDIO_VIDEO) != 0 &&
		    (minor_device & BT_COD_MINOR_DEVICE_CLASS_WEARABLE_HEADSET) != 0) {
			cod_a2dp = true;
		}

		eir = results[i].eir;

		while ((eir[0] > 2) && (len > eir[0])) {
			switch (eir[1]) {
			case BT_DATA_NAME_SHORTENED:
			case BT_DATA_NAME_COMPLETE:
				memcpy(temp, &eir[2], eir[0] - 1);
				temp[eir[0] - 1] = '\0'; /* Set end flag */
				printk(", name %s", temp);
				break;
			default:
				/* Skip the EIR */
				break;
			}
			len = len - eir[0] - 1;
			eir = eir + eir[0] + 1;
		}
		printk("\n");

		if (cod_a2dp) {
			break;
		}
	}

	if (!cod_a2dp) {
		(void)k_work_submit(&discover_work);
	} else {
		(void)k_work_cancel(&discover_work);
		default_conn = bt_conn_create_br(&results[i].addr, BT_BR_CONN_PARAM_DEFAULT);

		if (default_conn == NULL) {
			printk("Fail to create the connection\n");
		} else {
			bt_conn_unref(default_conn);
		}
	}
}

static void discover_work_handler(struct k_work *work)
{
	int err;

	br_discover.length = 10;
	br_discover.limited = false;

	err = bt_br_discovery_start(&br_discover, scan_result,
				    CONFIG_BT_A2DP_SOURCE_DISCOVER_RESULT_COUNT);
	if (err != 0) {
		printk("Fail to start discovery (err %d)\n", err);
		return;
	}
}

static struct bt_br_discovery_cb discovery_cb = {
	.timeout = discovery_timeout_cb,
};

static struct bt_a2dp_cb a2dp_cb = {
	.connected = app_a2dp_connected,
	.disconnected = app_a2dp_disconnected,
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

	bt_conn_cb_register(&conn_callbacks);

	bt_br_discovery_cb_register(&discovery_cb);

	bt_sdp_register_service(&a2dp_source_rec);

	bt_a2dp_register_ep(&sbc_source_ep, BT_AVDTP_AUDIO, BT_AVDTP_SOURCE);

	bt_a2dp_register_cb(&a2dp_cb);

	k_work_queue_init(&audio_play_work_q);
	k_work_queue_start(&audio_play_work_q, audio_play_work_q_thread_stack,
			   CONFIG_BT_A2DP_SOURCE_DATA_SEND_WORKQ_STACK_SIZE,
			   K_PRIO_COOP(CONFIG_BT_A2DP_SOURCE_DATA_SEND_WORKQ_PRIORITY), NULL);
	k_thread_name_set(&audio_play_work_q.thread, "audio play");

	k_work_init(&discover_work, discover_work_handler);

	(void)k_work_submit(&discover_work);
}

int main(void)
{
	int err;

	printk("Bluetooth A2DP Source demo start...\n");

	err = bt_enable(bt_ready);
	if (err != 0) {
		printk("Bluetooth init failed (err %d)\n", err);
	}
	return 0;
}
