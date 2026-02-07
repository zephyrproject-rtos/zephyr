/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/* Number of streams we support
 * If this value is > 1, then we may not always be able to perform the handover.
 * For example if the remote device supports 2 unicast sink streams, but only 1 broadcast sink
 * stream, then the handover procedure cannot be performed.
 */
#define CAP_STREAM_TX_MAX                                                                          \
	(MIN(CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT, CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT))

struct tx_stream {
	/** Reference to a stream */
	struct bt_cap_stream *stream;

	/** The last sent sequence number */
	uint16_t seq_num;
};

/**
 * @brief Initialize TX
 *
 * This will initialize TX if not already initialized. This creates and starts a thread that
 * will attempt to send data on all streams registered with cap_stream_tx_register_stream().
 */
void cap_stream_tx_init(void);

/**
 * @brief Register a stream for TX
 *
 * This will add it to the list of streams the TX thread will attempt to send on.
 *
 * @retval 0 on success
 * @retval -EINVAL if @p cap_stream is NULL
 * @retval -ENOMEM if not more streams can be registered
 */
int cap_stream_tx_register_stream(struct bt_cap_stream *cap_stream);

/**
 * @brief Unregister a stream for TX
 *
 * This will remove it to the list of streams the TX thread will attempt to send on.
 *
 * @retval 0 on success
 * @retval -EINVAL if @p cap_stream is NULL
 * @retval -EALREADY if the stream is currently not registered
 */
int cap_stream_tx_unregister_stream(struct bt_cap_stream *cap_stream);
