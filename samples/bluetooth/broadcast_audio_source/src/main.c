/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>

/* When BROADCAST_ENQUEUE_COUNT > 1 we can enqueue enough buffers to ensure that
 * the controller is never idle
 */
#define BROADCAST_ENQUEUE_COUNT 2U
#define TOTAL_BUF_NEEDED (BROADCAST_ENQUEUE_COUNT * CONFIG_BT_AUDIO_BROADCAST_SRC_STREAM_COUNT)

BUILD_ASSERT(CONFIG_BT_ISO_TX_BUF_COUNT >= TOTAL_BUF_NEEDED,
	     "CONFIG_BT_ISO_TX_BUF_COUNT should be at least "
	     "BROADCAST_ENQUEUE_COUNT * CONFIG_BT_AUDIO_BROADCAST_SRC_STREAM_COUNT");

static struct bt_audio_lc3_preset preset_16_2_1 =
	BT_AUDIO_LC3_BROADCAST_PRESET_16_2_1(BT_AUDIO_LOCATION_FRONT_LEFT,
					     BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
static struct bt_audio_stream streams[CONFIG_BT_AUDIO_BROADCAST_SRC_STREAM_COUNT];
static struct bt_audio_broadcast_source *broadcast_source;

NET_BUF_POOL_FIXED_DEFINE(tx_pool,
			  TOTAL_BUF_NEEDED,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU), 8, NULL);
static uint8_t mock_data[CONFIG_BT_ISO_TX_MTU];
static uint32_t seq_num;
static bool stopping;

static K_SEM_DEFINE(sem_started, 0U, ARRAY_SIZE(streams));
static K_SEM_DEFINE(sem_stopped, 0U, ARRAY_SIZE(streams));

#define BROADCAST_SOURCE_LIFETIME  30U /* seconds */

static void stream_started_cb(struct bt_audio_stream *stream)
{
	k_sem_give(&sem_started);
}

static void stream_stopped_cb(struct bt_audio_stream *stream)
{
	k_sem_give(&sem_stopped);
}

static void stream_sent_cb(struct bt_audio_stream *stream)
{
	static uint32_t sent_cnt;
	struct net_buf *buf;
	int ret;

	if (stopping) {
		return;
	}

	buf = net_buf_alloc(&tx_pool, K_FOREVER);
	if (buf == NULL) {
		printk("Could not allocate buffer when sending on %p\n",
		       stream);
		return;
	}

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, mock_data, preset_16_2_1.qos.sdu);
	ret = bt_audio_stream_send(stream, buf, seq_num++,
				   BT_ISO_TIMESTAMP_NONE);
	if (ret < 0) {
		/* This will end broadcasting on this stream. */
		printk("Unable to broadcast data on %p: %d\n", stream, ret);
		net_buf_unref(buf);
		return;
	}

	sent_cnt++;
	if ((sent_cnt % 1000U) == 0U) {
		printk("Sent %u total ISO packets\n", sent_cnt);
	}
}

static struct bt_audio_stream_ops stream_ops = {
	.started = stream_started_cb,
	.stopped = stream_stopped_cb,
	.sent = stream_sent_cb
};

void main(void)
{
	struct bt_audio_stream *streams_p[ARRAY_SIZE(streams)];
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}
	printk("Bluetooth initialized\n");

	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		streams[i].ops = &stream_ops;
		streams_p[i] = &streams[i];
	}

	for (size_t i = 0U; i < ARRAY_SIZE(mock_data); i++) {
		/* Initialize mock data */
		mock_data[i] = i;
	}

	while (true) {
		printk("Creating broadcast source\n");
		err = bt_audio_broadcast_source_create(streams_p,
						       ARRAY_SIZE(streams_p),
						       &preset_16_2_1.codec,
						       &preset_16_2_1.qos,
						       &broadcast_source);
		if (err != 0) {
			printk("Unable to create broadcast source: %d\n", err);
			return;
		}

		printk("Starting broadcast source\n");
		stopping = false;
		err = bt_audio_broadcast_source_start(broadcast_source);
		if (err != 0) {
			printk("Unable to start broadcast source: %d\n", err);
			return;
		}

		/* Wait for all to be started */
		for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
			k_sem_take(&sem_started, K_FOREVER);
		}
		printk("Broadcast source started\n");

		/* Initialize sending */
		for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
			for (unsigned int j = 0U; j < BROADCAST_ENQUEUE_COUNT; j++) {
				stream_sent_cb(&streams[i]);
			}
		}

		printk("Waiting %u seconds before stopped\n",
		       BROADCAST_SOURCE_LIFETIME);
		k_sleep(K_SECONDS(BROADCAST_SOURCE_LIFETIME));

		printk("Stopping broadcast source\n");
		stopping = true;
		err = bt_audio_broadcast_source_stop(broadcast_source);
		if (err != 0) {
			printk("Unable to stop broadcast source: %d\n", err);
			return;
		}

		/* Wait for all to be stopped */
		for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
			k_sem_take(&sem_stopped, K_FOREVER);
		}
		printk("Broadcast source stopped\n");

		printk("Deleting broadcast source\n");
		err = bt_audio_broadcast_source_delete(broadcast_source);
		if (err != 0) {
			printk("Unable to delete broadcast source: %d\n", err);
			return;
		}
		printk("Broadcast source deleted\n");
		broadcast_source = NULL;
		seq_num = 0;
	}
}
