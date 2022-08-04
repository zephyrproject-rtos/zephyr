/*
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_AUDIO_BROADCAST_SOURCE)

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include "common.h"

/* When BROADCAST_ENQUEUE_COUNT > 1 we can enqueue enough buffers to ensure that
 * the controller is never idle
 */
#define BROADCAST_ENQUEUE_COUNT 2U
#define TOTAL_BUF_NEEDED (BROADCAST_ENQUEUE_COUNT * CONFIG_BT_AUDIO_BROADCAST_SRC_STREAM_COUNT)

BUILD_ASSERT(CONFIG_BT_ISO_TX_BUF_COUNT >= TOTAL_BUF_NEEDED,
	     "CONFIG_BT_ISO_TX_BUF_COUNT should be at least "
	     "BROADCAST_ENQUEUE_COUNT * CONFIG_BT_AUDIO_BROADCAST_SRC_STREAM_COUNT");

NET_BUF_POOL_FIXED_DEFINE(tx_pool,
			  TOTAL_BUF_NEEDED,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU), 8, NULL);

extern enum bst_result_t bst_result;
static struct bt_audio_stream broadcast_source_streams[CONFIG_BT_AUDIO_BROADCAST_SRC_STREAM_COUNT];
static struct bt_audio_stream *streams[ARRAY_SIZE(broadcast_source_streams)];
static struct bt_audio_lc3_preset preset_16_2_1 =
	BT_AUDIO_LC3_BROADCAST_PRESET_16_2_1(BT_AUDIO_LOCATION_FRONT_LEFT,
					     BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
static struct bt_audio_lc3_preset preset_16_2_2 =
	BT_AUDIO_LC3_BROADCAST_PRESET_16_2_2(BT_AUDIO_LOCATION_FRONT_LEFT,
					     BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
CREATE_FLAG(flag_stopping);

static K_SEM_DEFINE(sem_started, 0U, ARRAY_SIZE(streams));
static K_SEM_DEFINE(sem_stopped, 0U, ARRAY_SIZE(streams));

static void started_cb(struct bt_audio_stream *stream)
{
	printk("Stream %p started\n", stream);
	k_sem_give(&sem_started);
}

static void stopped_cb(struct bt_audio_stream *stream)
{
	printk("Stream %p stopped\n", stream);
	k_sem_give(&sem_stopped);
}

static void sent_cb(struct bt_audio_stream *stream)
{
	static uint8_t mock_data[CONFIG_BT_ISO_TX_MTU];
	static bool mock_data_initialized;
	static uint16_t seq_num;
	struct net_buf *buf;
	int ret;

	if (TEST_FLAG(flag_stopping)) {
		return;
	}

	if (!mock_data_initialized) {
		for (size_t i = 0U; i < ARRAY_SIZE(mock_data); i++) {
			/* Initialize mock data */
			mock_data[i] = (uint8_t)i;
		}
		mock_data_initialized = true;
	}

	buf = net_buf_alloc(&tx_pool, K_FOREVER);
	if (buf == NULL) {
		printk("Could not allocate buffer when sending on %p\n",
		       stream);
		return;
	}

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
	/* Use preset_16_2_1 as that is the config we end up using */
	net_buf_add_mem(buf, mock_data, preset_16_2_1.qos.sdu);
	ret = bt_audio_stream_send(stream, buf, seq_num++,
				   BT_ISO_TIMESTAMP_NONE);
	if (ret < 0) {
		/* This will end broadcasting on this stream. */
		printk("Unable to broadcast data on %p: %d\n", stream, ret);
		net_buf_unref(buf);
		return;
	}
}

static struct bt_audio_stream_ops stream_ops = {
	.started = started_cb,
	.stopped = stopped_cb,
	.sent = sent_cb
};

static void test_main(void)
{
	int err;
	struct bt_audio_broadcast_source *source;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	(void)memset(broadcast_source_streams, 0,
		     sizeof(broadcast_source_streams));

	for (size_t i = 0; i < ARRAY_SIZE(streams); i++) {
		streams[i] = &broadcast_source_streams[i];
		bt_audio_stream_cb_register(streams[i], &stream_ops);
	}

	printk("Creating broadcast source with %zu streams\n", ARRAY_SIZE(streams));
	err = bt_audio_broadcast_source_create(streams, ARRAY_SIZE(streams),
					       &preset_16_2_2.codec,
					       &preset_16_2_2.qos,
					       &source);
	if (err != 0) {
		FAIL("Unable to create broadcast source: %d\n", err);
		return;
	}

	printk("Reconfiguring broadcast source\n");
	err = bt_audio_broadcast_source_reconfig(source, &preset_16_2_1.codec,
						 &preset_16_2_1.qos);
	if (err != 0) {
		FAIL("Unable to reconfigure broadcast source: %d\n", err);
		return;
	}

	printk("Starting broadcast source\n");
	err = bt_audio_broadcast_source_start(source);
	if (err != 0) {
		FAIL("Unable to start broadcast source: %d\n", err);
		return;
	}

	/* Wait for all to be started */
	printk("Waiting for streams to be started\n");
	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		k_sem_take(&sem_started, K_FOREVER);
	}

	/* Initialize sending */
	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		for (unsigned int j = 0U; j < BROADCAST_ENQUEUE_COUNT; j++) {
			sent_cb(streams[i]);
		}
	}

	/* Keeping running for a little while */
	k_sleep(K_SECONDS(10));

	printk("Stopping broadcast source\n");
	SET_FLAG(flag_stopping);
	err = bt_audio_broadcast_source_stop(source);
	if (err != 0) {
		FAIL("Unable to stop broadcast source: %d\n", err);
		return;
	}

	/* Wait for all to be stopped */
	printk("Waiting for streams to be stopped\n");
	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		k_sem_take(&sem_stopped, K_FOREVER);
	}

	printk("Deleting broadcast source\n");
	err = bt_audio_broadcast_source_delete(source);
	if (err != 0) {
		FAIL("Unable to delete broadcast source: %d\n", err);
		return;
	}
	source = NULL;

	/* Recreate broadcast source to verify that it's possible */
	printk("Recreating broadcast source\n");
	err = bt_audio_broadcast_source_create(streams, ARRAY_SIZE(streams),
					       &preset_16_2_1.codec,
					       &preset_16_2_1.qos,
					       &source);
	if (err != 0) {
		FAIL("Unable to create broadcast source: %d\n", err);
		return;
	}

	printk("Deleting broadcast source\n");
	err = bt_audio_broadcast_source_delete(source);
	if (err != 0) {
		FAIL("Unable to delete broadcast source: %d\n", err);
		return;
	}
	source = NULL;

	PASS("Broadcast source passed\n");
}

static const struct bst_test_instance test_broadcast_source[] = {
	{
		.test_id = "broadcast_source",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_broadcast_source_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_broadcast_source);
}

#else /* CONFIG_BT_AUDIO_BROADCAST_SOURCE */

struct bst_test_list *test_broadcast_source_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_AUDIO_BROADCAST_SOURCE */
