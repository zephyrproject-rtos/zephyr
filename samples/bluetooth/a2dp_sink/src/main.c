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
#include <zephyr/bluetooth/a2dp.h>
#include <zephyr/bluetooth/sdp.h>
#include <zephyr/libsbc/sbc.h>
#include <zephyr/bluetooth/a2dp_codec_sbc.h>
#include "app_connect.h"
#include "board_codec.h"

#define A2DP_CLASS_OF_DEVICE (0x200404U)
#define APP_A2DP_STREAMER_SYNC_TASK_PRIORITY (0U)
#define APP_A2DP_STREAMER_SYNC_TASK_STACK_SIZE (1024U)

extern void app_codec_stop(void);
extern void app_codec_start(void);
extern void app_codec_set(uint32_t sample_rate, uint8_t channels);
extern void app_codec_media_play(uint8_t *data, uint32_t length);
extern void app_codec_init(void);

#define A2DP_SBC_DATA_IND_COUNT (4)
#define A2DP_SBC_DATA_IND_SIZE (441 * 2 * 2)
#define A2DP_SBC_DECODER_PCM_BUFFER_SIZE  (A2DP_SBC_DATA_IND_SIZE * 73)
#define A2DP_SBC_ONE_FRAME_MAX_SIZE (2 * 2 * 16 * 8)

struct bt_a2dp *default_a2dp;
struct bt_a2dp_endpoint *default_endpoint;
uint8_t channel_num;
static uint32_t audio_start;
static uint8_t *audio_data_ind_buf[A2DP_SBC_DATA_IND_COUNT];
static uint8_t audio_data_ind_buf_w;
static uint8_t audio_data_ind_buf_r;
#if (defined CONFIG_BOARD_MIMXRT1060_EVK)
static __in_section_unique(dtcm_bss) __aligned(4) uint8_t a2dp_silence_data[A2DP_SBC_DATA_IND_SIZE];
#else
static __nocache __aligned(4) uint8_t a2dp_silence_data[A2DP_SBC_DATA_IND_SIZE];
#endif
BT_A2DP_SBC_SINK_ENDPOINT(sbc_endpoint);
K_SEM_DEFINE(audio_sem, 0, 50);

volatile bool sbc_first_data;
uint32_t sbc_expected_ts;
uint16_t pcm_frame_buffer[A2DP_SBC_ONE_FRAME_MAX_SIZE / 2];
#if (defined CONFIG_BOARD_MIMXRT1060_EVK)
static __in_section_unique(dtcm_bss) __aligned(4)
	uint8_t decoded_pcm_buf[A2DP_SBC_DECODER_PCM_BUFFER_SIZE];
#else
static __nocache __aligned(4) uint8_t decoded_pcm_buf[A2DP_SBC_DECODER_PCM_BUFFER_SIZE];
#endif
volatile uint32_t pcm_r;
volatile uint32_t pcm_w;
volatile uint32_t pcm_rm;
volatile uint32_t pcm_w_count;
volatile uint32_t pcm_r_count;
volatile uint32_t pcm_rm_count;
struct sbc_decoder sbc_decoder;

static struct bt_sdp_attribute a2dp_sink_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_AUDIO_SINK_SVCLASS)
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

static struct bt_sdp_record a2dp_sink_rec = BT_SDP_RECORD(a2dp_sink_attrs);

void a2dp_play_data(uint8_t *data, uint32_t length)
{
	audio_data_ind_buf[audio_data_ind_buf_w++ % A2DP_SBC_DATA_IND_COUNT] = data;
	if ((data != NULL) && (length != 0U)) {
		if (audio_start == 0) {
			return;
		}
		board_codec_media_play(data, length);
	} else {
		board_codec_media_play(a2dp_silence_data, sizeof(a2dp_silence_data));
	}
}

static void a2dp_sink_sbc_get_pcm_data(uint8_t **data, uint32_t *length)
{
	uint32_t data_space;

	uint32_t w_count = pcm_w_count;
	uint32_t r_count = pcm_r_count;

	if (w_count >= r_count) {
		data_space = w_count - r_count;
	} else {
		data_space = (uint32_t)(((uint64_t)w_count +
			(uint64_t)0x100000000u) - (uint64_t)r_count);
	}

	if (data_space < A2DP_SBC_DATA_IND_SIZE) {
		*data = NULL;
		*length = 0;
	} else {
		pcm_r_count += A2DP_SBC_DATA_IND_SIZE;
		*data = &decoded_pcm_buf[pcm_r];
		*length = A2DP_SBC_DATA_IND_SIZE;
		pcm_r += *length;
		if (pcm_r >= A2DP_SBC_DECODER_PCM_BUFFER_SIZE) {
			pcm_r = 0;
		}
	}
}

