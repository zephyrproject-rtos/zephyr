/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>

#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/conn.h>

struct tx_stream {
	struct bt_cap_stream *stream;
	uint16_t seq_num;
};

/**
 * @brief Initialize TX
 *
 * This will initialize TX if not already initialized. This creates and starts a thread that
 * will attempt to send data on all streams registered with cap_initiator_tx_register_stream().
 */
void cap_initiator_tx_init(void);

/**
 * @brief Register a stream for TX
 *
 * This will add it to the list of streams the TX thread will attempt to send on.
 *
 * @retval 0 on success
 * @retval -EINVAL if @p cap_stream is NULL
 * @retval -ENOMEM if not more streams can be registered
 */
int cap_initiator_tx_register_stream(struct bt_cap_stream *cap_stream);

/**
 * @brief Unregister a stream for TX
 *
 * This will remove it to the list of streams the TX thread will attempt to send on.
 *
 * @retval 0 on success
 * @retval -EINVAL if @p cap_stream is NULL
 * @retval -EALREADY if the stream is currently not registered
 */
int cap_initiator_tx_unregister_stream(struct bt_cap_stream *cap_stream);
