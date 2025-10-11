/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/types.h>

#include "cap_stream_tx.h"

LOG_MODULE_REGISTER(cap_stream_tx, LOG_LEVEL_INF);

/** Mutex to prevent race conditions as the values are accessed by multiple threads */
#define TX_MUTEX_TIMEOUT K_MSEC(1000)
static K_MUTEX_DEFINE(tx_mutex);

struct tx_stream tx_streams[CAP_STREAM_TX_MAX];

static void tx_thread_func(void *arg1, void *arg2, void *arg3)
{
	NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_ISO_TX_BUF_COUNT,
				  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
				  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
	static uint8_t data[CONFIG_BT_ISO_TX_MTU];

	for (size_t i = 0U; i < ARRAY_SIZE(data); i++) {
		data[i] = (uint8_t)i;
	}

	while (true) {
		bool delay_and_retry = true;
		int err;

		err = k_mutex_lock(&tx_mutex, TX_MUTEX_TIMEOUT);
		if (err != 0) {
			goto delay_and_retry;
		}

		for (size_t i = 0U; i < ARRAY_SIZE(tx_streams); i++) {
			const struct bt_bap_stream *bap_stream;
			struct bt_cap_stream *cap_stream;
			struct net_buf *buf;

			cap_stream = tx_streams[i].stream;
			if (cap_stream == NULL) {
				continue;
			}

			bap_stream = &cap_stream->bap_stream;

			/* No-op if stream is not configured */
			if (bap_stream->ep == NULL) {
				continue;
			}

			buf = net_buf_alloc(&tx_pool, K_FOREVER);
			net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

			net_buf_add_mem(buf, data, bap_stream->qos->sdu);

			err = bt_cap_stream_send(cap_stream, buf, tx_streams[i].seq_num);
			if (err == 0) {
				tx_streams[i].seq_num++;
				delay_and_retry = false;
			} else {
				/* -EBADMSG may be returned if the stream has disconnected, which is
				 * a valid case that we can handle
				 */
				if (err != -EBADMSG) {
					LOG_ERR("Unable to send: %d", err);
				}
				net_buf_unref(buf);
			}
		}

		err = k_mutex_unlock(&tx_mutex);
		__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

delay_and_retry:
		if (delay_and_retry) {
			/* In case of any errors or nothing sent, retry with a delay */
			k_sleep(K_MSEC(100));
		}
	}
}

int cap_stream_tx_register_stream(struct bt_cap_stream *cap_stream)
{
	int ret;
	int err;

	if (cap_stream == NULL) {
		return -EINVAL;
	}

	err = k_mutex_lock(&tx_mutex, TX_MUTEX_TIMEOUT);
	if (err != 0) {
		LOG_ERR("Failed to take mutex: %d", err);
		return err;
	}

	ret = -ENOMEM;
	for (size_t i = 0U; i < ARRAY_SIZE(tx_streams); i++) {
		if (tx_streams[i].stream == NULL) {
			tx_streams[i].stream = cap_stream;
			tx_streams[i].seq_num = 0U;
			ret = 0;

			break;
		}
	}

	err = k_mutex_unlock(&tx_mutex);
	__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

	return ret;
}

int cap_stream_tx_unregister_stream(struct bt_cap_stream *cap_stream)
{
	int ret;
	int err;

	if (cap_stream == NULL) {
		return -EINVAL;
	}

	err = k_mutex_lock(&tx_mutex, TX_MUTEX_TIMEOUT);
	if (err != 0) {
		LOG_ERR("Failed to take mutex: %d", err);
		return err;
	}

	ret = -EALREADY;
	for (size_t i = 0U; i < ARRAY_SIZE(tx_streams); i++) {
		if (tx_streams[i].stream == cap_stream) {
			tx_streams[i].stream = NULL;

			ret = 0;
			break;
		}
	}

	err = k_mutex_unlock(&tx_mutex);
	__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

	return ret;
}

void cap_stream_tx_init(void)
{
	static bool thread_started;

	if (!thread_started) {
		static K_KERNEL_STACK_DEFINE(tx_thread_stack, 1024);
		const int tx_thread_prio = K_PRIO_PREEMPT(5);
		static struct k_thread tx_thread;

		k_thread_create(&tx_thread, tx_thread_stack, K_KERNEL_STACK_SIZEOF(tx_thread_stack),
				tx_thread_func, NULL, NULL, NULL, tx_thread_prio, 0, K_NO_WAIT);
		k_thread_name_set(&tx_thread, "TX thread");
		thread_started = true;
	}
}
