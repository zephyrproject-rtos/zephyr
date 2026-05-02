/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef STREAM_TX_H
#define STREAM_TX_H

#include <stdint.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/types.h>

#include "stream_lc3.h"

struct tx_stream {
	struct bt_bap_stream *bap_stream;
	uint16_t seq_num;

#if defined(CONFIG_LIBLC3)
	struct stream_lc3_tx lc3_tx;
#endif /* CONFIG_LIBLC3 */
};

/**
 * @brief Initialize TX
 *
 * This will initialize TX if not already initialized. This creates and starts a thread that
 * will attempt to send data on all streams registered with stream_tx_register().
 */
void stream_tx_init(void);

/**
 * @brief Register a stream for TX
 *
 * This will add it to the list of streams the TX thread will attempt to send on.
 *
 * @retval 0 on success
 * @retval -EINVAL if @p bap_stream is NULL
 * @retval -EINVAL if @p bap_stream.codec_cfg contains invalid values
 * @retval -ENOEXEC if the LC3 encoder failed to initialize
 * @retval -ENOMEM if not more streams can be registered
 */
int stream_tx_register(struct bt_bap_stream *bap_stream);

/**
 * @brief Unregister a stream for TX
 *
 * This will remove it to the list of streams the TX thread will attempt to send on.
 *
 * @retval 0 on success
 * @retval -EINVAL if @p bap_stream is NULL
 * @retval -EALREADY if the stream is currently not registered
 */
int stream_tx_unregister(struct bt_bap_stream *bap_stream);

#endif /* STREAM_TX_H */
