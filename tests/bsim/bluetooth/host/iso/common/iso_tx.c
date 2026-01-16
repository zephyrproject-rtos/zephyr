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
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/atomic_types.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#include "babblekit/testcase.h"

#include "iso_tx.h"

/** Enqueue at least 2 per channel, but otherwise equal distribution based on the buf count */
#define ENQUEUE_CNT MAX(2, (CONFIG_BT_ISO_TX_BUF_COUNT / CONFIG_BT_ISO_MAX_CHAN))

/** Mutex to prevent race conditions as the values are accessed by multiple threads */
#define TX_MUTEX_TIMEOUT K_MSEC(1000)

LOG_MODULE_REGISTER(iso_tx, LOG_LEVEL_INF);

struct tx_stream {
	struct bt_iso_chan *iso_chan;
	struct k_mutex mutex;
	uint16_t seq_num;
	size_t tx_cnt;
	atomic_t enqueued;
};

static struct tx_stream tx_streams[CONFIG_BT_ISO_MAX_CHAN];

static void tx_thread_func(void *arg1, void *arg2, void *arg3)
{
	/* We set the SDU size to 3 x CONFIG_BT_ISO_TX_MTU to support the fragmentation tests */
	NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_ISO_TX_BUF_COUNT,
				  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU * 3),
				  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

	/* This loop will attempt to send on all streams in the streaming state in a round robin
	 * fashion.
	 * The TX is controlled by the number of buffers configured, and increasing
	 * CONFIG_BT_ISO_TX_BUF_COUNT will allow for more streams in parallel, or to submit more
	 * buffers per stream.
	 * Once a buffer has been freed by the stack, it triggers the next TX.
	 */
	while (true) {
		bool delay_and_retry = true;

		ARRAY_FOR_EACH_PTR(tx_streams, tx_stream) {
			struct bt_iso_chan *iso_chan;
			int err;

			err = k_mutex_lock(&tx_stream->mutex, K_NO_WAIT);
			if (err != 0) {
				continue;
			}

			iso_chan = tx_stream->iso_chan;
			if (iso_chan != NULL && iso_chan->state == BT_ISO_STATE_CONNECTED &&
			    atomic_get(&tx_stream->enqueued) < ENQUEUE_CNT) {
				/* Send between 1 and sdu number of octets */
				const size_t sdu = iso_chan->qos->tx->sdu;
				const size_t len_to_send = 1 + (tx_stream->tx_cnt % sdu);
				struct net_buf *buf;

				buf = net_buf_alloc(&tx_pool, TX_MUTEX_TIMEOUT);
				TEST_ASSERT(buf != NULL, "Failed to allocate buffer");

				net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

				TEST_ASSERT(len_to_send <= sizeof(mock_iso_data),
					    "Invalid len_to_send: %zu", len_to_send);
				net_buf_add_mem(buf, mock_iso_data, len_to_send);

				err = bt_iso_chan_send(iso_chan, buf, tx_stream->seq_num);
				if (err == 0) {
					tx_stream->seq_num++;
					atomic_inc(&tx_stream->enqueued);
					delay_and_retry = false;
				} else {
					if (iso_chan->state != BT_ISO_STATE_CONNECTED) {
						/* Can happen if we disconnected while waiting for a
						 * buffer - Ignore
						 */
					} else {
						TEST_FAIL("Unable to send: %d", err);
					}

					net_buf_unref(buf);
				}
			} /* No-op if stream is not streaming */

			err = k_mutex_unlock(&tx_stream->mutex);
			TEST_ASSERT(err == 0, "Failed to unlock mutex: %d", err);
		}

		if (delay_and_retry) {
			/* In case of any errors or nothing sent, retry with a delay */
			k_sleep(K_MSEC(5));
		}
	}
}

int iso_tx_register(struct bt_iso_chan *iso_chan)
{
	int err;

	if (iso_chan == NULL) {
		return -EINVAL;
	}

	if (!iso_tx_can_send(iso_chan)) {
		return -EINVAL;
	}

	ARRAY_FOR_EACH_PTR(tx_streams, tx_stream) {
		if (tx_stream->iso_chan == iso_chan) {
			return -EALREADY;
		}
	}

	err = -ENOMEM;
	ARRAY_FOR_EACH_PTR(tx_streams, tx_stream) {
		int mutex_err;

		mutex_err = k_mutex_lock(&tx_stream->mutex, TX_MUTEX_TIMEOUT);
		if (mutex_err != 0) {
			continue;
		}

		if (tx_stream->iso_chan == NULL) {
			tx_stream->iso_chan = iso_chan;
			tx_stream->seq_num = 0U;
			tx_stream->tx_cnt = 0U;
			atomic_set(&tx_stream->enqueued, 0);

			LOG_INF("Registered %p for TX", iso_chan);

			err = 0;
		}

		mutex_err = k_mutex_unlock(&tx_stream->mutex);
		TEST_ASSERT(mutex_err == 0, "Failed to unlock mutex: %d", mutex_err);

		if (err == 0) {
			break;
		}
	}

	return err;
}