int bt_a2dp_snk_media_sync(uint8_t *data, uint16_t datalen)
{
	uint8_t *get_data;
	uint32_t length;


	if (data != NULL) {
		length = (datalen == 0) ? A2DP_SBC_DATA_IND_SIZE : datalen;
		pcm_rm += length;
		pcm_rm_count += length;
		if (pcm_rm >= A2DP_SBC_DECODER_PCM_BUFFER_SIZE) {
			pcm_rm = 0;
		}
	}
	/* give more data */
	a2dp_sink_sbc_get_pcm_data(&get_data, &length);
	a2dp_play_data(get_data, length);

	return 0;
}

static uint32_t a2dp_sink_pcm_buffer_free(void)
{
	uint32_t w_count = pcm_w_count;
	uint32_t rm_count = pcm_rm_count;

	if (w_count >= rm_count) {
		return A2DP_SBC_DECODER_PCM_BUFFER_SIZE - (w_count - rm_count);
	} else {
		return A2DP_SBC_DECODER_PCM_BUFFER_SIZE - ((uint32_t)(((uint64_t)w_count +
			(uint64_t)0x100000000u) - (uint64_t)rm_count));
	}
}

static uint32_t a2dp_sink_sbc_add_pcm_data(uint8_t *data, uint32_t length)
{
	uint32_t free_space;

	free_space = a2dp_sink_pcm_buffer_free();

	if (free_space < length) {
		length = free_space;
	}

	/* copy data to buffer */
	if ((pcm_w + length) <= A2DP_SBC_DECODER_PCM_BUFFER_SIZE) {
		if (data != NULL) {
			memcpy(&decoded_pcm_buf[pcm_w], data, length);
		} else {
			memset(&decoded_pcm_buf[pcm_w], 0, length);
		}
		pcm_w += length;
	} else {
		if (data != NULL) {
			memcpy(&decoded_pcm_buf[pcm_w], data,
				(A2DP_SBC_DECODER_PCM_BUFFER_SIZE - pcm_w));
			memcpy(&decoded_pcm_buf[0],
				&data[(A2DP_SBC_DECODER_PCM_BUFFER_SIZE - pcm_w)],
				length - (A2DP_SBC_DECODER_PCM_BUFFER_SIZE - pcm_w));
		} else {
			memset(&decoded_pcm_buf[pcm_w], 0,
				(A2DP_SBC_DECODER_PCM_BUFFER_SIZE - pcm_w));
			memset(&decoded_pcm_buf[0], 0,
				length - (A2DP_SBC_DECODER_PCM_BUFFER_SIZE - pcm_w));
		}
		pcm_w = length - (A2DP_SBC_DECODER_PCM_BUFFER_SIZE - pcm_w);
	}
	pcm_w_count += length;

	if (pcm_w == A2DP_SBC_DECODER_PCM_BUFFER_SIZE) {
		pcm_w = 0;
	}

	return length;
}

void sbc_process_buf(struct net_buf *buf, uint16_t seq_num, uint32_t ts)
{
	uint32_t samples_count = 0;
	uint32_t post_len;
	uint32_t pcm_data_len;
	uint32_t samples_lost = 0;
	struct bt_a2dp_codec_sbc_media_packet_hdr *sbc_hdr;
	int err;
	uint32_t put_len;

	if (sbc_first_data == 0) {
		sbc_first_data = 1;
		sbc_expected_ts = ts;
	} else {
		if (sbc_expected_ts != ts) {
			if (sbc_expected_ts < ts) {
				samples_lost = ts - sbc_expected_ts;
			}
		}
	}

	if (samples_lost != 0) {
		post_len = samples_lost * 2U * channel_num;
		(void)a2dp_sink_sbc_add_pcm_data(NULL, post_len);
		sbc_expected_ts = sbc_expected_ts + samples_lost;
	}

	sbc_hdr = net_buf_pull_mem(buf, sizeof(*sbc_hdr));

	post_len = buf->len;
	do {
		buf->data += (buf->len - post_len);
		buf->len = post_len;
		pcm_data_len = sizeof(pcm_frame_buffer);
		err = sbc_decode(&sbc_decoder, buf->data, &post_len, pcm_frame_buffer,
			&pcm_data_len);
		if (!err) {
			put_len = a2dp_sink_sbc_add_pcm_data((uint8_t *)pcm_frame_buffer,
				pcm_data_len);
		} else {
			printk("decode err");
			break;
		}

		if (put_len == pcm_data_len) {
			samples_count += (put_len / 2 / channel_num);
		}
	} while ((post_len > 3) && (post_len != buf->len));

	sbc_expected_ts = ts + samples_count;
}

