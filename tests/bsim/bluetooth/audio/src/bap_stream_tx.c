/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <autoconf.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/atomic_types.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/types.h>

#include "bap_stream_tx.h"
#include "common.h"

/** Enqueue at least 2 per stream, but otherwise equal distribution based on the buf count */
#define ENQUEUE_CNT MAX(2, (CONFIG_BT_ISO_TX_BUF_COUNT / CONFIG_BT_ISO_MAX_CHAN))

LOG_MODULE_REGISTER(bap_stream_tx, LOG_LEVEL_INF);

struct tx_stream {
	struct bt_bap_stream *bap_stream;
	uint16_t seq_num;
	atomic_t enqueued;
};

static struct tx_stream tx_streams[CONFIG_BT_ISO_MAX_CHAN];

static bool stream_is_streaming(const struct bt_bap_stream *bap_stream)
{
	struct bt_bap_ep_info ep_info;
	int err;

	if (bap_stream == NULL) {
		return false;
	}

	/* No-op if stream is not configured */
	if (bap_stream->ep == NULL) {
		return false;
	}

	err = bt_bap_ep_get_info(bap_stream->ep, &ep_info);
	if (err != 0) {
		return false;
	}

	if (ep_info.iso_chan == NULL || ep_info.iso_chan->state != BT_ISO_STATE_CONNECTED) {
		return false;
	}

	return ep_info.state == BT_BAP_EP_STATE_STREAMING;
}

static void tx_thread_func(void *arg1, void *arg2, void *arg3)
{
	NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_ISO_TX_BUF_COUNT,
				  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
				  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

	/* This loop will attempt to send on all streams in the streaming state in a round robin
	 * fashion.
	 * The TX is controlled by the number of buffers configured, and increasing
	 * CONFIG_BT_ISO_TX_BUF_COUNT will allow for more streams in parallel, or to submit more
	 * buffers per stream.
	 * Once a buffer has been freed by the stack, it triggers the next TX.
	 */
	while (true) {
		int err = -ENOEXEC;

		for (size_t i = 0U; i < ARRAY_SIZE(tx_streams); i++) {
			struct bt_bap_stream *bap_stream = tx_streams[i].bap_stream;

			if (stream_is_streaming(bap_stream) &&
			    atomic_get(&tx_streams[i].enqueued) < ENQUEUE_CNT) {
				struct net_buf *buf;

				buf = net_buf_alloc(&tx_pool, K_FOREVER);
				net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

				net_buf_add_mem(buf, mock_iso_data, bap_stream->qos->sdu);

				err = bt_bap_stream_send(bap_stream, buf, tx_streams[i].seq_num);
				if (err == 0) {
					tx_streams[i].seq_num++;
					atomic_inc(&tx_streams[i].enqueued);
				} else {
					if (!stream_is_streaming(bap_stream)) {
						/* Can happen if we disconnected while waiting for a
						 * buffer - Ignore
						 */
					} else {
						FAIL("Unable to send: %d", err);
					}

					net_buf_unref(buf);
				}
			} /* No-op if stream is not streaming */
		}

		if (err != 0) {
			/* In case of any errors, retry with a delay */
			k_sleep(K_MSEC(10));
		}
	}
}

int bap_stream_tx_register(struct bt_bap_stream *bap_stream)
{
	if (bap_stream == NULL) {
		return -EINVAL;
	}

	if (!bap_stream_tx_can_send(bap_stream)) {
		return -EINVAL;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(tx_streams); i++) {
		if (tx_streams[i].bap_stream == NULL) {
			tx_streams[i].bap_stream = bap_stream;
			tx_streams[i].seq_num = 0U;

			LOG_INF("Registered %p for TX", bap_stream);

			return 0;
		}
	}

	return -ENOMEM;
}

int bap_stream_tx_unregister(struct bt_bap_stream *bap_stream)
{
	if (bap_stream == NULL) {
		return -EINVAL;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(tx_streams); i++) {
		if (tx_streams[i].bap_stream == bap_stream) {
			tx_streams[i].bap_stream = NULL;
			atomic_set(&tx_streams[i].enqueued, 0);

			LOG_INF("Unregistered %p for TX", bap_stream);

			return 0;
		}
	}

	return -ENODATA;
}

void bap_stream_tx_init(void)
{
	static bool thread_started;

	if (!thread_started) {
		static K_KERNEL_STACK_DEFINE(tx_thread_stack, 1024U);
		const int tx_thread_prio = K_PRIO_PREEMPT(5);
		static struct k_thread tx_thread;

		k_thread_create(&tx_thread, tx_thread_stack, K_KERNEL_STACK_SIZEOF(tx_thread_stack),
				tx_thread_func, NULL, NULL, NULL, tx_thread_prio, 0, K_NO_WAIT);
		k_thread_name_set(&tx_thread, "TX thread");
		thread_started = true;
	}
}

bool bap_stream_tx_can_send(const struct bt_bap_stream *stream)
{
	struct bt_bap_ep_info info;
	int err;

	if (stream == NULL || stream->ep == NULL) {
		return false;
	}

	err = bt_bap_ep_get_info(stream->ep, &info);
	if (err != 0) {
		return false;
	}

	return info.can_send;
}

void bap_stream_tx_sent_cb(struct bt_bap_stream *stream)
{
	struct audio_test_stream *test_stream = audio_test_stream_from_bap_stream(stream);

	if ((test_stream->tx_cnt % 100U) == 0U) {
		LOG_INF("Stream %p sent %zu SDUs", stream, test_stream->tx_cnt);
	}

	test_stream->tx_cnt++;

	for (size_t i = 0U; i < ARRAY_SIZE(tx_streams); i++) {
		if (tx_streams[i].bap_stream == stream) {
			const atomic_val_t old = atomic_dec(&tx_streams[i].enqueued);

			if (old == 0) {
				FAIL("Old enqueue count was 0");
			}
		}
	}
}