int iso_tx_unregister(struct bt_iso_chan *iso_chan)
{
	int err;

	if (iso_chan == NULL) {
		return -EINVAL;
	}

	err = -ENODATA;
	ARRAY_FOR_EACH_PTR(tx_streams, tx_stream) {
		bool pending_tx = true;
		int mutex_err;

		while (pending_tx) {
			mutex_err = k_mutex_lock(&tx_stream->mutex, TX_MUTEX_TIMEOUT);
			if (mutex_err != 0) {
				continue;
			}

			if (tx_stream->iso_chan != iso_chan) {
				mutex_err = k_mutex_unlock(&tx_stream->mutex);
				TEST_ASSERT(mutex_err == 0, "Failed to unlock mutex: %d",
					    mutex_err);
				break; /* check next tx_stream */
			}

			if (atomic_get(&tx_stream->enqueued) == 0U) {
				tx_stream->iso_chan = NULL;
				pending_tx = false;
				err = 0;

				LOG_INF("Unregistered %p for TX", iso_chan);
			}

			mutex_err = k_mutex_unlock(&tx_stream->mutex);
			TEST_ASSERT(mutex_err == 0, "Failed to unlock mutex: %d", mutex_err);

			k_sleep(K_MSEC(100));
		}
	}

	return err;
}

void iso_tx_init(void)
{
	static bool thread_started;

	if (!thread_started) {
		static K_KERNEL_STACK_DEFINE(tx_thread_stack, 1024U);
		const int tx_thread_prio = K_PRIO_PREEMPT(5);
		static struct k_thread tx_thread;

		ARRAY_FOR_EACH_PTR(tx_streams, tx_stream) {
			const int err = k_mutex_init(&tx_stream->mutex);

			TEST_ASSERT(err == 0);
		}

		k_thread_create(&tx_thread, tx_thread_stack, K_KERNEL_STACK_SIZEOF(tx_thread_stack),
				tx_thread_func, NULL, NULL, NULL, tx_thread_prio, 0, K_NO_WAIT);
		k_thread_name_set(&tx_thread, "TX thread");
		thread_started = true;
	}
}

bool iso_tx_can_send(const struct bt_iso_chan *iso_chan)
{
	struct bt_iso_info info;
	int err;

	if (iso_chan == NULL || iso_chan->iso == NULL) {
		return false;
	}

	err = bt_iso_chan_get_info(iso_chan, &info);
	if (err != 0) {
		return false;
	}

	return info.can_send;
}

static void decrement_enqueued(struct bt_iso_chan *iso_chan)
{
	ARRAY_FOR_EACH_PTR(tx_streams, tx_stream) {
		int mutex_err;

		mutex_err = k_mutex_lock(&tx_stream->mutex, K_FOREVER);
		TEST_ASSERT(mutex_err == 0, "Failed to lock mutex: %d", mutex_err);

		if (tx_stream->iso_chan == iso_chan) {
			const atomic_val_t old = atomic_dec(&tx_stream->enqueued);

			TEST_ASSERT(old != 0, "Old enqueue count was 0");

			tx_stream->tx_cnt++;
			if ((tx_stream->tx_cnt % 100U) == 0U) {
				LOG_INF("Channel %p sent %zu SDUs", iso_chan, tx_stream->tx_cnt);
			}

			mutex_err = k_mutex_unlock(&tx_stream->mutex);
			TEST_ASSERT(mutex_err == 0, "Failed to unlock mutex: %d", mutex_err);

			return;
		}

		mutex_err = k_mutex_unlock(&tx_stream->mutex);
		TEST_ASSERT(mutex_err == 0, "Failed to unlock mutex: %d", mutex_err);
	}
}

void iso_tx_sent_cb(struct bt_iso_chan *iso_chan)
{
	decrement_enqueued(iso_chan);
}

void iso_tx_send_failed_cb(struct bt_iso_chan *iso_chan, int err)
{
	LOG_DBG("Send failed: %d", err);
	decrement_enqueued(iso_chan);
}

size_t iso_tx_get_sent_cnt(const struct bt_iso_chan *iso_chan)
{
	if (iso_chan == NULL) {
		return 0U;
	}

	ARRAY_FOR_EACH_PTR(tx_streams, tx_stream) {
		int mutex_err;

		mutex_err = k_mutex_lock(&tx_stream->mutex, K_FOREVER);
		TEST_ASSERT(mutex_err == 0, "Failed to lock mutex: %d", mutex_err);

		if (tx_stream->iso_chan == iso_chan) {
			const size_t tx_cnt = tx_stream->tx_cnt;

			mutex_err = k_mutex_unlock(&tx_stream->mutex);
			TEST_ASSERT(mutex_err == 0, "Failed to unlock mutex: %d", mutex_err);

			return tx_cnt;
		}

		mutex_err = k_mutex_unlock(&tx_stream->mutex);
		TEST_ASSERT(mutex_err == 0, "Failed to unlock mutex: %d", mutex_err);
	}

	return 0U;
}
