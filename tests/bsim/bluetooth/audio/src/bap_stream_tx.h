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
 * @param bap_stream The stream to register
 *
 * @retval 0 on success
 * @retval -EINVAL @p bap_stream is NULL
 * @retval -EINVAL @p bap_stream is not configured for TX
 * @retval -EINVAL @p bap_stream.codec_cfg contains invalid values
 * @retval -ENOMEM if not more streams can be registered
 */
int stream_tx_register(struct bt_bap_stream *bap_stream);

/**
 * @brief Unregister a stream for TX
 *
 * This will remove it to the list of streams the TX thread will attempt to send on.
 *
 * @param bap_stream The stream to unregister
 *
 * @retval 0 on success
 * @retval -EINVAL @p bap_stream is NULL
 * @retval -EINVAL @p bap_stream is not configured for TX
 * @retval -EALREADY @p bap_stream is currently not registered
 */
int stream_tx_unregister(struct bt_bap_stream *bap_stream);

/**
 * @brief Test if the provided stream has been configured for TX
 *
 * @param bap_stream The stream to test for TX support
 *
 * @retval true if it has been configured for TX, and false if not
 */
bool stream_is_tx(const struct bt_bap_stream *stream);

/**
 * @brief Callback to indicate a TX complete
 *
 * @param stream The stream that completed TX
 */
void stream_tx_sent_cb(struct bt_bap_stream *stream);
#endif /* STREAM_TX_H */
