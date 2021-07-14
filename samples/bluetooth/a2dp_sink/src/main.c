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
#include <bluetooth/sdp.h>
#include <bluetooth/a2dp_codec_sbc.h>
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

struct bt_a2dp *default_a2dp;
struct bt_a2dp_endpoint *default_endpoint;
static uint32_t audio_start;
static __nocache __aligned(4) uint8_t a2dp_silence_data[48*2*2*2];
BT_A2DP_SBC_SINK_ENDPOINT(sbc_endpoint);

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

void board_media_data_played(void)
{
	bt_a2dp_snk_media_sync(default_endpoint, NULL, 0U);
}

void sbc_configured(struct bt_a2dp_endpoint_configure_result *config)
{
	if (config->err == 0) {
		default_endpoint = &sbc_endpoint;
		board_codec_set(
			bt_a2dp_sbc_get_sampling_frequency(
				(struct bt_a2dp_codec_sbc_params *)&config->config.media_config->codec_ie[0]),
			bt_a2dp_sbc_get_channel_num(
				(struct bt_a2dp_codec_sbc_params *)&config->config.media_config->codec_ie[0]));
	}
}

void sbc_start_play(int err)
{
	if (err == 0) {
		audio_start = 1;
		/* Start Audio Player */
		board_codec_start();
		printk("a2dp start playing\r\n");
	}
}

void sbc_stop_play(int err)
{
	if (err == 0) {
		audio_start = 0;
		/* Stop Audio Player */
		board_codec_stop();
		printk("a2dp stop playing\r\n");
	}
}

void sbc_streamer_data(uint8_t *data, uint32_t length)
{
	if ((data != NULL) && (length != 0U)) {
		if (audio_start == 0) {
			return;
		}
		board_codec_media_play(data, length);
	} else {
		board_codec_media_play(a2dp_silence_data, sizeof(a2dp_silence_data));
	}
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
	/* Stop Audio Player */
	board_codec_stop();
}

static void app_edgefast_a2dp_init(void)
{
	struct bt_a2dp_connect_cb connectCb;

	connectCb.connected = app_connected;
	connectCb.disconnected = app_disconnected;

	sbc_endpoint.control_cbs.configured = sbc_configured;
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

void main(void)
{
	int err;

	board_codec_init();
	/* Initializate BT Host stack */
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}
}