void board_media_data_played(void)
{
	k_sem_give(&audio_sem);
}

void sbc_configured(struct bt_a2dp_endpoint_configure_result *config)
{
	if (config->err != 0) {
		return;
	}

	default_endpoint = &sbc_endpoint;
	channel_num = bt_a2dp_sbc_get_channel_num(
		(struct bt_a2dp_codec_sbc_params *)&config->config.media_config->codec_ie[0]);
	sbc_setup_decoder(&sbc_decoder);
	board_codec_configure(
		bt_a2dp_sbc_get_sampling_frequency(
		(struct bt_a2dp_codec_sbc_params *)&config->config.media_config->codec_ie[0]),
		channel_num);
}

void sbc_deconfigured(int err)
{
	if (err == 0) {
		audio_start = 0;
		/* Stop Audio Player */
		printk("a2dp deconfigure\r\n");
		board_codec_deconfigure();
	}
}

void sbc_start_play(int err)
{
	if (err == 0) {
		audio_start = 1;
		/* Start Audio Player */
		printk("a2dp start playing\r\n");
		pcm_r = 0;
		pcm_w = 0;
		pcm_rm = 0;
		pcm_w_count = 0;
		pcm_r_count = 0;
		pcm_rm_count = 0;
		sbc_first_data = 0;
		board_codec_start();

		for (uint8_t index = 0; index < A2DP_SBC_DATA_IND_COUNT; index++) {
			a2dp_play_data(a2dp_silence_data, sizeof(a2dp_silence_data));
		}
	}
}

void sbc_stop_play(int err)
{
	if (err == 0) {
		audio_start = 0;
		/* Stop Audio Player */
		printk("a2dp stop playing\r\n");
		board_codec_stop();
	}
}

void sbc_streamer_data(struct net_buf *buf, uint16_t seq_num, uint32_t ts)
{
	sbc_process_buf(buf, seq_num, ts);
}

void app_connected(struct bt_a2dp *a2dp, int err)
{
	if (!err) {
		default_a2dp = a2dp;
		printk("a2dp connected success\r\n");
	} else {
		printk("a2dp connected fail\r\n");
	}
}

void app_disconnected(struct bt_a2dp *a2dp)
{
	audio_start = 0;
}

static void app_edgefast_a2dp_init(void)
{
	struct bt_a2dp_connect_cb connectCb;

	connectCb.connected = app_connected;
	connectCb.disconnected = app_disconnected;

	sbc_endpoint.control_cbs.configured = sbc_configured;
	sbc_endpoint.control_cbs.deconfigured = sbc_deconfigured;
	sbc_endpoint.control_cbs.start_play = sbc_start_play;
	sbc_endpoint.control_cbs.stop_play = sbc_stop_play;
	sbc_endpoint.control_cbs.sink_streamer_data = sbc_streamer_data;
	bt_a2dp_register_endpoint(&sbc_endpoint, BT_A2DP_AUDIO, BT_A2DP_SINK);

	bt_a2dp_register_connect_callback(&connectCb);
}

static void bt_ready(int err)
{
	struct net_buf *buf = NULL;
	struct bt_hci_cp_write_class_of_device *cp;

	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	buf = bt_hci_cmd_create(BT_HCI_OP_WRITE_CLASS_OF_DEVICE, sizeof(*cp));
	if (buf != NULL) {
		cp = net_buf_add(buf, sizeof(*cp));
		sys_put_le24(A2DP_CLASS_OF_DEVICE, &cp->class_of_device[0]);
		err = bt_hci_cmd_send_sync(BT_HCI_OP_WRITE_CLASS_OF_DEVICE, buf, NULL);
	} else {
		err = -ENOBUFS;
	}

	if (err) {
		printk("setting class of device failed\n");
	}

	app_connect_init();

	err = bt_br_set_connectable(true);
	if (err) {
		printk("BR/EDR set/rest connectable failed (err %d)\n", err);
		return;
	}
	err = bt_br_set_discoverable(true);
	if (err) {
		printk("BR/EDR set discoverable failed (err %d)\n", err);
		return;
	}
	printk("BR/EDR set connectable and discoverable done\n");

	bt_sdp_register_service(&a2dp_sink_rec);
	app_edgefast_a2dp_init();
}

int main(void)
{
	int err;

	board_codec_init();
	/* Initializate BT Host stack */
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	} else {
		while (true) {
			if (!k_sem_take(&audio_sem, K_FOREVER)) {
				bt_a2dp_snk_media_sync(audio_data_ind_buf[audio_data_ind_buf_r++ %
						A2DP_SBC_DATA_IND_COUNT], 0U);
			}
		}
	}

	return err;
}
