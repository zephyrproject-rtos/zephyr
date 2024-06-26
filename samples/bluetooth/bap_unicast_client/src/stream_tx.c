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
#include <zephyr/net/buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/types.h>

#include "stream_lc3.h"
#include "stream_tx.h"

LOG_MODULE_REGISTER(stream_tx, LOG_LEVEL_INF);

static struct tx_stream tx_streams[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT];

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
		false;
	}

	return ep_info.state == BT_BAP_EP_STATE_STREAMING;
}

static void tx_thread_func(void *arg1, void *arg2, void *arg3)
{
	NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_ISO_TX_BUF_COUNT,
				  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
				  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
	static uint8_t mock_data[CONFIG_BT_ISO_TX_MTU];

	for (size_t i = 0U; i < ARRAY_SIZE(mock_data); i++) {
		mock_data[i] = (uint8_t)i;
	}

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

			if (stream_is_streaming(bap_stream)) {
				struct net_buf *buf;

				buf = net_buf_alloc(&tx_pool, K_FOREVER);
				net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

				if (IS_ENABLED(CONFIG_LIBLC3) &&
				    bap_stream->codec_cfg->id == BT_HCI_CODING_FORMAT_LC3) {
					stream_lc3_add_data(&tx_streams[i], buf);
				} else {
					net_buf_add_mem(buf, mock_data, bap_stream->qos->sdu);
				}

				err = bt_bap_stream_send(bap_stream, buf, tx_streams[i].seq_num);
				if (err == 0) {
					tx_streams[i].seq_num++;
				} else {
					LOG_ERR("Unable to send: %d", err);
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

int stream_tx_register(struct bt_bap_stream *bap_stream)
{
	if (bap_stream == NULL) {
		return -EINVAL;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(tx_streams); i++) {
		if (tx_streams[i].bap_stream == NULL) {
			tx_streams[i].bap_stream = bap_stream;
			tx_streams[i].seq_num = 0U;

			if (IS_ENABLED(CONFIG_LIBLC3) &&
			    bap_stream->codec_cfg->id == BT_HCI_CODING_FORMAT_LC3) {
				const int err = stream_lc3_init(&tx_streams[i]);

				if (err != 0) {
					tx_streams[i].bap_stream = NULL;

					return err;
				}
			}

			LOG_INF("Registered %p for TX", bap_stream);

			return 0;
		}
	}

	return -ENOMEM;
}

int stream_tx_unregister(struct bt_bap_stream *bap_stream)
{
	if (bap_stream == NULL) {
		return -EINVAL;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(tx_streams); i++) {
		if (tx_streams[i].bap_stream == bap_stream) {
			tx_streams[i].bap_stream = NULL;

			LOG_INF("Unregistered %p for TX", bap_stream);

			return 0;
		}
	}

	return -ENODATA;
}

void stream_tx_init(void)
{
	static bool thread_started;

	if (!thread_started) {
		static K_KERNEL_STACK_DEFINE(tx_thread_stack,
					     IS_ENABLED(CONFIG_LIBLC3) ? 4096U : 1024U);
		const int tx_thread_prio = K_PRIO_PREEMPT(5);
		static struct k_thread tx_thread;

		k_thread_create(&tx_thread, tx_thread_stack, K_KERNEL_STACK_SIZEOF(tx_thread_stack),
				tx_thread_func, NULL, NULL, NULL, tx_thread_prio, 0, K_NO_WAIT);
		k_thread_name_set(&tx_thread, "TX thread");
		thread_started = true;
	}
}
