/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>

#include "cap_initiator.h"

LOG_MODULE_REGISTER(cap_initiator_tx, LOG_LEVEL_INF);

struct tx_stream
	tx_streams[IS_ENABLED(CONFIG_SAMPLE_UNICAST) + IS_ENABLED(CONFIG_SAMPLE_BROADCAST)];

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
		int err = -ENOEXEC;

		for (size_t i = 0U; i < ARRAY_SIZE(tx_streams); i++) {
			struct bt_cap_stream *cap_stream = tx_streams[i].stream;
			const struct bt_bap_stream *bap_stream;
			struct bt_bap_ep_info ep_info;

			if (cap_stream == NULL) {
				continue;
			}

			bap_stream = &cap_stream->bap_stream;

			/* No-op if stream is not configured */
			if (bap_stream->ep == NULL) {
				continue;
			}

			err = bt_bap_ep_get_info(bap_stream->ep, &ep_info);
			if (err != 0) {
				continue;
			}

			if (ep_info.state == BT_BAP_EP_STATE_STREAMING) {
				struct net_buf *buf;

				buf = net_buf_alloc(&tx_pool, K_FOREVER);
				net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

				net_buf_add_mem(buf, data, bap_stream->qos->sdu);

				err = bt_cap_stream_send(cap_stream, buf, tx_streams[i].seq_num);
				if (err == 0) {
					tx_streams[i].seq_num++;
				} else {
					LOG_ERR("Unable to send: %d", err);
					net_buf_unref(buf);
				}
			}
		}

		if (err != 0) {
			/* In case of any errors, retry with a delay */
			k_sleep(K_MSEC(100));
		}
	}
}

int cap_initiator_tx_register_stream(struct bt_cap_stream *cap_stream)
{
	if (cap_stream == NULL) {
		return -EINVAL;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(tx_streams); i++) {
		if (tx_streams[i].stream == NULL) {
			tx_streams[i].stream = cap_stream;
			tx_streams[i].seq_num = 0U;

			return 0;
		}
	}

	return -ENOMEM;
}

int cap_initiator_tx_unregister_stream(struct bt_cap_stream *cap_stream)
{
	if (cap_stream == NULL) {
		return -EINVAL;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(tx_streams); i++) {
		if (tx_streams[i].stream == cap_stream) {
			tx_streams[i].stream = NULL;

			return 0;
		}
	}

	return -ENODATA;
}

void cap_initiator_tx_init(void)
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
